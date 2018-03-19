// AudioWorklet polyfill
// Jari Kleimola 2017-18 (jari@webaudiomodules.org)
//
var AWGS = { processors:[] }

// --------------------------------------------------------------------------
//
//
AWGS.AudioWorkletGlobalScope = function () {
  var ctors = {}; // node name to processor definition map

  function registerOnWorker(name, ctor) {
    if (!ctors[name]) {
      ctors[name] = ctor;
      postMessage({ type:"register", name:name, descriptor:ctor.parameterDescriptors });
    }
    else {
      postMessage({ type:"state", node:nodeID, state:"error" });
      throw new Error("AlreadyRegistered");      
    }
  };

  function constructOnWorker (name, port, options) {
    if (ctors[name]) {
      options = options || {}
      options._port = port;
      var processor = new ctors[name](options);
      if (!(processor instanceof AudioWorkletProcessor)) {
        postMessage({ type:"state", node:nodeID, state:"error" });
        throw new Error("InvalidStateError");
      }
      return processor;
    }
    else {
      postMessage({ type:"state", node:nodeID, state:"error" });
      throw new Error("NotSupportedException");       
    }
  }

  class AudioWorkletProcessorPolyfill {
    constructor (options) { this.port = options._port; }
    process (inputs, outputs, params) {}
  }
  
  return {
    'AudioWorkletProcessor': AudioWorkletProcessorPolyfill,
    'registerProcessor': registerOnWorker,
    '_createProcessor':  constructOnWorker
  }
}


AudioWorkletGlobalScope = AWGS.AudioWorkletGlobalScope();
AudioWorkletProcessor   = AudioWorkletGlobalScope.AudioWorkletProcessor;
registerProcessor = AudioWorkletGlobalScope.registerProcessor;
sampleRate = 44100;
hasSAB = true;

onmessage = function (e) {
  var msg = e.data;
  switch (msg.type) {

    case "init":
      sampleRate = AudioWorkletGlobalScope.sampleRate = msg.sampleRate;
      break;
      
    case "import":
      importScripts(msg.url);
      postMessage({ type:"load", url:msg.url });
      break;
      
    case "createProcessor":
      // -- slice io to match with SPN bufferlength
      var a = msg.args;
      var slices = [];
      var buflen = 128;
      var numSlices = (a.options.samplesPerBuffer/buflen)|0;
      
      hasSAB = (a.bufferCount === undefined);
      if (hasSAB) {
        for (var i=0; i<numSlices; i++) {
          var sliceStart = i * buflen;
          var sliceEnd   = sliceStart + buflen;

          // -- create io buses
          function createBus (buffers) {
            var ports = [];
            for (var iport=0; iport<buffers.length; iport++) {          
              var port = [];
              for (var channel=0; channel<buffers[iport].length; channel++) {
                var buf = new Float32Array(buffers[iport][channel]);
                port.push(buf.subarray(sliceStart, sliceEnd));
              }
              ports.push(port);
            }
            return ports;
          }
          var inbus  = createBus(a.bus.input);
          var outbus = createBus(a.bus.output);

          slices.push({ inbus:inbus, outbus:outbus });
        }
      }

      // -- create processor
      var processor = AudioWorkletGlobalScope._createProcessor(a.name, e.ports[0], a.options);
      processor.node = a.node;
      processor.id = AWGS.processors.length;
      processor.numSlices = numSlices;
      AWGS.processors.push({ awp:processor, slices:slices });
      postMessage({ type:"state", node:a.node, processor:processor.id, state:"running" });
      break;
      
    case "process":
      var processor = AWGS.processors[msg.processor];
      if (processor) {
        if (hasSAB) {
          for (var i=0; i<processor.slices.length; i++) {
            var slice = processor.slices[i];
            processor.awp.process(slice.inbus, slice.outbus, []);
          }
        }
        else if (msg.buf.byteLength) {
          var buf = new Float32Array(msg.buf);
          var start = 0;
          for (var i=0; i<processor.awp.numSlices; i++) {
            var arr = buf.subarray(start, start + 128);
            processor.awp.process([], [[arr]], []);
            start += 128;
          }
          postMessage({ buf:msg.buf, type:"process", node:processor.awp.node }, [msg.buf]);         
        }
      }
      break;
  }
}
