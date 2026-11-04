"""Serialize complete operations, not just individual native calls."""
from datetime import datetime, timezone
from threading import Lock
import time
from .babicmd import BabirlError

class ApiError(Exception):
    def __init__(self, status, code, message, **details):
        self.status, self.code, self.message = status, code, message
        self.details = details

class RunService:
    def __init__(self, client, settings):
        self.client, self.settings = client, settings
        self.lock = Lock()
        if hasattr(client,'command_timeout_sec'):
            client.command_timeout_sec=settings.daq.command_timeout_sec

    def target(self):
        d = self.settings.daq
        return {'name': d.name, 'babild': d.babild.model_dump(),
                'es_port_base': d.es_port_base, 'allow_writes': d.allow_writes,
                'command_timeout_sec':d.command_timeout_sec}

    def envelope(self, data):
        return {'target': self.target(), 'observed_at': datetime.now(timezone.utc).isoformat(),
                'data': data}

    def invoke(self, command, argument='', mutation=False):
        try:
            return self.client.execute(command, argument)
        except BabirlError as exc:
            # Failed/absent mutation replies cannot prove that no action occurred.
            code = 'operation_result_unknown' if mutation else 'daq_unavailable'
            raise ApiError(502, code, str(exc), outcome='unknown' if mutation else 'unavailable',
                           retry_automatically=False) from exc

    def state(self):
        config = self.invoke('getconfig')
        efn=config.get('ebinfo',{}).get('efn')
        if isinstance(efn,int) and not isinstance(efn,bool) and 0<=efn<256:self.last_efn=efn
        return config, config['runinfo']['runstatus']

    def require_writes(self):
        if not self.settings.daq.allow_writes:
            raise ApiError(403, 'writes_disabled', 'Enable daq.allow_writes in config.json')

    def get_run(self):
        with self.lock:
            config, _ = self.state()
            return self.envelope(config['runinfo'])

    def get_config(self):
        with self.lock:
            config, _ = self.state()
            # Includes EF/SSM/HD information from the original JSON renderer.
            return self.envelope(config)

    def events(self, es=False):
        with self.lock:
            data = self.invoke('getesevtnumber' if es else 'getevtnumber')
            return self.envelope(data)

    def connections(self, reconnect=False, idle_only=False):
        with self.lock:
            config, status = self.state()
            if reconnect:
                self.require_writes()
                if status!='IDLE':raise ApiError(409,'run_not_idle','esconnect requires verified IDLE')
            if idle_only and status!='IDLE':
                return self.envelope({'runstatus':status,'skipped':True,'items':[],'all_on_connected':False,'enabled_count':0,'reconnect_attempted':False})
            may_connect = reconnect and status == 'IDLE' and self.settings.daq.allow_writes
            items = []
            for ef in config.get('eflist', []):
                enabled = str(ef.get('onoff', '')).upper() in ('ON','SCR','PAS')
                checked = enabled
                item = {**ef, 'enabled': enabled, 'connect': None, 'config': None,
                        'connect_requested': False}
                if checked:
                    try:
                        if may_connect and enabled:
                            self.invoke('esconnect', str(ef['efn']), mutation=True)
                            item['connect_requested'] = True
                        result = self.invoke('getesconfig', str(ef['efn']))
                        item['config'] = result['efrc']
                        item['connect'] = result['efrc'].get('connect')
                    except ApiError as exc:
                        item['error'] = "Can't communicate with ES host=%s efn=%s: %s" % (ef.get('host','?'),ef.get('efn','?'),exc.message)
                    except (KeyError, TypeError) as exc:
                        item['error'] = 'Invalid ES configuration response: ' + str(exc)
                items.append(item)
            active = [item for item in items if item['enabled']]
            return self.envelope({'runstatus': status, 'reconnect_attempted': may_connect,
                'all_on_connected': bool(active) and all(item['connect'] == 1 for item in active),
                'enabled_count': len(active), 'items': items})

    def configure(self, changes):
        with self.lock:
            self.require_writes()
            _, status = self.state()
            if status != 'IDLE':
                raise ApiError(409, 'run_not_idle', 'Configuration requires IDLE', runstatus=status)
            completed = []
            operations = {'runname': ('setrunname', str), 'runnumber': ('setrunnumber', str),
                          'header': ('wth', str)}
            try:
                for field, value in changes.items():
                    command, convert = operations[field]
                    self.invoke(command, convert(value), mutation=True)
                    completed.append(field)
                result, _ = self.state()
            except ApiError as exc:
                exc.details['completed_fields'] = completed
                exc.details['atomic'] = False
                raise
            mismatches = {k: {'requested': v, 'observed': result['runinfo'].get(k)}
                          for k,v in changes.items() if result['runinfo'].get(k) != v}
            if mismatches:
                raise ApiError(502, 'verification_failed', 'Readback differs from requested settings',
                               mismatches=mismatches, completed_fields=completed, atomic=False)
            return self.envelope({'operation':'configure', 'verified':True,
                                  'runinfo':result['runinfo'], 'atomic':False})

    def start(self, save):
        with self.lock:
            self.require_writes()
            self.active_action='start' if save else 'nssta'
            try:
                _, status = self.state()
                if status != 'IDLE':
                    raise ApiError(409, 'run_not_idle', 'Start requires IDLE', runstatus=status)
                self.last_stop_attempt=None
                self.last_action_attempt=time.monotonic()
                self.invoke('start' if save else 'nssta', mutation=True)
                try:
                    result, observed = self.state()
                except ApiError as exc:
                    exc.code = 'operation_result_unknown'; exc.details['outcome'] = 'unknown'
                    raise
                expected = 'START' if save else 'NSSTA'
                if observed != expected:
                    raise ApiError(502, 'verification_failed', 'Start state not observed',
                                   expected=expected, observed=observed, outcome='unknown',
                                   retry_automatically=False)
                return self.envelope({'operation':'start','verified':True,'runinfo':result['runinfo']})
            finally:
                self.active_action=None

    def stop(self, ender):
        self.require_writes()
        # Stop must reach EB even when native Start is still waiting for ES.
        while not self.lock.acquire(timeout=.1):
            if getattr(self,'active_action',None) in ('start','nssta') and hasattr(self.client,'cancel_wait'):
                self.client.cancel_wait()
        try:
            self.require_writes()
            self.last_stop_attempt=time.monotonic()
            self.last_action_attempt=self.last_stop_attempt
            self.stop_verified=False
            result=self.invoke('stop', ender, mutation=True)
            observed=result.get('runinfo',{}).get('runstatus')
            if observed != 'IDLE':
                raise ApiError(502, 'verification_failed', 'Stop state not observed',
                               observed=observed, outcome='unknown', retry_automatically=False)
            self.stop_verified=True
            return self.envelope({'operation':'stop','verified':True,'runinfo':result['runinfo']})
        finally:
            self.lock.release()
