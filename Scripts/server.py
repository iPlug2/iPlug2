import BaseHTTPServer, SimpleHTTPServer
SimpleHTTPServer.SimpleHTTPRequestHandler.extensions_map['.wasm'] = 'application/wasm'
port = 8000
httpd = BaseHTTPServer.HTTPServer(('localhost', 8000), SimpleHTTPServer.SimpleHTTPRequestHandler)
print("Serving at http://localhost:8000/ waiting for connections")
httpd.serve_forever()
