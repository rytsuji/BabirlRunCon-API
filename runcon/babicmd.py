"""Python 3 standard-library-only wrapper; no executable subprocesses."""
import ctypes as C
import json
from pathlib import Path
import sys
import socket


class BabirlError(RuntimeError):
    def __init__(self, message, raw=None):
        super().__init__(message)
        self.raw = raw


class Client:
    def __init__(self, host, library=None, port=17511, es_port_base=17651, command_timeout_sec=300):
        self.command_timeout_sec=command_timeout_sec
        if isinstance(port, bool) or not isinstance(port, int) or not 1 <= port <= 65535:
            raise ValueError('port must be an integer in [1, 65535]')
        if isinstance(es_port_base, bool) or not isinstance(es_port_base, int) or not 1 <= es_port_base <= 65280:
            raise ValueError('es_port_base must be in [1, 65280]')
        self.es_port_base = es_port_base
        self.port = port
        self._encode(host)  # Validate type/NUL before name resolution.
        try:
            socket.inet_pton(socket.AF_INET, host)
            address = host
        except OSError:
            try:
                addresses = socket.getaddrinfo(host, port, socket.AF_INET,
                                               socket.SOCK_STREAM)
                address = addresses[0][4][0]
            except (OSError, UnicodeError, IndexError) as exc:
                raise BabirlError(f'cannot resolve IPv4 host {host!r}: {exc}') from exc
        self.host = self._encode(address)
        library = library or Path(__file__).with_name(
            'libbabicmd.dylib' if sys.platform == 'darwin' else 'libbabicmd.so')
        self._lib = C.CDLL(str(Path(library).resolve()))
        self._lib.babi_call_target.argtypes = [C.c_char_p, C.c_char_p, C.c_char_p,
                                        C.c_int, C.c_int, C.POINTER(C.c_void_p)]
        self._lib.babi_call_target.restype = C.c_int
        self._lib.babi_call_target_timed.argtypes = [C.c_char_p,C.c_char_p,C.c_char_p,C.c_int,C.c_int,C.c_int,C.POINTER(C.c_void_p)]
        self._lib.babi_call_target_timed.restype = C.c_int
        self._lib.babi_cancel_wait.argtypes=[]
        self._lib.babi_cancel_wait.restype=None
        self._lib.babi_free.argtypes = [C.c_void_p]
        self._lib.babi_free.restype = None

    @staticmethod
    def _encode(value):
        if not isinstance(value, str):
            raise TypeError('expected str')
        if '\0' in value:
            raise ValueError('NUL is forbidden')
        return value.encode('utf-8')

    def raw(self, command, argument=''):
        out = C.c_void_p()
        status = self._lib.babi_call_target_timed(self.host, self._encode(command),
                                     self._encode(argument), self.port, self.es_port_base, self.command_timeout_sec, C.byref(out))
        try:
            text = C.string_at(out).decode('utf-8') if out.value else ''
        finally:
            if out.value:
                self._lib.babi_free(out)
        if status == -1:
            raise ValueError('unsupported command or invalid argument (max 79 UTF-8 bytes)')
        if status == -2:
            raise MemoryError('native output allocation failed')
        if status:
            reason = text.strip() or f'native status {status}'
            try:
                payload = json.loads(text)
                if isinstance(payload, dict):
                    reason = str(payload.get('error', reason))
            except (json.JSONDecodeError, TypeError):
                pass
            raise BabirlError(f'native transport failure: {reason}', text)
        return text

    def cancel_wait(self):
        self._lib.babi_cancel_wait()

    def execute(self, command, argument=''):
        text = self.raw(command, argument)
        try:
            result = json.loads(text)
        except json.JSONDecodeError as exc:
            raise BabirlError('upstream output is not valid JSON', text) from exc
        if isinstance(result, dict) and 'error' in result:
            raise BabirlError(str(result['error']), text)
        return result

    def getconfig(self):
        return self.execute('getconfig')

    def getevtnumber(self):
        return self.execute('getevtnumber')['eventbuiltnumber']

    def setrunnumber(self, number):
        if isinstance(number, bool) or not isinstance(number, int) or not 0 <= number <= 2147483647:
            raise ValueError('number must be an integer in [0, 2147483647]')
        return self.execute('setrunnumber', str(number))

    def start(self):
        return self.execute('start')

    def stop(self, ender=''):
        return self.execute('stop', ender)
