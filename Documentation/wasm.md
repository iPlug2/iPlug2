# Building WASM Plugins

iPlug2 supports compiling plugins to WebAssembly (WASM) for running in web browsers. This guide covers the modern split DSP/UI architecture.

## Architecture Overview

The WASM build system creates two separate modules:

| Module | Thread | Purpose |
|--------|--------|---------|
| **DSP** | AudioWorklet | Audio processing, runs in real-time audio thread |
| **UI** | Main | IGraphics rendering, user interaction |

Communication between modules uses `postMessage` for parameters/MIDI and optionally `SharedArrayBuffer` for low-latency visualization data.

### Why Split Architecture?

- **Audio thread isolation**: DSP runs in AudioWorklet, isolated from main thread jank
- **Smaller initial load**: DSP module embedded as BASE64, UI loads asynchronously
- **No WAM SDK dependency**: Uses standard Web Audio API directly
- **Shadow DOM support**: UI encapsulated for embedding in web pages
- **Multi-instance support**: Multiple plugin instances can run in the same AudioContext

## Prerequisites

### Install Emscripten

```bash
cd ~
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
```

Add `source ~/emsdk/emsdk_env.sh` to your shell profile for persistence.

### Server Requirements

For `SharedArrayBuffer` support (needed for visualization data), your server must send these headers:

```
Cross-Origin-Opener-Policy: same-origin
Cross-Origin-Embedder-Policy: require-corp
```

Without these headers, the plugin will fall back to `postMessage` for all communication (higher latency for visualization).

## Building with Makefile

Each example project has a `makedist-wasm.sh` script:

```bash
cd Examples/IPlugEffect/scripts
./makedist-wasm.sh        # Build and launch in Chrome
./makedist-wasm.sh off    # Build only, don't launch browser
./makedist-wasm.sh on safari  # Build and launch in Safari
```

The script:
1. Packages resources (fonts, images, SVGs)
2. Builds DSP module with `SINGLE_FILE=1` (BASE64 embedded)
3. Builds UI module (if `PLUG_HAS_UI=1`)
4. Copies templates and generates the web bundle
5. Outputs to `build-web-wasm/`

### Headless Plugins

For plugins without IGraphics (`PLUG_HAS_UI=0`), only the DSP module is built. The template auto-generates parameter controls.

## Building with CMake

```bash
cd Examples/IPlugEffect
mkdir build && cd build
cmake .. -DTARGET_WASM=ON -G Ninja
ninja
```

Or with Xcode generator:
```bash
cmake .. -DTARGET_WASM=ON -G Xcode
```

CMake targets:
- `IPlugEffect-wasm-dsp` - DSP module
- `IPlugEffect-wasm-ui` - UI module
- `IPlugEffect-wasm-dist` - Full distribution bundle

## Project Configuration

### Makefile Config

Create `config/YourPlugin-wasm.mk`:

```makefile
# Project-specific settings
PLUG_NAME = YourPlugin

# Extra source files
EXTRA_SRC = $(PROJECT_ROOT)/MyDSP.cpp

# Extra include paths
EXTRA_INCLUDES = -I$(PROJECT_ROOT)/libs

# Extra defines
EXTRA_CFLAGS = -DUSE_MY_FEATURE=1
```

### DSP/UI Project Files

Create separate project files for DSP and UI modules:

**`projects/YourPlugin-wasm-dsp.mk`**:
```makefile
include $(IPLUG2_ROOT)/common-wasm.mk
include ../config/YourPlugin-wasm.mk
```

**`projects/YourPlugin-wasm-ui.mk`**:
```makefile
include $(IPLUG2_ROOT)/common-wasm.mk
include ../config/YourPlugin-wasm.mk
```

## Message Protocol

DSP and UI communicate via typed messages:

### UI → DSP (via `postMessage`)

| Type | Fields | Description |
|------|--------|-------------|
| `param` | `paramIdx`, `value` | Parameter change |
| `midi` | `status`, `data1`, `data2` | MIDI message |
| `sysex` | `data` (ArrayBuffer) | SysEx message |
| `arbitrary` | `msgTag`, `ctrlTag`, `data` | Custom message |
| `tick` | - | Idle tick (flush queued messages) |

### DSP → UI (via `postMessage`)

| Verb | Fields | Description |
|------|--------|-------------|
| `SPVFD` | `paramIdx`, `value` | Parameter value from DSP |
| `SCVFD` | `ctrlTag`, `value` | Control value (visualization) |
| `SCMFD` | `ctrlTag`, `msgTag`, `data` | Control message |
| `SAMFD` | `msgTag`, `data` | Arbitrary message |
| `SSMFD` | `data` | SysEx from DSP |
| `pluginInfo` | `data` | Plugin metadata (params, channels) |

### SharedArrayBuffer (Low-Latency Path)

For high-frequency visualization data, `SCVFD`/`SCMFD`/`SAMFD` can use a ring buffer in `SharedArrayBuffer`:

```
Header (16 bytes):
  [0-3]   writeIdx (Uint32, atomic)
  [4-7]   readIdx (Uint32, atomic)
  [8-11]  capacity (Uint32)
  [12-15] reserved

Message:
  [0]     msgType (0=SCVFD, 1=SCMFD, 2=SAMFD)
  [1]     reserved
  [2-3]   dataSize (Uint16)
  [4-7]   ctrlTag (Int32)
  [8-11]  msgTag (Int32)
  [12+]   payload
```

## Template Files

The build copies templates from `IPlug/WEB/TemplateWasm/`:

