"""Isolated, read-only native client for configuration requests."""
import json
import sys
from .babicmd import Client


def main():
    try:
        options=json.load(sys.stdin)
        client=Client(**options)
        result={'data':client.execute('getconfig')}
    except Exception as exc:
        result={'error':str(exc)}
    print(json.dumps(result))


if __name__=='__main__':
    main()
