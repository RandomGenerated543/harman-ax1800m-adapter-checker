#!/usr/bin/env python3
"""Read adapter identity/UI and SSH/Telnet banners. No login, upload or CGI commands."""
import argparse
import ipaddress
import json
from pathlib import Path
import socket
import urllib.request

class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, *args, **kwargs):
        raise ValueError('Redirect refused')

def local_host(value):
    address = ipaddress.ip_address(value)
    if not (address.is_private or address.is_loopback) or address.version != 4:
        raise ValueError('Use the adapter local IPv4 address')
    return str(address)

def banner(host, port):
    try:
        with socket.create_connection((host, port), timeout=3) as conn:
            conn.settimeout(2)
            try:
                data = conn.recv(512)
            except socket.timeout:
                data = b''
            return {'open': True, 'banner': data.decode('ascii', 'replace')}
    except OSError as error:
        return {'open': False, 'error': str(error)}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--host', default='192.168.50.100')
    parser.add_argument('--out', default='harman-adapter-probe')
    parser.add_argument('--dry-run', action='store_true')
    args = parser.parse_args()
    host = local_host(args.host)
    if args.dry_run:
        print('DRY_RUN=PASS requests=0 writes_to_adapter=0')
        return
    dest = Path(args.out)
    dest.mkdir(parents=True, exist_ok=False)
    opener = urllib.request.build_opener(urllib.request.ProxyHandler({}), NoRedirect())
    report = {'host': host, 'writes_to_adapter': 0, 'authentication_attempts': 0, 'http': {}}
    for path, name in [('/cgi-bin/getversion.cgi', 'version.txt'), ('/', 'web-page.html')]:
        request = urllib.request.Request('http://' + host + path)
        with opener.open(request, timeout=10) as response:
            body = response.read(2 * 1024 * 1024 + 1)
            if len(body) > 2 * 1024 * 1024:
                raise ValueError('Oversized response')
            (dest / name).write_bytes(body)
            report['http'][path] = {'status': response.status, 'headers': dict(response.headers), 'bytes': len(body)}
        if name == 'version.txt':
            text = body.decode('utf-8', 'replace')
            identity = {k.strip(): v.strip() for line in text.splitlines() if ':' in line
                        for k, v in [line.split(':', 1)]}
            report['identity'] = {k: identity.get(k) for k in ('platform', 'custom', 'product_Model',
                                                             'system_version', 'protocol_version')}
            expected = ('AX1800M', 'suding', '202603120956CA', '20260109175612')
            actual = tuple(identity.get(k) for k in ('platform', 'custom', 'system_version', 'protocol_version'))
            if actual != expected:
                raise ValueError('Different adapter identity; stop here and send version.txt')
            print('IDENTITY=' + json.dumps(report['identity']), flush=True)
    for port in (22, 23):
        report[str(port)] = banner(host, port)
        print('PORT=' + str(port) + ' ' + json.dumps(report[str(port)]), flush=True)
    (dest / 'REPORT.json').write_text(json.dumps(report, indent=2) + '\n')
    print('PROBE=COMPLETE folder=' + str(dest.resolve()) + ' uploads=0 installs=0 reboots=0')

if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError) as error:
        raise SystemExit('PROBE=FAIL ' + str(error) + '; no upload/installation requested')
