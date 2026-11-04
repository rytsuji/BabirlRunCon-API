import secrets
from typing import Optional
from fastapi import Depends, FastAPI, Header, Request
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import JSONResponse
from pydantic import Field, model_validator
from .config import Model
from .service import ApiError, RunService
from .webutil import attach_webutil

class ConfigUpdate(Model):
    runname: Optional[str] = None
    runnumber: Optional[int] = Field(default=None, ge=0, le=2147483647, strict=True)
    header: Optional[str] = None

    @model_validator(mode='after')
    def validate_values(self):
        if not self.model_fields_set:
            raise ValueError('provide at least one setting')
        for name in self.model_fields_set:
            value = getattr(self,name)
            if value is None:
                raise ValueError(name+' cannot be null')
            if isinstance(value,str):
                if not value or '\x00' in value or len(value.encode('utf-8')) > 79:
                    raise ValueError(name+' must contain 1..79 UTF-8 bytes without NUL')
                if name == 'runname' and any(c.isspace() for c in value):
                    raise ValueError('runname must be one token')
        return self

class StartRequest(Model):
    save: bool = Field(strict=True)

class StopRequest(Model):
    ender: str = 'completed'

    @model_validator(mode='after')
    def validate_ender(self):
        if '\x00' in self.ender or len(self.ender.encode('utf-8')) > 79:
            raise ValueError('ender must contain at most 79 UTF-8 bytes without NUL')
        return self


def create_app(settings, client, token=None):
    settings.check_exposure(token)
    service = RunService(client, settings)

    def authorize(authorization: Optional[str] = Header(default=None)):
        if token and not secrets.compare_digest((authorization or '').encode('utf-8'), ('Bearer '+token).encode('utf-8')):
            raise ApiError(401, 'unauthorized', 'Bearer token required')

    app = FastAPI(title='babirl Run Control API', version='0.1.0',
                  dependencies=[Depends(authorize)])
    app.state.service = service
    if settings.api.cors_origins:
        app.add_middleware(CORSMiddleware, allow_origins=settings.api.cors_origins,
                           allow_methods=['GET','PATCH','POST'],
                           allow_headers=['Authorization','Content-Type'])

    @app.middleware('http')
    async def require_json(request: Request, call_next):
        if request.method in ('POST','PATCH'):
            origin = request.headers.get('origin')
            same_origin = str(request.base_url).rstrip('/')
            if origin and origin != same_origin and origin not in settings.api.cors_origins:
                return JSONResponse(status_code=403,content={'error':{'code':'origin_denied'}})
            if request.headers.get('content-type','').split(';')[0].strip() != 'application/json':
                return JSONResponse(status_code=415,content={'error':{'code':'json_required'}})
        return await call_next(request)

    @app.exception_handler(ApiError)
    async def api_error(request, exc):
        return JSONResponse(status_code=exc.status,
                            content={'error':{'code':exc.code,'message':exc.message,**exc.details}})

    @app.get('/api/v1/health')
    def health():
        return {'status':'ok','daq_checked':False}

    @app.get('/api/v1/target')
    def target(): return service.target()

    @app.get('/api/v1/run')
    def run(): return service.get_run()

    @app.get('/api/v1/run/config')
    def config(): return service.get_config()

    @app.get('/api/v1/run/connections')
    def connections_read(idle_only:bool=False): return service.connections(False,idle_only)

    @app.post('/api/v1/run/connections')
    def connections_refresh(): return service.connections(False)

    @app.post('/api/v1/run/connections/connect')
    def connections_connect(): return service.connections(True)

    @app.patch('/api/v1/run/config')
    def configure(body: ConfigUpdate):
        return service.configure(body.model_dump(exclude_unset=True))

    @app.get('/api/v1/run/events')
    def events(): return service.events()

    @app.get('/api/v1/run/events/es')
    def es_events(): return service.events(es=True)

    @app.post('/api/v1/run/start')
    def start(body: StartRequest): return service.start(body.save)

    @app.post('/api/v1/run/stop')
    def stop(body: StopRequest): return service.stop(body.ender)

    attach_webutil(app, service)
    return app
