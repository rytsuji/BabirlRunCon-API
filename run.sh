#!/bin/sh
set -eu
cd "$(dirname "$0")"


if [ ! -d .venv ]; then
  python3 -m venv .venv
fi


.venv/bin/python -m pip install -r requirements.txt


exec .venv/bin/python babirl_runcon.py
