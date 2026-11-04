"""Typed webutil operations using the upstream native babirl TCP protocol.

Protocol sources: babirl250503/include/bi-common.h, webutil/babilib.php,
webutil/monitor/{babirl.rb,ridf.rb,show_rawdata.cgi}, babimo/babimo.c.
"""
import ctypes as C
import hashlib
import json
import socket
import struct
import time
from pathlib import Path
from typing import List, Optional, Literal

from fastapi import HTTPException
from pydantic import Field, field_validator, model_validator

from .config import Model
from .service import ApiError

I = struct.Struct("=i")
NAME = C.c_char * 80


class EF(C.Structure):
    _fields_ = [("ex",C.c_int),("of",C.c_int),("name",NAME),("host",NAME)]


class HD(C.Structure):
    _fields_ = [("ex",C.c_int),("of",C.c_int),("path",NAME),
                ("free",C.c_uint64),("full",C.c_uint64),("maxsize",C.c_uint64)]


class SSM(C.Structure):
    _fields_ = [("ex",C.c_int),("of",C.c_int),("host",NAME),("start",NAME),("stop",NAME)]


class ES(C.Structure):
    _fields_ = [("efid",C.c_int),("runname",NAME),("runnumber",C.c_int),
                ("erport",C.c_short),("comport",C.c_short),("erhost",NAME),
                ("hd1",C.c_int),("hd1dir",NAME),("hd2",C.c_int),("hd2dir",NAME),
                ("mt",C.c_int),("mtdir",NAME),("connect",C.c_int)]


def text80(value):
    if "\0" in value or len(value.encode("utf-8")) > 79:
        raise ValueError("Text must contain at most 79 UTF-8 bytes without NUL")
    return value


class EFRow(Model):
    efn: int = Field(ge=0,le=255,strict=True)
    name: str = Field(min_length=1)
    host: str = Field(min_length=1)
    mode: Literal["OFF","ON","SCR","PAS"] = "ON"
    _text = field_validator("name","host")(text80)


class EFWrite(Model):
    rows: List[EFRow] = Field(max_length=256)

    @model_validator(mode="after")
    def unique(self):
        if len({r.efn for r in self.rows}) != len(self.rows):
            raise ValueError("ES numbers must be unique")
        return self


class HDRow(Model):
    hdn: int = Field(ge=0,le=9,strict=True)
    path: str = Field(min_length=1)
    enabled: bool = True
    _text = field_validator("path")(text80)


class HDWrite(Model):
    rows: List[HDRow] = Field(max_length=10)

    @model_validator(mode="after")
    def unique(self):
        if len({r.hdn for r in self.rows}) != len(self.rows):
            raise ValueError("Disk numbers must be unique")
        return self


class SSMWrite(Model):
    enabled: bool = False
    host: str = ""
    start: str = ""
    stop: str = ""
    _text = field_validator("host","start","stop")(text80)


class ESWrite(Model):
    erhost: str
    hd1: bool = False
    hd1dir: str = ""
    hd2: bool = False
    hd2dir: str = ""
    mt: bool = False
    rtdrv: str = ""
    _text = field_validator("erhost","hd1dir","hd2dir","rtdrv")(text80)


class RestartWrite(Model):
    processes: List[Literal["babinfo","babild","babissm","babian"]] = Field(min_length=1,max_length=4)
    efn: Optional[int] = Field(default=None,ge=0,le=255,strict=True)
    force_after_stop: bool = False
    force_after_timeout: bool = False
    wait_timeout_sec: float = Field(default=30,ge=1,le=3600)

    @model_validator(mode="after")
    def unique(self):
        if len(set(self.processes)) != len(self.processes):
            raise ValueError("Duplicate process name")
        return self


class ProcessWrite(Model):
    process: Literal['babinfo','babild','babissm','babian']
    action: Literal['start','stop']
    efn: int = Field(default=1, ge=0, le=255, strict=True)
    allow_unavailable: bool = False
    allow_running: bool = False


def read_exact(conn, size):
    data=bytearray()
    while len(data)<size:
        chunk=conn.recv(size-len(data))
        if not chunk:
            raise OSError("Socket closed before the complete response")
        data.extend(chunk)
    return bytes(data)


