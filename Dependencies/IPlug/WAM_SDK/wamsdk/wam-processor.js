// WAM AudioWorkletProcessor
// Jari Kleimola 2017-18 (jari@webaudiomodules.org)
//
// work in progress
// helper class for WASM data marshalling and C function call binding
// also provides midi, patch data, parameter and arbitrary message support

if (!AudioWorkletGlobalScope.WAMProcessor) {

class WAMProcessor extends AudioWorkletProcessor
{
  static get parameterDescriptors() {
    return [];
  }
  
  constructor(options) {
    options = options || {}
    if (options.numberOfInputs === undefined)       options.numberOfInputs = 0;
    if (options.numberOfOutputs === undefined)      options.numberOfOutputs = 1;
    if (options.inputChannelCount === undefined)    options.inputChannelCount  = [];
    if (options.outputChannelCount === undefined)   options.outputChannelCount = [1];
    if (options.inputChannelCount.length  != options.numberOfInputs)  throw new Error("InvalidArgumentException");
    if (options.outputChannelCount.length != options.numberOfOutputs) throw new Error("InvalidArgumentException");

    super(options);
    this.bufsize = 128;
    this.sr = AudioWorkletGlobalScope.sampleRate || sampleRate;    
    this.audiobufs = [[],[]];
    
    var WAM = options.mod;
    this.WAM = WAM;
    
    // -- construction C function wrappers
    var wam_ctor = WAM.cwrap("createModule", 'number', []);
    var wam_init = WAM.cwrap("wam_init", null, ['number','number','number','string']);

    // -- runtime C function wrappers
    this.wam_terminate = WAM.cwrap("wam_terminate", null, ['number']);
    this.wam_onmidi = WAM.cwrap("wam_onmidi", null, ['number','number','number','number']);
    this.wam_onpatch = WAM.cwrap("wam_onpatch", null, ['number','number','number']);
    this.wam_onprocess = WAM.cwrap("wam_onprocess", 'number', ['number','number','number']);
    this.wam_onparam = WAM.cwrap("wam_onparam", null, ['number','number','number']);
    this.wam_onsysex = WAM.cwrap("wam_onsysex", null, ['number','number','number']);    
    this.wam_onmessageN = WAM.cwrap("wam_onmessageN", null, ['number','string','string','number']);
    this.wam_onmessageS = WAM.cwrap("wam_onmessageS", null, ['number','string','string','string']);
    
    // -- supress warnings for older WAMs
    if (WAM["_wam_onmessageA"])
      this.wam_onmessageA = WAM.cwrap("wam_onmessageA", null, ['number','string','string','number','number']);
    
    this.inst = wam_ctor();
    var desc  = wam_init(this.inst, this.bufsize, this.sr, "");

    // -- audio io configuration
    this.numInputs  = options.numberOfInputs;
    this.numOutputs = options.numberOfOutputs;
    this.numInChannels  = options.inputChannelCount;
    this.numOutChannels = options.outputChannelCount;
    
    var ibufs = this.numInputs  > 0 ? WAM._malloc(this.numInputs)  : 0;
    var obufs = this.numOutputs > 0 ? WAM._malloc(this.numOutputs) : 0;
    this.audiobus = WAM._malloc(2*4);
    WAM.setValue(this.audiobus,   ibufs, 'i32');
    WAM.setValue(this.audiobus+4, obufs, 'i32');

    for (var i=0; i<this.numInputs; i++) {
      var buf = WAM._malloc( this.bufsize*4 );
      WAM.setValue(ibufs + i*4, buf, 'i32');
      this.audiobufs[0].push(buf/4);
    }
    for (var i=0; i<this.numOutputs; i++) {
      var numChannels = 2; // this.numOutChannels[i];
      for (var c=0; c<numChannels; c++) {
        var buf = WAM._malloc( this.bufsize*4 );
        WAM.setValue(obufs + (i*numChannels+c)*4, buf, 'i32');
        this.audiobufs[1].push(buf/4);
      }
    }

    this.port.onmessage = this.onmessage.bind(this);
    this.port.start();    
  }
  
  onmessage (e) {
    var msg  = e.data;
    var data = msg.data;
    switch (msg.type) {
      case "midi":  this.onmidi(data[0], data[1], data[2]); break;
      case "sysex": this.onsysex(data); break;
      case "patch": this.onpatch(data); break;
      case "param": this.onparam(msg.key, msg.value); break;
      case "msg":   this.onmsg(msg.verb, msg.prop, msg.data); break;
    //case "osc":   this.onmsg(msg.prop, msg.type, msg.data); break;
    }
  }
  
  onmidi (status, data1, data2) {
    this.wam_onmidi(this.inst, status, data1, data2);
  }
  
  onparam (key, value) {
   if (typeof key === "string")
      this.wam_onmessageN(this.inst, "set", key, value);
    else this.wam_onparam(this.inst, key, value);
  }
  
  onmsg (verb, prop, data) {
    if (data instanceof ArrayBuffer) {
      var buffer = new Uint8Array(data);
      var len = data.byteLength;
      var WAM = this.WAM;
      var buf = WAM._malloc(data.byteLength);
      for (var i=0; i<len; i++)
        WAM.setValue(buf+i, buffer[i], 'i8');
      this.wam_onmessageA(this.inst, verb, prop, buf, len);
      WAM._free(buf);
    }
    else if (typeof data === "string")
      this.wam_onmessageS(this.inst, verb, prop, data);
    else this.wam_onmessageN(this.inst, verb, prop, data);
  }
  
  onpatch (data) {
    var buffer = new Uint8Array(data);
    var len = data.byteLength;
    var WAM = this.WAM;
    var buf = WAM._malloc(len);
    for (var i=0; i<len; i++)
      WAM.setValue(buf+i, buffer[i], 'i8');
    this.wam_onpatch(this.inst, buf, len);
    WAM._free(buf);
  }

	onsysex (data) {
    var WAM = this.WAM;
    var buf = WAM._malloc(data.length);
    for (var i = 0; i < data.length; i++)
      WAM.setValue(buf+i, data[i], 'i8');
		this.wam_onsysex(this.inst, buf, data.length);
    WAM._free(buf);
	}

  process (inputs,outputs,params) {
    var WAM = this.WAM;
    
    // -- inputs
    for (var i=0; i<this.numInputs; i++) {
      var waain = inputs[i][0];
      var wamin = this.audiobufs[0][i];
      WAM.HEAPF32.set(waain, wamin);
    }
    
    this.wam_onprocess(this.inst, this.audiobus, 0);
    
    // -- outputs
    for (var i=0; i<this.numOutputs; i++) {
      var numChannels = this.numOutChannels[i];
      for (var c=0; c<numChannels; c++) {
        var waaout = outputs[i][c];
        var wamout = this.audiobufs[1][i*numChannels+c];
        waaout.set(WAM.HEAPF32.subarray(wamout, wamout + this.bufsize));
      }
    }
    
    return true;    
  }
}

// -- hack to share data between addModule() calls
AudioWorkletGlobalScope.WAMProcessor = WAMProcessor;
}