| File | Purpose |
|------|---------|
| `index.html` | Main page with Web Audio setup |
| `scripts/IPlugWasmBundle.js.template` | Controller class, connects DSP↔UI |
| `scripts/IPlugWasmProcessor.js.template` | AudioWorkletProcessor wrapper |
| `styles/style.css` | Default styling |

Placeholders like `NAME_PLACEHOLDER` are replaced with the plugin name during build.

## IGraphics Popup Menu Styling

When an IGraphics control opens a platform popup menu in a web build, iPlug2 renders it with the browser's HTML Popover API. The generated menu supports submenus, pointer input, Escape dismissal, and keyboard navigation with arrow keys and Enter/Space. A custom IGraphics popup menu control attached with `AttachPopupMenuControl()` still overrides the web platform menu.

Add CSS custom properties to the template stylesheet, or to any stylesheet loaded by the host page:

```css
:root {
  --iplug-popup-menu-background: #1e1e1e;
  --iplug-popup-menu-color: #ddd;
  --iplug-popup-menu-border: 1px solid #555;
  --iplug-popup-menu-border-radius: 6px;
  --iplug-popup-menu-shadow: 0 6px 18px rgba(0, 0, 0, 0.4);
  --iplug-popup-menu-font: 13px -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
  --iplug-popup-menu-padding: 4px 0;
  --iplug-popup-menu-max-height: 70vh;
  --iplug-popup-menu-hover-background: #3b82f6;
  --iplug-popup-menu-hover-color: #fff;
  --iplug-popup-menu-item-padding: 5px 12px;
  --iplug-popup-menu-disabled-opacity: 0.45;
  --iplug-popup-menu-min-width: 160px;
  --iplug-popup-menu-title-padding: 6px 12px 2px;
  --iplug-popup-menu-title-color: #888;
  --iplug-popup-menu-title-font-size: 11px;
  --iplug-popup-menu-separator-margin: 4px 8px;
  --iplug-popup-menu-separator-color: #444;
}
```

For more specific styling, the DOM uses these class hooks: `.iplug-popup-menu`, `.iplug-popup-menu__panel`, `.iplug-popup-menu__item`, `.iplug-popup-menu__check`, `.iplug-popup-menu__submenu-indicator`, `.iplug-popup-menu__title`, and `.iplug-popup-menu__separator`.

## Debugging

### Console Messages

Both DSP and UI modules log to the browser console. DSP messages are prefixed with the plugin name.

### Common Issues

**"SharedArrayBuffer is not defined"**
Server missing COOP/COEP headers. Plugin will work but visualization data uses slower `postMessage` path.

**"Module not found in globalThis"**
DSP module failed to load. Check browser console for WASM compilation errors.

**Audio glitches**
DSP module may be too heavy. Profile with Chrome DevTools Performance panel. Consider:
- Reducing buffer processing
- Disabling SIMD if causing issues
- Checking for allocations in `ProcessBlock`

### Local Development Server

Python 3 with COOP/COEP headers:

```python
#!/usr/bin/env python3
from http.server import HTTPServer, SimpleHTTPRequestHandler
import sys

class CORPHandler(SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
        self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')
        super().end_headers()

    extensions_map = {
        **SimpleHTTPRequestHandler.extensions_map,
        '.wasm': 'application/wasm',
    }

port = int(sys.argv[1]) if len(sys.argv) > 1 else 8000
print(f'Serving at http://localhost:{port}')
HTTPServer(('localhost', port), CORPHandler).serve_forever()
```

Save as `serve.py` and run: `python3 serve.py 8080`

## Multi-Instance Support

The WASM DSP module supports multiple plugin instances running in the same AudioWorklet context. This enables:

- Multiple instances of the same plugin (e.g., two compressors in a chain)
- Plugin chains where audio flows through multiple effects
- Hosting multiple plugins in a single web page

### How It Works

Each `AudioWorkletProcessor` creates its own DSP instance:

```javascript
// In AudioWorkletProcessor constructor
this.instanceId = Module.createInstance();  // Returns unique ID

// All calls include instance ID
Module.init(this.instanceId, sampleRate, blockSize);
Module.processBlock(this.instanceId, inputPtrs, outputPtrs, nFrames);
Module.onParam(this.instanceId, paramIdx, value);
```

The C++ side maintains an instance registry:
- `createInstance()` - allocates new plugin, returns ID
- `destroyInstance(id)` - cleans up instance
- All other functions take `instanceId` as first parameter

### Instance Lifecycle

1. **Create**: Processor constructor calls `Module.createInstance()`
2. **Register**: Processor registers its `port` for postMessage callbacks
3. **Use**: All WASM calls include instance ID
4. **Destroy**: On cleanup, call `Module.destroyInstance(instanceId)`

### Memory Considerations

Each instance has its own:
- Plugin state and parameters
- Audio buffers (allocated separately per processor)
- Message port for DSP→UI communication

The WASM module code is shared across all instances, but each instance has isolated state.

## Comparison with WAM Builds

| Feature | WASM (Split) | WAM |
|---------|--------------|-----|
| SDK dependency | None | WAM SDK required |
| Architecture | Split DSP/UI | Combined |
| AudioWorklet | Native | Via SDK |
| Visualization | SAB + postMessage | WAM events |
| DAW integration | Basic | WAM host support |

Use **WASM Split** for: standalone web plugins, embedding in web apps, simple deployment.

Use **WAM** for: DAW integration, WAM-compatible hosts, standardized plugin format.

## See Also

- [How to setup iPlug2 with emscripten](_wiki/How-to-setup-iPlug2-with-emscripten.md)
- [Build a WAM project using Emscripten and Docker](_wiki/Build-a-WAM-project-using-Emscripten-and-Docker.md)
- [Understanding UI-DSP communication](_wiki/Understanding-UI-DSP-communication.md)