def rpc(host,port,command,payload=b"",reply=True,max_bytes=1048576,timeout=5):
    with socket.create_connection((host,port),timeout=timeout) as conn:
        deadline=time.monotonic()+timeout
        conn.sendall(I.pack(4+len(payload))+I.pack(command)+payload)
        if not reply:
            return b""
        length=I.unpack(read_exact(conn,4))[0]
        if length<0 or length>max_bytes:
            raise OSError("Invalid response length")
        data=bytearray()
        while len(data)<length:
            remaining=deadline-time.monotonic()
            if remaining<=0: raise TimeoutError("Protocol deadline exceeded")
            conn.settimeout(remaining)
            chunk=conn.recv(min(length-len(data),65536))
            if not chunk: raise OSError("Truncated response")
            data.extend(chunk)
        return bytes(data)


def decode(value):
    return bytes(value).split(b"\0",1)[0].decode("utf-8",errors="replace")


class Webutil:
    def __init__(self,service):
        self.service=service
        self.settings=service.settings

    def abi(self):
        lib=getattr(self.service.client,"_lib",None)
        if lib is None or not hasattr(lib,"babi_webutil_layout"):
            raise ApiError(503,"rebuild_required","Rebuild the API with the webutil C binding update")
        lib.babi_webutil_layout.argtypes=[C.c_int]
        lib.babi_webutil_layout.restype=C.c_int
        expected=[C.sizeof(EF),C.sizeof(HD),C.sizeof(SSM),C.sizeof(ES),256,10]
        if [lib.babi_webutil_layout(i) for i in range(1,7)] != expected:
            raise ApiError(503,"abi_mismatch","Native babirl struct layout does not match this API")

    def eb(self,command,payload=b""):
        target=self.settings.daq.babild
        return rpc(target.host,target.port,command,payload)

    def native_read(self,command,ctype):
        self.abi()
        data=self.eb(command)
        if len(data)!=C.sizeof(ctype):
            raise OSError("Native reply size mismatch")
        return ctype.from_buffer_copy(data)

    def require_idle(self):
        self.service.require_writes()
        before,status=self.service.state()
        if status!='IDLE':
            raise ApiError(409,"run_not_idle","Settings and process restart require verified IDLE",runstatus=status)
        return before

    def es_target(self,efn,config=None):
        config=config or self.service.state()[0]
        if not 0<=efn<256: raise ApiError(422,"invalid_efn","ES number must be 0..255")
        item=next((item for item in config.get('eflist',[]) if item['efn']==efn),None)
        if item is None: raise ApiError(404,"es_not_found","ES is not registered")
        return item['host'],self.settings.daq.es_port_base+efn

    def read_es(self,efn,config=None):
        self.abi()
        data=rpc(*self.es_target(efn,config),2)
        if len(data)!=C.sizeof(ES): raise OSError("ES configuration size mismatch")
        value=ES.from_buffer_copy(data)
        result={key:decode(getattr(value,key)) if field is NAME else getattr(value,key) for key,field in ES._fields_}
        result['rtdrv']=result.pop('mtdir')
        result['efn']=result.pop('efid')
        return value,result

    def write(self,kind,body,efn=None):
        with self.service.lock:
            before=self.require_idle()
            self.abi()
            try:
                before_es=None
                if kind=='eflist':
                    value=(EF*256)()
                    for row in body.rows:
                        value[row.efn].ex=1; value[row.efn].of=['OFF','ON','SCR','PAS'].index(row.mode)
                        value[row.efn].name=row.name.encode();value[row.efn].host=row.host.encode()
                    command=7; wanted=bytes(value)
                    self.ack(self.eb(command,wanted))
                    observed=bytes(self.native_read(6,EF*256))
                elif kind=='hdlist':
                    original=self.native_read(17,HD*10)
                    value=(HD*10)()
                    for row in body.rows:
                        value[row.hdn]=original[row.hdn]
                        value[row.hdn].ex=1;value[row.hdn].of=int(row.enabled);value[row.hdn].path=row.path.encode()
                    wanted=bytes(value)
                    self.ack(self.eb(18,wanted))
                    read=self.native_read(17,HD*10)
                    # Free/full sizes can change during readback.
                    observed=bytes(read)
                    if any((read[i].ex,read[i].of,bytes(read[i].path)) != (value[i].ex,value[i].of,bytes(value[i].path)) for i in range(10)):
                        raise OSError("HD readback mismatch")
                    observed=wanted
                elif kind=='ssm':
                    value=SSM();value.ex=1;value.of=int(body.enabled)
                    for key in ('host','start','stop'):setattr(value,key,getattr(body,key).encode())
                    wanted=bytes(value)
                    self.ack(self.eb(52,wanted));observed=bytes(self.native_read(51,SSM))
                else:
                    value,before_es=self.read_es(efn,before)
                    for key,item in body.model_dump().items():
                        name='mtdir' if key=='rtdrv' else key
                        setattr(value,name,item.encode() if isinstance(item,str) else int(item))
                    wanted=bytes(value)
                    # babies replies with an int acknowledgement; some variants
                    # return the native struct. Always verify with a separate GET.
                    response=rpc(*self.es_target(efn,before),1,wanted)
                    if len(response)==4:self.ack(response)
                    elif len(response)!=C.sizeof(ES):raise OSError("Invalid ES_SET_CONFIG reply")
                    read,result=self.read_es(efn,before)
                    if any((result[key] != item if isinstance(item,str) else bool(result[key]) != item) for key,item in body.model_dump().items()):
                        raise OSError("ES readback mismatch")
                    observed=wanted
                if observed!=wanted:raise OSError("Configuration readback mismatch")
                after=self.service.state()[0]
                return self.service.envelope({'verified':True,'operation':kind,'before':before,'after':after,
                    **({'before_es':before_es,'es_config':result} if kind=='es' else {})})
            except (OSError,ValueError) as exc:
                raise ApiError(502,"setting_result_unknown",str(exc),outcome='unknown',before=before,retry_automatically=False) from exc

    @staticmethod
    def ack(data):
        if len(data)!=4 or I.unpack(data)[0]==0:raise OSError("DAQ rejected the setting")

    def raw(self):
        data=rpc(self.settings.daq.babild.host,self.settings.daq.babinfo_port,10,max_bytes=1048576)
        return self.service.envelope(parse_ridf(data))

    def process_pid(self,name):
        data=rpc(self.settings.daq.babild.host,self.settings.daq.babimo_port,12,name.encode(),max_bytes=4)
        if len(data)!=4:raise OSError("Invalid babimo PID reply")
        return I.unpack(data)[0]

    def wait_pid(self,name,present,timeout=12):
        deadline=time.monotonic()+timeout
        while True:
            pid=self.process_pid(name)
            if (pid>0)==present:return pid
            if time.monotonic()>=deadline:raise TimeoutError(f"babimo did not confirm {name} {'started' if present else 'stopped'}")
            time.sleep(.2)

    def restart(self,body):
        # Only fixed process names; never accept arbitrary shell commands/arguments.
        selected=set(body.processes)
        if 'babild' in selected:selected.add('babinfo')
        order=[name for name in ('babinfo','babild','babissm','babian') if name in selected]
        if body.force_after_timeout:
            self.service.require_writes()
            attempted=getattr(self.service,'last_action_attempt',None)
            # GUI timing includes delivery/header setup just before native Start.
            if attempted is None or time.monotonic()-attempted<max(0,body.wait_timeout_sec-1):
                raise ApiError(409,'timeout_required','No expired Start/Stop wait is available')
            if hasattr(self.service.client,'cancel_wait'):self.service.client.cancel_wait()
        with self.service.lock:
            self.service.require_writes()
            before={}
            if body.force_after_timeout:
                self.service.last_action_attempt=None
            elif body.force_after_stop:
                attempted=getattr(self.service,'last_stop_attempt',None)
                elapsed=time.monotonic()-attempted if attempted is not None else -1
                if not 5<=elapsed<=120:
                    raise ApiError(409,'stop_attempt_required','Emergency restart requires an unconfirmed Stop attempt 5 to 120 seconds ago')
            else:
                try:
                    before,status=self.service.state()
                except Exception:
                    pass
                else:
                    if status in ('START','NSSTA'):
                        raise ApiError(409,'run_not_idle','Stop acquisition before restarting DAQ')
            live_efn=before.get('ebinfo',{}).get('efn')
            efn=live_efn if isinstance(live_efn,int) and not isinstance(live_efn,bool) and 0<=live_efn<256 else body.efn if body.efn is not None else getattr(self.service,'last_efn',None)
            if 'babild' in order and (not isinstance(efn,int) or not 0<=efn<256):
                raise ApiError(409,'missing_eb_efn','Set the EB fragment number in Edit configuration for offline recovery')
            self.service.last_stop_attempt=None
            steps=[]
            try:
                stop_order=list(reversed(order))
                if 'babinfo' in selected and 'babild' not in stop_order:stop_order.insert(0,'babild')
                for name in stop_order:
                    if self.process_pid(name)>0:
                        rpc(self.settings.daq.babild.host,self.settings.daq.babimo_port,13,name.encode(),reply=False)
                    self.wait_pid(name,False)
                    steps.append({'process':name,'action':'stop','verified':True})
                for name in order:
                    launch=f'babild -l {efn}' if name=='babild' else name
                    rpc(self.settings.daq.babild.host,self.settings.daq.babimo_port,16,launch.encode(),reply=False)
                    pid=self.wait_pid(name,True)
                    steps.append({'process':name,'action':'start','pid':pid,'verified':True})
                return self.service.envelope({'verified':True,'steps':steps,'readiness':'PID verified; device initialization may still be in progress'})
            except OSError as exc:
                raise ApiError(502,'restart_result_unknown',str(exc),outcome='unknown',steps=steps,retry_automatically=False) from exc

    def processes(self):
        items=[]
        for name in ('babinfo','babild','babissm','babian'):
            try:
                pid=self.process_pid(name)
                items.append({'name':name,'pid':pid,'running':pid>0})
            except OSError as exc:
                items.append({'name':name,'pid':None,'running':None,'error':str(exc)})
        return self.service.envelope({'items':items,'babimo_connected':all(item['running'] is not None for item in items)})

    def process_action(self,body):
        with self.service.lock:
            self.service.require_writes()
            try:
                _,status=self.service.state()
            except Exception:
                if not body.allow_unavailable:
                    try:producer_running=self.process_pid('babild')>0
                    except OSError as exc:raise ApiError(502,'babimo_unavailable',str(exc)) from exc
                    if producer_running:
                        raise ApiError(409,'daq_unavailable','DAQ state unavailable while babild is running; explicitly confirm recovery mode first')
            else:
                if status!='IDLE' and not body.allow_running:
                    raise ApiError(409,'run_not_idle','Process actions are blocked while DAQ is running or state is unknown')
            name=body.process
            steps=[]
            try:
                current=self.process_pid(name)
                if body.action=='start' and current>0:
                    raise ApiError(409,'process_already_running',name+' is already running')
                if body.action=='stop' and current<=0:
                    raise ApiError(409,'process_already_stopped',name+' is already stopped')

                def stop(process):
                    if self.process_pid(process)>0:
                        rpc(self.settings.daq.babild.host,self.settings.daq.babimo_port,13,process.encode(),reply=False)
                    pid=self.wait_pid(process,False)
                    steps.append({'process':process,'action':'stop','pid':pid,'verified':True})

                if body.action=='stop':
                    # Stop the producer first, including when the requested process is already absent.
                    order={'babinfo':('babild','babinfo'),
                           'babild':('babild','babinfo'),
                           'babian':('babian',),
                           'babissm':('babissm',)}[name]
                    for process in order:stop(process)
                    pid=self.wait_pid(name,False)
                else:
                    if name=='babild' and self.process_pid('babinfo')<=0:
                        raise ApiError(409,'dependency_not_running','Start babinfo before starting babild')
                    if current<=0:
                        # A newly started babinfo invalidates existing producer/consumer connections.
                        if name=='babinfo':
                            stop('babild')
                        launch=f'babild -l {body.efn}' if name=='babild' else name
                        rpc(self.settings.daq.babild.host,self.settings.daq.babimo_port,16,launch.encode(),reply=False)
                    pid=self.wait_pid(name,True)
                    steps.append({'process':name,'action':'start','pid':pid,'verified':True})
                return self.service.envelope({'verified':True,'process':name,'action':body.action,'pid':pid,'steps':steps})
            except OSError as exc:
                raise ApiError(502,'process_result_unknown',str(exc),outcome='unknown',steps=steps,retry_automatically=False) from exc


