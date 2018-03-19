# audioworklet-polyfill
Strictly unofficial polyfill for Web Audio API AudioWorklet. The processor runs in a Web Worker, which is connected via SharedArrayBuffer to a main thread ScriptProcessorNode.

_edit:_ SharedArrayBuffers (SABs) are currently disabled in all major browsers due to security concerns. As a workaround, the polyfill falls back to transferable ArrayBuffers that are bounced back and forth between main thread and web worker. This requires double buffering, which increases latency (especially in Firefox it seems). The polyfill still works reasonably well even with this, in all tested user agents. But yeah, I do hope SABs get re-enabled rather sooner than later.

## demos
[https://webaudiomodules.org/wamsynths](https://webaudiomodules.org/wamsynths)

Tested in stable Chrome 64, Firefox 57 and Safari 11. Edge test is still pending.

More info at [webaudiomodules.org](http://www.webaudiomodules.org/blog/audioworklet_polyfill/)

## usage
```
<script src="audioworklet.js"></script>
<!-- audioworker.js should also reside at root -->

var context = new AudioContext();
AWPF.polyfill( context ).then( function () {
  // that's it, then just proceed 'normally'
  // var awn = new MyAudioWorkletNode( context );
  // ...
});
```

`AWPF.polyfill()` resolves immediately if polyfill is not required. Chrome 66 requires AudioContext activation via [user gesture](goo.gl/7K7WLu). This is reflected in the polyfill, which now accepts AudioContext instance as an argument.

## description
**audioworklet.js** polyfills AudioWorkletNode and creates a web worker. Worker is initialized with **audioworker.js** script, which in turn polyfills AudioWorkletGlobalScope and AudioWorkletProcessor. audioWorklet.addModule() is thereby routed to web worker's importScript(), and raw audio processing takes place off main thread. Processed audio is put into a SAB (or transferable ArrayBuffer when SAB is unavailable), which is accessed in main thread ScriptProcessorNode (SPN) onaudioprocess() for audio output.

## caveats
Due to SPN restrictions the number of input and output ports is limited to 1, and the minimum buffer length is 256. I've also cut corners here and there, so the polyfill does not accurately follow the spec in all details. Please raise an issue if you find an offending conflict.

AudioParams are still unsupported.
