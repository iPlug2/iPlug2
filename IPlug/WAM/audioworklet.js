// AudioWorklet polyfill
// Jari Kleimola 2017 (jari@webaudiomodules.org)
//
// based on https://github.com/GoogleChromeLabs/houdini-samples/blob/master/animation-worklet/anim-worklet.js
// feature detection borrowed from Google's AudioWorklet demo page
//
AudioContext = window.AudioContext || window.webkitAudioContext;

(function(scope) {
  "use strict";
  
  // namespace to avoid polluting global scope
  window.AWPF = window.AWPF || {}
  
  // --------------------------------------------------------------------------
  //
  //
  AWPF.PolyfillAudioWorklet = function() {
    var imports = {};
   
    function importOnWorker(src) {
      if (!AWPF.worker.onmessage) AWPF.worker.onmessage = onmessage;
      return new Promise(function(resolve, reject) {
        imports[src] = { resolve:resolve, reject:reject };
        AWPF.worker.postMessage({ type:"import", url:src });
      });
    }
    
    var onmessage = function (e) {
      var msg = e.data;
      switch (msg.type) {
        case "load":
          var script = imports[msg.url];
          if (script) {
            if (!msg.error) script.resolve();
            else script.reject(Error('Failed to load ' + msg.url));
            delete imports[msg.url];
          }
          else throw new Error("InvalidStateError");
          break;
        case "register":
          AWPF.descriptorMap[msg.name] = msg.descriptor;
          break;
        case "state":
          var node = AWPF.workletNodes[msg.node];
          if (node) {
            if (msg.state == "running")
              node.processor = msg.processor;
            var event = new CustomEvent('statechange', { detail: msg.state });
            node.onprocessorstatechange(event);
          }
          break;
      }
    }
    
    return { addModule:importOnWorker }
  }
  
  // --------------------------------------------------------------------------
  //
  //
  AWPF.AudioWorkletNode = function (context, nodeName, options) {
    
    if (AWPF.descriptorMap[nodeName] === undefined)
      throw new Error("NotSupportedException");
    // TODO step 9

    this.id = AWPF.workletNodes.length;    
    AWPF.workletNodes.push(this);

    var messageChannel = new MessageChannel();
    this.port = messageChannel.port1;

    // -- SPN min bufsize is 256, and it has max one input and max one output
    options = options || {}
    if (!options.samplesPerBuffer) options.samplesPerBuffer = 512;
    if (options.numberOfInputs === undefined)       options.numberOfInputs = 0;
    if (options.numberOfOutputs === undefined)      options.numberOfOutputs = 1;
    if (options.inputChannelCount === undefined)    options.inputChannelCount  = [];
    if (options.outputChannelCount === undefined)   options.outputChannelCount = [1];
    if (options.inputChannelCount.length  != options.numberOfInputs)  throw new Error("InvalidArgumentException");
    if (options.outputChannelCount.length != options.numberOfOutputs) throw new Error("InvalidArgumentException");
    
    // -- io configuration is currently static
    this.inputBus  = [];
    this.outputBus = [];
    
    function configureBus (type, options) {
      var numPorts = (type == "input") ? options.numberOfInputs : options.numberOfOutputs;
      var channelCount = (type == "input") ? options.inputChannelCount : options.outputChannelCount;
      if (numPorts > 0) {
        if (numPorts > 1) numPorts = 1; // SPN restriction
        var bus = new Array(numPorts);
        for (var i=0; i<numPorts; i++) {
          var nchannels = channelCount[i];
          if (nchannels <= 0) throw new Error("InvalidArgumentException");        
          var port = new Array(nchannels);
          for (var c=0; c<nchannels; c++)
            port[c] = new SharedArrayBuffer(options.samplesPerBuffer * 4);
          bus[i] = port;
        }
        return bus;
      }
      return [];
    }
    
    this.inputBus  = configureBus("input",  options);
    this.outputBus = configureBus("output", options);

    this.processorState = "pending";
    var args = { node:this.id, name:nodeName, options:options }
    args.bus = { input:this.inputBus, output:this.outputBus }
    AWPF.worker.postMessage({ type:"createProcessor", args:args }, [messageChannel.port2])

    var spn = context.createScriptProcessor(options.samplesPerBuffer, 0,1);
    this.input = spn;
    var outbuf = new Float32Array(this.outputBus[0][0]);  // spn limitation
    
    this.connect = function (dst) {
      spn.onaudioprocess = onprocess.bind(this);
      spn.connect(dst)
    }
    
    this.disconnect = function () {
      spn.onaudioprocess = null;
      spn.disconnect();
    }

    this.onprocessorstatechange = function (e) {
      this.processorState = e.detail;
      console.log("state:", e.detail);              
    }
    
    var onprocess = function (ape) {
      if (this.processor === undefined) return;
      
      var ibuff = ape.inputBuffer;
      var obuff = ape.outputBuffer;
      var outL  = obuff.getChannelData(0);
      outL.set(outbuf);
      
      var msg = { type:"process", processor:this.processor, time:context.currentTime };
      AWPF.worker.postMessage(msg);
    }
  }
  
  // --------------------------------------------------------------------------
  
  // -- borrowed from Google's AudioWorklet demo page
  AWPF.AudioWorkletAvailable = function (actx) {
    return actx.audioWorklet &&
      actx.audioWorklet instanceof AudioWorklet &&
      typeof actx.audioWorklet.addModule === 'function' &&
      window.AudioWorkletNode;
  }
  
  if (!AWPF.AudioWorkletAvailable(scope)) {
    AWPF.descriptorMap = {};   // node name to parameter descriptor map (should be in BAC)
    AWPF.workletNodes  = [];
    AWPF.audioWorklet = AWPF.PolyfillAudioWorklet();
    AWPF.context = scope;
    scope.audioWorklet = AWPF.audioWorklet;
    window.AudioWorkletNode = AWPF.AudioWorkletNode;
    
    AWPF.worker = new Worker("audioworker.js");
    AWPF.worker.postMessage({ type:"init", sampleRate:scope.sampleRate });
    
    console.warn('Using Worker polyfill of AudioWorklet, audio will not be performance isolated.');
    AWPF.isAudioWorkletPolyfilled = true;
  } 
})(new AudioContext())
