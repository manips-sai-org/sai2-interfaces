# adopted from Toki Migimatsu's CS223A homework script

from __future__ import print_function, division
import threading
from multiprocessing import Process
from argparse import ArgumentParser
from urllib.parse import urlparse, parse_qs
import redis
import json
import os
import shutil
import cgi
import mimetypes
import sys
import logger
WEB_DIRECTORY = os.path.join(os.path.dirname(__file__), "..", "web")

if sys.version.startswith("3"):
    from http.server import HTTPServer
else:
    from BaseHTTPServer import HTTPServer


if sys.version.startswith("3"):
    from http.server import BaseHTTPRequestHandler
else:
    from BaseHTTPServer import BaseHTTPRequestHandler


def makeHTTPRequestHandler(get_callback=None, post_callback=None, callback_args={}):
    """
    Factory method to create HTTPRequestHandler class with custom GET and POST callbacks.

    Usage:

    extra_args = {"random_num": 123}
    http_server = HTTPServer(("", args.http_port), makeHTTPRequestHandler(handle_get_request, handle_post_request, extra_args))
    http_server.serve_forever()

    def handle_get_request(http_request_handler, get_vars, **kwargs):
        with open("index.html","rb") as f:
            http_request_handler.wfile.write(f.read())

    def handle_post_request(http_request_handler, post_vars, **kwargs):
        for key, val in post_vars.items():
            print("%s: %s" % (key, val))
            print(kwargs["random_num"])
    """

    class HTTPRequestHandler(BaseHTTPRequestHandler):

        if not mimetypes.inited:
            mimetypes.init() # try to read system mime.types
        extensions_map = mimetypes.types_map.copy()
        extensions_map.update({
            "": "text/html" # Default
        })

        def __init__(self, request, client_address, server):
            BaseHTTPRequestHandler.__init__(self, request, client_address, server)

        def guess_type(self, path):
            """
            Guess the mime type of a file.
            """

            base, ext = os.path.splitext(path)
            if ext in self.extensions_map:
                return self.extensions_map[ext]

            ext = ext.lower()
            if ext in self.extensions_map:
                return self.extensions_map[ext]

            return self.extensions_map[""]

        def set_headers(self):
            """
            Return OK message.
            """
            self.send_response(200)
            self.send_header("Content-type", self.guess_type(self.path))
            self.end_headers()

        def do_GET(self):
            """
            Parse GET request and call get_callback(HTTPRequestHandler, get_vars, **callback_args).
            """
            self.set_headers()

            # Call get_callback argument
            if get_callback is not None:
                get_callback(self, None, **callback_args)

        def do_POST(self):
            """
            Parse POST request and call post_callback(HTTPRequestHandler, post_vars, **callback_args)
            """

            self.set_headers()

            # Parse post content
            content_type, parse_dict = cgi.parse_header(self.headers["Content-Type"])
            parse_dict = {key: val.encode("utf-8") for key, val in parse_dict.items()}
            if content_type == "multipart/form-data":
                post_vars = cgi.parse_multipart(self.rfile, parse_dict)
            elif content_type == "application/x-www-form-urlencoded":
                content_length = int(self.headers["Content-Length"])
                post_vars = cgi.parse_qs(self.rfile.read(content_length), keep_blank_values=1)
            else:
                post_vars = {}

            # Call post_callback argument
            if post_callback is not None:
                post_callback(self, post_vars, **callback_args)

    return HTTPRequestHandler


def handle_get_request(request_handler, get_vars, **kwargs):
    if request_handler.path.startswith('/redis'):
        get_redis_value(request_handler, **kwargs)
    else:
        serve_file(request_handler)

def get_redis_value(request_handler, **kwargs):
    query_params = parse_qs(urlparse(request_handler.path).query)
    key = query_params["key"][0]
    value = kwargs["redis_db"].get(key)
    request_handler.wfile.write(value.encode("utf-8"))

def serve_file(request_handler):
    # Serve content inside WEB_DIRECTORY
    path_tokens = [token for token in request_handler.path.split("/") if token]

    # Default to index.html
    if not path_tokens or ".." in path_tokens:
        request_path = "index.html"
    else:
        request_path = os.path.join(*path_tokens)
    
    request_path = os.path.join(WEB_DIRECTORY, request_path)
    print('request_path: ' + request_path)

    # Check if file exists
    if not os.path.isfile(request_path):
        request_handler.send_error(404, "File not found.")
        return

    # Otherwise send file directly
    with open(request_path, "rb") as f:
        shutil.copyfileobj(f, request_handler.wfile)


def handle_post_request(request_handler, post_vars, **kwargs):
    """
    HTTPRequestHandler callback:

    Set POST variables as Redis keys
    """
    path = request_handler.path
    if path.startswith('/log'):
        handle_log(request_handler, **kwargs)
    elif path.startswith('/redis'):
        set_redis_key_vals(post_vars, **kwargs)

def handle_log(request_handler):
    path = request_handler.path
    if path.startswith('/log/start'):
        dir = '../logs'
        logger.start_logging(dir, kwargs["redis_db"])

    elif path.startswith('/log/stop'):
        logger.stop_logging()

def set_redis_key_vals(post_vars, **kwargs):
    for key, val_str in post_vars.items():
        val = val_str[0].decode('utf-8')
        print("%s: %s" % (key, val))
        kwargs["redis_db"].set(key, val)


if __name__ == "__main__":
    # Parse arguments
    parser = ArgumentParser(description=(
        "Monitor Redis keys in the browser."
    ))
    parser.add_argument("-hp", "--http_port", help="HTTP Port (default: 8000)", default=8000, type=int)
    parser.add_argument("-wp", "--ws_port", help="WebSocket port (default: 8001)", default=8001, type=int)
    parser.add_argument("-rh", "--redis_host", help="Redis hostname (default: localhost)", default="localhost")
    parser.add_argument("-rp", "--redis_port", help="Redis port (default: 6379)", default=6379, type=int)
    parser.add_argument("-rd", "--redis_db", help="Redis database number (default: 0)", default=0, type=int)
    parser.add_argument("-r", "--refresh_rate", help="Redis refresh rate in seconds (default: 0.05)", default=0.05, type=float)
    parser.add_argument("--realtime", action="store_true", help="Subscribe to realtime Redis SET pubsub notifications")
    args = parser.parse_args()

    # connect to redis
    redis_db = redis.Redis(host=args.redis_host, port=args.redis_port, db=args.redis_db, decode_responses=True)

    # Create HTTPServer
    get_post_args = {"redis_db": redis_db}
    http_server = HTTPServer(("", args.http_port), makeHTTPRequestHandler(handle_get_request, handle_post_request, get_post_args))
    http_server_process = Process(target=http_server.serve_forever)
    http_server_process.start()
    print("Started HTTP server on port %d" % (args.http_port))

    # create redis monitor
    # redis_monitor = RedisMonitor(host=args.redis_host, port=args.redis_port, db=args.redis_db)
    # redis_monitor.run_forever(ws_server)

    http_server_process.join()
