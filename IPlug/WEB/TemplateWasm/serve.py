#!/usr/bin/env python3
"""Serve an iPlug2 Wasm distribution with SharedArrayBuffer headers."""

from __future__ import annotations

import argparse
import os
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer


class IPlugWasmRequestHandler(SimpleHTTPRequestHandler):
  extensions_map = {
    **SimpleHTTPRequestHandler.extensions_map,
    ".wasm": "application/wasm",
    ".js": "text/javascript",
  }

  def end_headers(self) -> None:
    self.send_header("Cross-Origin-Opener-Policy", "same-origin")
    self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
    self.send_header("Cross-Origin-Resource-Policy", "same-origin")
    super().end_headers()


def main() -> None:
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument("-b", "--bind", default="127.0.0.1", help="Address to bind")
  parser.add_argument("-p", "--port", default=8080, type=int, help="Port to listen on")
  parser.add_argument(
    "-d",
    "--directory",
    default=os.path.dirname(os.path.abspath(__file__)),
    help="Directory to serve",
  )
  args = parser.parse_args()

  directory = os.path.abspath(args.directory)
  handler = partial(IPlugWasmRequestHandler, directory=directory)
  server = ThreadingHTTPServer((args.bind, args.port), handler)
  print(f"Serving {directory} at http://{args.bind}:{args.port}/")
  print("SharedArrayBuffer headers enabled.")

  try:
    server.serve_forever()
  except KeyboardInterrupt:
    pass
  finally:
    server.server_close()


if __name__ == "__main__":
  main()
