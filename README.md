# babirl Run Control API update

Copy the files in this directory into your existing babirl-runcon-api directory,
keeping your `config.json` and token configuration. Then follow these steps.

## Install

Requires Python 3.9 or later, a C compiler, and the readline, ncurses and Expat
development libraries. On AlmaLinux:

```sh
sudo dnf install gcc python3-pip readline-devel ncurses-devel expat-devel
python3 -m venv .venv
.venv/bin/python -m pip install -r requirements.txt
.venv/bin/python build.py
```

## Configure and run

Edit `config.json`:

- `api.bind_host`, `api.port`: API listening address and port.
- `daq.babild.host`, `daq.babild.port`: babild server; usual port `17511`.
- `daq.es_port_base`: ES base port; usual value `17651`.
- `daq.allow_writes`: set `true` to allow DAQ control.
- `daq.command_timeout_sec`: API command wait limit; default `300` seconds.

For a non-loopback listening address, set the token environment variable named
by `api.token_env` (default `BABIRL_API_TOKEN`) before starting the API.
Set the same token in the GUI server environment and register its variable name
in the GUI's API settings.

```sh
.venv/bin/python babirl_runcon.py
```

API documentation is available at `http://<API-server-IP>:<API-port>/docs`.
Register `http://<API-server-IP>:<API-port>` as the Run Control API base URL
in the Web GUI. The API host must be able to reach babild and the configured ES.
Process Manager and Restart DAQ also require babimo on the Event Builder host.

Stop with Ctrl+C. Restart the API after changing `config.json`.

## Update

Keep `config.json` and any token configuration. Replace the supplied source
files, run `.venv/bin/python build.py`, then restart the API.
