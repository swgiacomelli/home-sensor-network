import http.server
import socketserver
import os.path
import sys
import json
from secrets import token_hex

PORT = 8000

_STATIC_FILES = {
    "/": "index.html",
    "/config.css": "config.css",
    "/config.js": "config.js",
}

_MIME_TYPES = {
    "html": "text/html",
    "css": "text/css",
    "js": "application/javascript",
    "json": "application/json"
}
_DEFAULT_MIME_TYPE = "text/plain"


def _get_mime_type(filename):
    _, ext = os.path.splitext(filename)

    if len(ext) > 1:
        ext = ext[1:]

    return _MIME_TYPES.get(ext, _DEFAULT_MIME_TYPE)

def _get_networks():
    return json.dumps([token_hex(10) for _ in range(10)])

def _get_values():
    return json.dumps({"deviceID":"Test Device", "mqttPort":1883})

def _get_update_response():
    return json.dumps({"success":True, "message": "Device successfully updated - going to sensor mode."})

class TestServer(http.server.BaseHTTPRequestHandler):
    def _serve_static(self):
        self.send_response(200)
        self.send_header("Content-type",
                         _get_mime_type(_STATIC_FILES[self.path]))
        self.end_headers()
        with open(f"/static/{_STATIC_FILES[self.path]}", "rt") as f:
            data = f.read()
            self.wfile.write(data.encode('utf-8'))

    def _serve_not_found(self):
        self.send_response(404)
        self.send_header("Content-type", _DEFAULT_MIME_TYPE)
        self.end_headers()
        self.wfile.write(f"file: {self.path} not found".encode("utf-8"))

    def _serve_json(self, data:str):
        self.send_response(200)
        self.send_header("Content-type",
                         _MIME_TYPES["json"])
        self.end_headers()
        self.wfile.write(data.encode('utf-8'))

    def do_GET(self):
        if self.path in _STATIC_FILES:
            self._serve_static()
            return

        if self.path == "/networks":
            self._serve_json(_get_networks())
            return

        if self.path == "/values":
            self._serve_json(_get_values())
            return

        self._serve_not_found()

    def do_POST(self):
        if self.path == "/update":
            self._serve_json(_get_update_response())
            return
        self._serve_not_found()

if __name__ == "__main__":
    Handler = TestServer

    with socketserver.TCPServer(("", PORT), TestServer) as httpd:
        sys.stderr.write(f"serving http://localhost:{PORT}\n")
        sys.stderr.flush()
        httpd.serve_forever()
