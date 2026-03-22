# Messaging Protocol Reference

Complete reference for bidirectional C++/JS communication in iPlug2 WebView UIs.

## Overview

- **JS to C++:** Call `IPlugSendMsg(jsonObj)` -- injected automatically by the platform WebView layer
- **C++ to JS:** `WebViewEditorDelegate` calls JS functions via `EvaluateJavaScript()`
- All messages are JSON. Binary data is base64-encoded.

## JS to C++ Messages

Each message is a JSON object with a `"msg"` field identifying the type.

### SPVFUI -- Send Parameter Value

```javascript
function SPVFUI(paramIdx, value) {
  IPlugSendMsg({
    msg: "SPVFUI",
    paramIdx: parseInt(paramIdx),
    value: value  // normalized 0-1
  });
}
```

Arrives at `SendParameterValueFromUI(paramIdx, value)` in C++.

### BPCFUI -- Begin Parameter Change

```javascript
function BPCFUI(paramIdx) {
  IPlugSendMsg({
    msg: "BPCFUI",
    paramIdx: parseInt(paramIdx)
  });
}
```

Arrives at `BeginInformHostOfParamChangeFromUI(paramIdx)`. Must be called before SPVFUI during a gesture (mouse drag, touch).

### EPCFUI -- End Parameter Change

```javascript
function EPCFUI(paramIdx) {
  IPlugSendMsg({
    msg: "EPCFUI",
    paramIdx: parseInt(paramIdx)
  });
}
```

Arrives at `EndInformHostOfParamChangeFromUI(paramIdx)`. Must be called after the gesture ends (mouseup, touchend).

### SAMFUI -- Send Arbitrary Message

```javascript
function SAMFUI(msgTag, ctrlTag = -1, data = 0) {
  IPlugSendMsg({
    msg: "SAMFUI",
    msgTag: msgTag,
    ctrlTag: ctrlTag,
    data: data  // base64-encoded string, or 0 for no data
  });
}
```

Arrives at `OnMessage(msgTag, ctrlTag, dataSize, pData)` in the plugin class. Override `OnMessage` to handle.

### SMMFUI -- Send MIDI Message

```javascript
function SMMFUI(statusByte, dataByte1, dataByte2) {
  IPlugSendMsg({
    msg: "SMMFUI",
    statusByte: statusByte,
    dataByte1: dataByte1,
    dataByte2: dataByte2
  });
}
```

Arrives at `SendMidiMsgFromUI(msg)`. Example: Note On middle C at max velocity: `SMMFUI(0x90, 60, 0x7F)`.

### SKPFUI -- Send Key Press (Internal)

Keyboard events are automatically captured and forwarded. Fields: `keyCode`, `utf8`, `S` (shift), `C` (ctrl), `A` (alt), `isUp`.

## C++ to JS Functions

These global JS functions are called by `WebViewEditorDelegate` via `EvaluateJavaScript`. You must define them in your web UI.

### SPVFD -- Parameter Value From Delegate

```javascript
function SPVFD(paramIdx, val) {
  // val is normalized (0-1)
  // Update your UI control for this parameter
}
```

Called whenever a parameter changes (automation, preset load, host-side edit).

### SCVFD -- Control Value From Delegate

```javascript
function SCVFD(ctrlTag, val) {
  // val is normalized (0-1)
  // Update non-parameter controls by tag
}
```

Called by `SendControlValueFromDelegate(ctrlTag, value)`.

### SCMFD -- Control Message From Delegate

```javascript
function SCMFD(ctrlTag, msgTag, dataSize, msg) {
  // msg is base64-encoded binary data
  // Used by ISender for audio visualization data
  var decoded = atob(msg);
  // Convert to typed arrays as needed
}
```

Called by `SendControlMsgFromDelegate(ctrlTag, msgTag, dataSize, pData)`. This is how ISender data reaches the WebView.

### SAMFD -- Arbitrary Message From Delegate

```javascript
function SAMFD(msgTag, dataSize, msg) {
  // msg is base64-encoded data
  // msgTag == -1 is the auto-init params message
}
```