CLASS_NAMES={0:'EF BLOCK',1:'EA BLOCK',2:'EAEF BLOCK',3:'EVENT',4:'SEGMENT',5:'COMMENT',6:'EVENT + TIMESTAMP',
             8:'BLOCK NUMBER',9:'END BLOCK',11:'SCALER',12:'CSCALER',13:'SCALER32',16:'TIMESTAMP',21:'STATUS'}
SEGMENT_NAMES=json.loads(Path(__file__).with_name('segids.json').read_text())


def parse_ridf(data,max_rows=300):
    rows=[]
    def walk(start,end,depth=0):
        offset=start
        while offset+8<=end and len(rows)<max_rows:
            first,efn=struct.unpack_from('=II',data,offset)
            layer=(first>>28)&3; cls=(first>>22)&63; size=first&0x3fffff; length=size*2
            if length<8 or offset+length>end:
                rows.append({'offset':offset,'error':'Invalid or truncated RIDF block','class_id':cls,'words':[]})
                break
            label=CLASS_NAMES.get(cls,f'CLASS {cls}')
            description=f'Layer {layer} · {label} · {size} words · EF {efn}'
            prefix={0:8,1:8,2:8,3:12,6:20}.get(cls)
            if cls in (3,6) and length>=12:description+=f' · Event {struct.unpack_from("=I",data,offset+8)[0]}'
            if cls==6 and length>=20:description+=f' · Timestamp {struct.unpack_from("=Q",data,offset+12)[0]}'
            if cls==4 and length>=12:
                segid=struct.unpack_from('=I',data,offset+8)[0]
                description+=f' · Segment {segid}'
                segment={'device':(segid>>20)&63,'focal':(segid>>14)&63,'detector':(segid>>8)&63,'module':segid&255}
            else:segment=None
            if cls in (8,9) and length>=12:description+=f' · Value {struct.unpack_from("=I",data,offset+8)[0]}'
            if cls in (11,12,13) and length>=16:
                date,ident=struct.unpack_from('=II',data,offset+8)
                description+=f' · Date {date} · Scaler ID {ident}'
            if cls in (5,21) and length>16:
                description+=' · '+data[offset+16:offset+length].replace(b'\0',b'').decode('utf-8',errors='replace')[:200]
            visible=min(prefix if prefix and length>=prefix else length,512)
            words=list(struct.unpack_from('='+str(visible//2)+'H',data,offset))
            rows.append({'offset':offset,'class_id':cls,'layer':layer,'size':size,'efn':efn,'description':description,
                         'words':words,'truncated':visible<length and not prefix,'segment':segment})
            if segment:
                rows[-1]['segment_names']={key:SEGMENT_NAMES.get(key,{}).get(str(value),'Unknown') for key,value in segment.items()}
            if prefix and length>prefix and depth<8:walk(offset+prefix,offset+length,depth+1)
            offset+=length
    walk(0,len(data))
    return {'bytes':len(data),'rows':rows,'truncated':len(rows)>=max_rows,'word_columns':8}


def attach_webutil(app,service):
    util=Webutil(service)
    app.state.webutil=util

    @app.get('/api/v1/webutil/processes')
    def processes():return util.processes()

    @app.post('/api/v1/webutil/processes')
    def process_action(body:ProcessWrite):return util.process_action(body)

    @app.get('/api/v1/webutil/config')
    def config():return service.get_config()

    @app.put('/api/v1/webutil/eflist')
    def ef(body:EFWrite):return util.write('eflist',body)

    @app.put('/api/v1/webutil/hdlist')
    def hd(body:HDWrite):return util.write('hdlist',body)

    @app.put('/api/v1/webutil/ssm')
    def ssm(body:SSMWrite):return util.write('ssm',body)

    @app.get('/api/v1/webutil/es/{efn}')
    def read_es(efn:int):
        with service.lock:
            try:return service.envelope(util.read_es(efn)[1])
            except OSError as exc:raise ApiError(502,'es_unavailable',str(exc)) from exc

    @app.put('/api/v1/webutil/es/{efn}')
    def es(efn:int,body:ESWrite):return util.write('es',body,efn)

    @app.get('/api/v1/webutil/raw')
    def raw():
        try:return util.raw()
        except OSError as exc:raise ApiError(502,'raw_unavailable',str(exc)) from exc

    @app.post('/api/v1/webutil/restart')
    def restart(body:RestartWrite):return util.restart(body)
