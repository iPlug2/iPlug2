---
name: webview-ui
description: This skill should be used when the user asks to "create a web UI", "add a WebView", "build an HTML interface", "use Svelte", "use p5.js", "use three.js", "use React", "use web components", "send messages to JavaScript", "receive messages from JavaScript", "hot reload the UI", "use IWebViewControl", "embed a WebView", "WebView editor delegate", "use IPlugSendMsg", "web-based plugin UI", "load HTML in plugin", "use Vite", or discusses WebView setup, JS/C++ messaging, or web framework integration in an iPlug2 plugin.
---

# WebView UI Authoring

Guidance for building WebView-based UIs in iPlug2 plugins. Covers integration mode selection, C++/JS messaging, web framework setup, resource loading, hot reloading, and audio-to-UI visualization.

## Core Pattern

Every full-WebView plugin sets up its editor in the constructor via `mEditorInitFunc`:

```cpp
// In plugin constructor
#ifdef DEBUG
  SetEnableDevTools(true);
#endif
  SetCustomUrlScheme("iplug2"); // optional, needed for some frameworks

  mEditorInitFunc = [&]() {
    LoadIndexHtml(__FILE__, GetBundleID());
    // OR for hot-reload development:
    // LoadURL("http://localhost:5173/");
    EnableScroll(false);
  };
```

The plugin class inherits from `Plugin`. The build configuration must define `WEBVIEW_EDITOR_DELEGATE` and `NO_IGRAPHICS` so the delegate selection header (`IPlugDelegate_select.h`) routes to `WebViewEditorDelegate`.

On the web side, provide `resources/web/index.html` as the entry point. The C++ side automatically injects `IPlugSendMsg()` as the JS-to-native bridge function.

Reference examples: `Examples/IPlugWebUI/` (minimal), `Examples/IPlugSvelteUI/` (modern framework), `Examples/IPlugP5js/` (creative coding).

## Choosing an Integration Mode

| Scenario | Mode | Key Setup |
|----------|------|-----------|
| Entire UI is web content | Full WebView (`WebViewEditorDelegate`) | Define `WEBVIEW_EDITOR_DELEGATE` + `NO_IGRAPHICS`, use `mEditorInitFunc` |
| Mix of native IGraphics + embedded web panel | Embedded (`IWebViewControl`) | Normal IGraphics setup, attach `IWebViewControl` in layout lambda |
| Quick HTML display (logs, status) | Embedded (`IWebViewControl`) with `LoadHTML()` | Attach `IWebViewControl`, call `LoadHTML(str)` directly |

## Choosing a Web Framework

| Need | Framework | Build Step | Example |
|------|-----------|------------|---------|
| Simplest, no tooling | Raw HTML/CSS/JS + Web Components | None | `Examples/IPlugWebUI/` |
| Creative coding / shaders | p5.js (bundled directly) | None | `Examples/IPlugP5js/` |
| Modern reactive UI, TypeScript | Svelte + Vite | `npm run build` | `Examples/IPlugSvelteUI/` |
| Component-heavy UI | React / Vue + Vite | `npm run build` | Follow Svelte pattern |
| 3D visualization | three.js (bundled or via npm) | Optional | Follow p5.js or Svelte pattern |

For detailed framework setup and hot reloading, consult **`references/framework-selection.md`**.

## JavaScript Bridge Functions

**JS to C++ (send via `IPlugSendMsg(jsonObj)`):**

| Function | Purpose | Key Fields |
|----------|---------|------------|
| `SPVFUI(paramIdx, value)` | Send parameter value (normalized 0-1) | `paramIdx`, `value` |
| `BPCFUI(paramIdx)` | Begin parameter change gesture | `paramIdx` |
| `EPCFUI(paramIdx)` | End parameter change gesture | `paramIdx` |
| `SAMFUI(msgTag, ctrlTag, data)` | Send arbitrary message (base64 data) | `msgTag`, `ctrlTag`, `data` |
| `SMMFUI(statusByte, d1, d2)` | Send MIDI message | `statusByte`, `dataByte1`, `dataByte2` |

**C++ to JS (called automatically via `EvaluateJavaScript`):**

| Function | Purpose | Called By |
|----------|---------|-----------|
| `SPVFD(paramIdx, normalizedValue)` | Parameter value changed | `SendParameterValueFromDelegate` |
| `SCVFD(ctrlTag, value)` | Control value changed | `SendControlValueFromDelegate` |
| `SCMFD(ctrlTag, msgTag, dataSize, base64)` | Control message (ISender data) | `SendControlMsgFromDelegate` |
| `SAMFD(msgTag, dataSize, base64)` | Arbitrary message (incl. params init) | `SendArbitraryMsgFromDelegate` |
| `SMMFD(statusByte, d1, d2)` | MIDI message | `SendMidiMsgFromDelegate` |