Called by `SendArbitraryMsgFromDelegate(msgTag, dataSize, pData)`. Also used by `SendJSONFromDelegate(json)` which wraps JSON as `SAMFD(-1, size, base64json)`.

### SMMFD -- MIDI Message From Delegate

```javascript
function SMMFD(statusByte, dataByte1, dataByte2) {
  // Handle MIDI from the plugin (e.g. display on virtual keyboard)
}
```

Called by `SendMidiMsgFromDelegate(msg)`.

## Auto-Initialization (Parameter Info)

When the page finishes loading (`OnWebContentLoaded`), `WebViewEditorDelegate` sends all parameter info as JSON via `SAMFD` with `msgTag == -1`:

```javascript
function SAMFD(msgTag, dataSize, msg) {
  if (msgTag == -1 && dataSize > 0) {
    let json = JSON.parse(atob(msg));
    if (json.id === "params") {
      // json.params is an array:
      // [{name: "Gain", type: 0, min: -70, max: 0, default: -70, ...}, ...]
      json.params.forEach((param, idx) => {
        // Configure your UI controls with param info
      });
    }
  }
}
```

This fires after `OnUIOpen()`, so parameter values are also sent via individual `SPVFD` calls.

## Parameter Gesture Protocol

For proper DAW undo history and automation recording, parameter changes must follow the begin/set/end sequence:

```javascript
// Example: Knob drag handler
function startDrag(e, paramIdx) {
  BPCFUI(paramIdx);  // 1. Begin gesture

  const onMove = (e) => {
    const normalizedValue = calculateValue(e);
    SPVFUI(paramIdx, normalizedValue);  // 2. Send values during drag
  };

  const onEnd = () => {
    EPCFUI(paramIdx);  // 3. End gesture
    document.removeEventListener('mousemove', onMove);
    document.removeEventListener('mouseup', onEnd);
  };

  document.addEventListener('mousemove', onMove);
  document.addEventListener('mouseup', onEnd);
}
```

Values sent via `SPVFUI` must be **normalized (0-1)**. Convert from display range: `normalized = (value - min) / (max - min)`.

## Binary Data Exchange

### C++ to JS (e.g., ISender data)

C++ encodes with `wdl_base64encode`, sends via `SCMFD` or `SAMFD`. JS decodes:

```javascript
function decodeBase64ToBinary(base64Str) {
  const binaryStr = atob(base64Str);
  const bytes = new Uint8Array(binaryStr.length);
  for (let i = 0; i < binaryStr.length; i++) {
    bytes[i] = binaryStr.charCodeAt(i);
  }
  return bytes;
}

// Then reinterpret as needed:
const floats = new Float32Array(bytes.buffer);
const ints = new Int32Array(bytes.buffer, offset, count);
```

### JS to C++ (e.g., sending binary data)

JS encodes with `btoa()`, sends via `SAMFUI`:

```javascript
// Create binary data
const uint8 = new Uint8Array([0, 4, 4, 0]);
const binaryStr = String.fromCharCode.apply(null, uint8);
SAMFUI(msgTag, -1, btoa(binaryStr));
```

C++ receives in `OnMessage(msgTag, ctrlTag, dataSize, pData)` as decoded bytes.

## IWebViewControl Messaging

When using `IWebViewControl` (embedded in IGraphics), there is **no automatic parameter routing**. Messages from JS arrive at the `OnMessageFunc` lambda as a raw JSON string:

```cpp
pGraphics->AttachControl(new IWebViewControl(bounds, true,
  [](IWebViewControl* pControl) {
    pControl->LoadFile("console.html", bundleID);
  },
  [&](IWebViewControl* pControl, const char* jsonMsg) {
    // Parse JSON manually
    // Call SendParameterValueFromUI etc. if needed
  },
  enableDevTools), kCtrlTagWebView);
```

To send data to the embedded WebView:

```cpp
auto* pWebView = pGraphics->GetControlWithTag(kCtrlTagWebView)->As<IWebViewControl>();
pWebView->EvaluateJavaScript("updateDisplay('hello')");
pWebView->LoadHTML("<h1>Status: OK</h1>");  // replace entire content
```
