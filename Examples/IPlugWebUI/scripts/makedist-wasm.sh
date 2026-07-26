#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
export IPLUG_WASM_PROJECT_ROOT="$SCRIPT_DIR/.."
export IPLUG_WASM_IPLUG2_ROOT="$SCRIPT_DIR/../../.."

exec "$IPLUG_WASM_IPLUG2_ROOT/Scripts/makedist-wasm-webview.sh" "$@"
