# ISender and Visualization

How to send real-time audio data from the DSP thread to a WebView UI for visualization.

## Architecture

```
Audio Thread                    Main Thread                   WebView
┌──────────┐  lock-free queue  ┌──────────────┐  JS call    ┌────────────┐
│ProcessBlock├─────────────────►│OnIdle/       ├────────────►│SCMFD()     │
│ sender.   │                  │TransmitData  │  base64     │ decode     │
│ Process   │                  │ SendControl  │  encoded    │ typed array│
│ Block()   │                  │ MsgFrom      │             │ render     │
└──────────┘                   │ Delegate()   │             └────────────┘
                               └──────────────┘
```

The ISender system is the same on the C++ side whether you use IGraphics or WebView. The difference is the receiving end: instead of an `IControl::OnMsgFromDelegate`, data arrives at a JavaScript `SCMFD` function.

## C++ Setup

Identical to the IGraphics ISender pattern:

### 1. Declare sender and control tag

```cpp
// Plugin.h
#include "ISender.h"

enum EControlTags { kCtrlTagMeter = 0 };

class MyPlugin : public Plugin {
  IPeakSender<2> mSender;  // 2-channel peak sender
};
```

### 2. Feed data in ProcessBlock

```cpp
void MyPlugin::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  // ... DSP processing ...
  mSender.ProcessBlock(outputs, nFrames, kCtrlTagMeter);
}
```

### 3. Transmit in OnIdle

```cpp
void MyPlugin::OnIdle()
{
  mSender.TransmitData(*this);
}
```

### 4. Reset on sample rate change (if needed)

```cpp
void MyPlugin::OnReset()
{
  mSender.Reset(GetSampleRate());
}
```

### Available Sender Types

| Sender | Data Sent | Use Case |
|--------|-----------|----------|
| `IPeakSender<N>` | Peak values per channel | Level meters |
| `IPeakAvgSender<N>` | Peak + average per channel | LED meters with ballistics |
| `IBufferSender<N>` | Raw sample buffers | Oscilloscope / waveform |
| `ISpectrumSender<N>` | FFT magnitude data | Spectrum analyzer |
| `ISender<N,Q,T>` | Custom typed data | Generic visualization |

## JavaScript Receiver Pattern

ISender data arrives via `SCMFD(ctrlTag, msgTag, dataSize, base64Data)`. The binary payload has a fixed header followed by the audio data.

### Decoding the ISender Data Format

```javascript
globalThis.SCMFD = (ctrlTag, msgTag, dataSize, msg) => {
  // 1. Decode base64 to bytes
  const binaryStr = atob(msg);
  const bytes = new Uint8Array(binaryStr.length);
  for (let i = 0; i < binaryStr.length; i++) {
    bytes[i] = binaryStr.charCodeAt(i);
  }

  // 2. Parse header: 3 x Int32 (12 bytes)
  const header = new Int32Array(bytes.buffer, 0, 3);
  const controlTag = header[0];
  const nChans = header[1];
  const chanOffset = header[2];

  // 3. Parse audio data: Float32 array after header
  const data = new Float32Array(bytes.buffer, 12);

  // 4. Route by control tag
  if (controlTag === 0) {  // kCtrlTagMeter
    updateMeter(data[0], data[1]);  // left peak, right peak
  }
};
```

### Data Layout by Sender Type

**IPeakSender\<N\>:** `data[0..N-1]` = peak value per channel (0.0 to 1.0+)

**IPeakAvgSender\<N\>:** `data[0..N-1]` = peak, `data[N..2N-1]` = average

**IBufferSender\<N\>:** `data` contains interleaved sample buffers

**ISpectrumSender\<N\>:** `data` contains FFT magnitude bins

## Svelte VU Meter Component

A complete VU meter component receiving ISender peak data:

```svelte
<script lang="ts">
  export let label: string;
  let level = 0;

  export function setLevel(value: number) {
    if (value === 0) { level = 0; return; }
    const db = 20 * Math.log10(value);
    level = Math.max(0, Math.min(1, (db + 60) / 60));
  }

  function getColor(h: number): string {
    if (h > 0.9) return '#ff4444';
    if (h > 0.75) return '#ffaa00';
    return '#4CAF50';
  }
</script>

<div class="meter-body">
  <div class="meter-fill"
       style="height: {level * 100}%; background: {getColor(level)}" />
</div>

<style>
  .meter-body {
    width: 30px; height: 200px;
    background: #333; border-radius: 4px;
    position: relative; overflow: hidden;
  }
  .meter-fill {
    position: absolute; bottom: 0; left: 0; width: 100%;
    transition: height 100ms linear;
  }
</style>
```

Wire it up in the parent component:

```svelte
<script>
  let vuMeter;
  globalThis.SCMFD = (ctrlTag, msgTag, dataSize, msg) => {
    if (ctrlTag === 0) {
      const bytes = new Uint8Array(atob(msg).split('').map(c => c.charCodeAt(0)));
      const data = new Float32Array(bytes.buffer, 12);
      vuMeter?.setLevel(data[0]);
    }
  };
</script>
<VUMeter bind:this={vuMeter} label="Level" />
```

See `Examples/IPlugSvelteUI/web-ui/src/lib/VUMeter.svelte` for the full implementation.

## Canvas / WebGL Visualization

For custom visualizations using Canvas 2D or WebGL:

```javascript
let latestData = null;

globalThis.SCMFD = (ctrlTag, msgTag, dataSize, msg) => {
  const bytes = new Uint8Array(atob(msg).split('').map(c => c.charCodeAt(0)));
  latestData = new Float32Array(bytes.buffer, 12);
};

const canvas = document.getElementById('scope');
const ctx = canvas.getContext('2d');

function render() {
  requestAnimationFrame(render);
  if (!latestData) return;

  ctx.clearRect(0, 0, canvas.width, canvas.height);
  ctx.beginPath();
  for (let i = 0; i < latestData.length; i++) {
    const x = (i / latestData.length) * canvas.width;
    const y = (1 - latestData[i]) * canvas.height * 0.5 + canvas.height * 0.25;
    i === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
  }
  ctx.stroke();
}
render();
```

## Performance Considerations

- **Base64 overhead:** ~33% size increase. For large buffers (spectrum data), consider reducing FFT size or update rate.
- **`SetMaxJSStringLength`:** Call `SetMaxJSStringLength(32768)` if sending large data payloads that exceed the default 8192 character limit.
- **Throttle rendering:** Use `requestAnimationFrame` rather than rendering on every SCMFD call. Store the latest data and render at display refresh rate.
- **Avoid decoding in tight loops:** Cache decoded `Uint8Array` if the same data is used multiple times per frame.
- **CSS transitions:** For simple meters, CSS `transition` (like the VU meter example) is more efficient than JavaScript animation.
