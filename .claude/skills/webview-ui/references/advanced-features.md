# Advanced Features

File downloads, drag and drop, MIDI, URL navigation, and window resizing in WebView UIs.

## File Downloads

Override these methods in your plugin class to handle file downloads initiated from the WebView:

```cpp
// Allow downloads for specific MIME types (default: all blocked)
bool OnCanDownloadMIMEType(const char* mimeType) override
{
  return std::string_view(mimeType) != "text/html";  // allow everything except HTML
}

// Set where downloaded files are saved
void OnGetLocalDownloadPathForFile(const char* fileName, WDL_String& localPath) override
{
  DesktopPath(localPath);
  localPath.AppendFormatted(MAX_WIN32_PATH_LEN, "/%s", fileName);
}

// Handle successful download
void OnDownloadedFile(const char* path) override
{
  DBGMSG("Downloaded to: %s\n", path);
}

// Handle failed download
void OnFailedToDownloadFile(const char* path) override
{
  DBGMSG("Download failed: %s\n", path);
}

// Track download progress
void OnReceivedData(size_t numBytesReceived, size_t totalNumBytes) override
{
  // Update progress UI if needed
}
```

Trigger a download from JavaScript by navigating to a URL:

```javascript
window.location.href = 'https://example.com/file.wav';
```

## File Drag and Drop (Into WebView)

Use the standard HTML5 Drag and Drop API to accept files dropped onto the WebView:

```javascript
const dropzone = document.getElementById('dropzone');

['dragenter', 'dragover', 'dragleave', 'drop'].forEach(event => {
  dropzone.addEventListener(event, e => { e.preventDefault(); e.stopPropagation(); });
});

dropzone.addEventListener('dragover', () => dropzone.classList.add('dragover'));
dropzone.addEventListener('dragleave', () => dropzone.classList.remove('dragover'));

dropzone.addEventListener('drop', (e) => {
  dropzone.classList.remove('dragover');
  const file = e.dataTransfer.files[0];
  if (!file) return;

  // Read file and send to C++
  const reader = new FileReader();
  reader.onload = () => {
    const bytes = new Uint8Array(reader.result);
    const binaryStr = String.fromCharCode.apply(null, bytes);
    SAMFUI(kMsgTagFileData, -1, btoa(binaryStr));
  };
  reader.readAsArrayBuffer(file);
});
```

See `Examples/IPlugWebUI/resources/web/index.html` for a complete drag-and-drop implementation including drag-out support.

## URL Navigation Filtering

Override `OnCanNavigateToURL` to control which URLs the WebView can navigate to:

```cpp
bool OnCanNavigateToURL(const char* url) override
{
  DBGMSG("Navigating to: %s\n", url);
  // Return false to block navigation
  return true;
}
```

This is called for all navigations including link clicks and `window.location` changes. Useful for preventing users from navigating away from the plugin UI or for logging.

## MIDI from WebView

Send MIDI messages from JavaScript using `SMMFUI`:

```javascript
// Note On: channel 1, middle C, max velocity
SMMFUI(0x90, 60, 0x7F);

// Note Off: channel 1, middle C
SMMFUI(0x80, 60, 0x00);

// CC: channel 1, CC#1 (mod wheel), value 64
SMMFUI(0xB0, 1, 64);

// Virtual keyboard example
function noteOn(noteNumber, velocity = 127) {
  SMMFUI(0x90, noteNumber, velocity);
}

function noteOff(noteNumber) {
  SMMFUI(0x80, noteNumber, 0);
}

// Timed note
function playNote(noteNumber, durationMs = 500) {
  noteOn(noteNumber);
  setTimeout(() => noteOff(noteNumber), durationMs);
}
```

MIDI messages from C++ arrive via `SMMFD(statusByte, dataByte1, dataByte2)`:

```javascript
function SMMFD(statusByte, dataByte1, dataByte2) {
  const type = statusByte & 0xF0;
  if (type === 0x90 && dataByte2 > 0) {
    highlightKey(dataByte1, true);   // Note on
  } else if (type === 0x80 || (type === 0x90 && dataByte2 === 0)) {
    highlightKey(dataByte1, false);  // Note off
  }
}
```

On the C++ side, override `ProcessMidiMsg` to forward MIDI to the WebView:

```cpp
void ProcessMidiMsg(const IMidiMsg& msg) override
{
  SendMidiMsg(msg);  // Forwards to SMMFD in JS
}
```

## Window Resizing from JavaScript

The WebView itself doesn't directly resize the plugin window. Instead, send a message to C++ and call `Resize()`:

### JavaScript side

```javascript
function requestResize(width, height) {
  SAMFUI(kMsgTagResize, -1, btoa(JSON.stringify({ w: width, h: height })));
}
```

Or use simple tag-based messages (no data payload):

```javascript
// Pre-defined sizes
SAMFUI(0);  // small
SAMFUI(1);  // medium
SAMFUI(2);  // large
```

### C++ side

```cpp
// Define message tags
enum EMsgTags {
  kMsgTagSmall = 0,
  kMsgTagMedium = 1,
  kMsgTagLarge = 2
};

bool OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData) override
{
  if (msgTag == kMsgTagSmall)
    Resize(512, 335);
  else if (msgTag == kMsgTagMedium)
    Resize(1024, 335);
  else if (msgTag == kMsgTagLarge)
    Resize(1024, 768);
  return false;
}
```

`Resize()` is a method on `WebViewEditorDelegate` that resizes both the host window and the WebView. See `Examples/IPlugWebUI/` for the complete resize pattern.

## Sending JSON from C++

For structured data beyond the standard message types, use `SendJSONFromDelegate`:

```cpp
nlohmann::json msg;
msg["id"] = "status";
msg["connected"] = true;
msg["latency"] = 2.1;
SendJSONFromDelegate(msg);
```

This arrives via `SAMFD` with `msgTag == -1`. Decode in JavaScript:

```javascript
function SAMFD(msgTag, dataSize, msg) {
  if (msgTag == -1 && dataSize > 0) {
    const json = JSON.parse(atob(msg));
    if (json.id === "status") {
      updateStatusDisplay(json);
    }
  }
}
```
