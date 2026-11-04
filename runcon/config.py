"""Validated configuration; paths are resolved relative to the config file."""
import ipaddress
import json
from pathlib import Path
from typing import List, Optional
from pydantic import BaseModel, ConfigDict, Field

class Model(BaseModel):
    model_config = ConfigDict(extra='forbid')

class ApiConfig(Model):
    bind_host: str = '127.0.0.1'
    port: int = Field(default=8000, ge=1, le=65535, strict=True)
    cors_origins: List[str] = Field(default_factory=list)
    token_env: str = 'BABIRL_API_TOKEN'

class Endpoint(Model):
    host: str = Field(min_length=1, max_length=127)
    port: int = Field(default=17511, ge=1, le=65535, strict=True)

class DaqConfig(Model):
    name: str = 'Main DAQ'
    babild: Endpoint
    es_port_base: int = Field(default=17651, ge=1, le=65280, strict=True)
    allow_writes: bool = Field(default=False, strict=True)
    command_timeout_sec: int = Field(default=300, ge=1, le=3600, strict=True)
    babimo_port: int = Field(default=17518, ge=1, le=65535, strict=True)
    babinfo_port: int = Field(default=17516, ge=1, le=65535, strict=True)
    library: Optional[str] = None

class Settings(Model):
    api: ApiConfig = Field(default_factory=ApiConfig)
    daq: DaqConfig

    def check_exposure(self, token):
        # Require an explicit token for any non-loopback bind, including 0.0.0.0.
        try:
            loopback = ipaddress.ip_address(self.api.bind_host).is_loopback
        except ValueError:
            loopback = False
        if not loopback and not token:
            raise ValueError('non-loopback bind requires '+self.api.token_env)
        if '*' in self.api.cors_origins:
            raise ValueError('cors_origins must list explicit GUI origins')


def load_settings(path):
    path = Path(path).resolve()
    settings = Settings.model_validate(json.loads(path.read_text(encoding='utf-8')))
    if settings.daq.library:
        settings.daq.library = str((path.parent / settings.daq.library).resolve())
    return settings
