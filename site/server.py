#!/usr/bin/env python3
"""Serve o site e expõe o JSON escrito pelo programa em C.
   GET  /api/data      -> conteúdo de /tmp/triolink.json
   POST /api/shutdown  -> sudo /sbin/shutdown -h now
Uso: python3 server.py [porta]   (padrão 8080)"""
import json, os, subprocess, sys
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer

JSON_PATH = "/tmp/triolink.json"
HERE = os.path.dirname(os.path.abspath(__file__))


class Handler(SimpleHTTPRequestHandler):
    def __init__(self, *a, **kw):
        super().__init__(*a, directory=HERE, **kw)

    def _json(self, code, payload):
        body = json.dumps(payload).encode()
        self.send_response(code)
        self.send_header("Content-Type", "application/json")
        self.send_header("Cache-Control", "no-store")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        if self.path.split("?")[0] == "/api/data":
            try:
                with open(JSON_PATH) as f:
                    self._json(200, json.load(f))
            except (OSError, ValueError):
                self._json(503, {"erro": "sem dados do programa em C"})
        else:
            super().do_GET()

    def do_POST(self):
        if self.path == "/api/shutdown":
            self._json(200, {"ok": True})
            subprocess.Popen(["sudo", "/sbin/shutdown", "-h", "now"])
        else:
            self._json(404, {"erro": "rota inexistente"})


if __name__ == "__main__":
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8080
    print(f"TrioLink em http://0.0.0.0:{port}")
    ThreadingHTTPServer(("0.0.0.0", port), Handler).serve_forever()
