#!/usr/bin/env python3
"""Run one API process. Does not start/stop DAQ processes."""
import argparse
import os
from pathlib import Path
import uvicorn
from runcon.app import create_app
from runcon.babicmd import Client
from runcon.config import load_settings


def main():
    parser = argparse.ArgumentParser(description='babirl Run Control API')
    parser.add_argument('--config', type=Path,
                        default=Path(__file__).resolve().with_name('config.json'))
    args = parser.parse_args()
    settings = load_settings(args.config)
    token = os.environ.get(settings.api.token_env) or None
    settings.check_exposure(token)
    d = settings.daq
    client = Client(d.babild.host, library=d.library, port=d.babild.port,
                    es_port_base=d.es_port_base)
    print(f'API: http://{settings.api.bind_host}:{settings.api.port}', flush=True)
    print(f'DAQ: {d.name} {d.babild.host} -> {client.host.decode()}:{d.babild.port}', flush=True)
    print(f'Writes: {d.allow_writes}; ES ports: {d.es_port_base}+EFID', flush=True)
    app = create_app(settings, client, token=token)
    uvicorn.run(app,host=settings.api.bind_host,port=settings.api.port,workers=1)

if __name__ == '__main__':
    main()
