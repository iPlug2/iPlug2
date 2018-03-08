// WAM AudioWorkletController
// Jari Kleimola 2017-18 (jari@webaudiomodules.org)
// work in progress

class WAMController extends AudioWorkletNode
{
  constructor(context, processorName, options) {
    super(context, processorName, options);
  }
  
  setParam(key,value) {
    this.port.postMessage({ type:"param", key:key, value:value });    
  }

  setPatch(patch) {
    this.port.postMessage({ type:"patch", data:patch });    
  }

  setSysex(sysex) {
    this.port.postMessage({ type:"sysex", data:sysex });    
  }

  onMidi(msg) {
    this.port.postMessage({ type:"midi", data:msg });
  }
  
  set midiIn (port) {
    if (this._midiInPort) {
      this._midiInPort.close();
      this._midiInPort.onmidimessage = null;
    }
    this._midiInPort = port;
    this._midiInPort.onmidimessage = function (msg) {
      this.port.postMessage({ type:"midi", data:msg.data });
    }.bind(this);
  }
  
  sendMessage(verb, prop, data) {
    this.port.postMessage({ type:"msg", verb:verb, prop:prop, data:data });
  }
  
  get gui () { return null; }
}