For the complete messaging protocol, JSON schemas, and binary data exchange, consult **`references/messaging-protocol.md`**.

## Parameter Handling from JavaScript

Parameters require a three-step gesture protocol for proper DAW undo/automation:

```javascript
// 1. Begin gesture (e.g. on mousedown)
BPCFUI(paramIdx);

// 2. Send values during drag (normalized 0-1)
SPVFUI(paramIdx, normalizedValue);

// 3. End gesture (e.g. on mouseup)
EPCFUI(paramIdx);
```

On page load, `WebViewEditorDelegate` automatically sends all parameter info as a JSON blob via `SAMFD` with `msgTag == -1`. Decode it in your `SAMFD` handler:

```javascript
function SAMFD(msgTag, dataSize, msg) {
  if (msgTag == -1 && dataSize > 0) {
    let json = JSON.parse(atob(msg));
    if (json.id === "params") {
      // json.params is an array of {name, type, min, max, default, ...}
      // Use this to configure your UI controls
    }
  }
}
```

Parameter changes from the C++ side arrive via `SPVFD(paramIdx, normalizedValue)`. Define this function globally to handle them.

## Audio-to-UI Data (ISender to WebView)

The C++ side is identical to IGraphics -- declare a sender, feed it in `ProcessBlock`, transmit in `OnIdle`:

```cpp
// .h
IPeakSender<2> mSender;
enum EControlTags { kCtrlTagMeter = 0 };

// ProcessBlock
mSender.ProcessBlock(outputs, nFrames, kCtrlTagMeter);

// OnIdle
mSender.TransmitData(*this);
```

Data arrives in JavaScript via `SCMFD`. Decode the base64 payload to typed arrays:

```javascript
globalThis.SCMFD = (ctrlTag, msgTag, dataSize, msg) => {
  const bytes = new Uint8Array(atob(msg).split('').map(c => c.charCodeAt(0)));
  const header = new Int32Array(bytes.buffer, 0, 3); // [controlTag, nChans, chanOffset]
  const data = new Float32Array(bytes.buffer, 12);   // audio data
  // data[0] = channel 0 peak, data[1] = channel 1 peak, etc.
};
```

For detailed visualization patterns, consult **`references/isender-and-visualization.md`**.

## Embedded WebView in IGraphics

Use `IWebViewControl` to embed a WebView panel within an IGraphics UI:

```cpp
#include "IWebViewControl.h"

// In mLayoutFunc:
pGraphics->AttachControl(new IWebViewControl(bounds, true,
  [](IWebViewControl* pControl) {            // onReady
    pControl->LoadFile("index.html", "com.yourcompany.yourplugin");
  },
  [](IWebViewControl* pControl, const char* jsonMsg) {  // onMessage
    DBGMSG("Received: %s\n", jsonMsg);
  },
  enableDevTools), kCtrlTagWebView);
```

The WebView renders as a native overlay on top of IGraphics. Messages from JS arrive at the `onMessage` lambda as raw JSON -- no automatic parameter routing. Use `pControl->EvaluateJavaScript("...")` to call JS from C++.

## Custom URL Schemes and DevTools

**Custom URL scheme:** `SetCustomUrlScheme("iplug2")` serves web content as if from an `iplug2://` origin. Required when frameworks generate absolute path imports. Set before the WebView opens.

**DevTools:** `SetEnableDevTools(true)` enables the browser inspector (right-click context menu on macOS, F12 on Windows). Typically wrap in `#ifdef DEBUG`.

## Key Source Files

| Concept | File |
|---------|------|
| IWebView base class | `IPlug/Extras/WebView/IPlugWebView.h` |
| WebView editor delegate | `IPlug/Extras/WebView/IPlugWebViewEditorDelegate.h` |
| Editor delegate selection | `IPlug/IPlugDelegate_select.h` |
| Embedded WebView control | `IGraphics/Controls/IWebViewControl.h` |
| ISender (audio-to-UI) | `IPlug/ISender.h` |
| JS bridge (vanilla) | `Examples/IPlugWebUI/resources/web/script.js` |
| JS bridge (TypeScript) | `Examples/IPlugSvelteUI/web-ui/src/lib/iplug.ts` |
| TypeScript declarations | `Examples/IPlugSvelteUI/web-ui/src/types/iplug.d.ts` |
| Raw HTML example | `Examples/IPlugWebUI/` |
| Svelte example | `Examples/IPlugSvelteUI/` |
| p5.js example | `Examples/IPlugP5js/` |
| Embedded WebView example | `Examples/IPlugOSCEditor/` |
| CMake WebView config | `Scripts/cmake/WebView.cmake` |
