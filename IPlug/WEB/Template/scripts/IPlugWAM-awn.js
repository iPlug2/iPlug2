/* Declares the NAME_PLACEHOLDER Audio Worklet Node */

class NAME_PLACEHOLDERController extends WAMController
{
  constructor (actx, options) {
    options = options || {};
    if (options.numberOfInputs === undefined)       options.numberOfInputs = 0;
    if (options.numberOfOutputs === undefined)      options.numberOfOutputs = 1;
    if (options.outputChannelCount === undefined)   options.outputChannelCount = [2];
    if (options.processorOptions.inputChannelCount === undefined) options.processorOptions = {inputChannelCount:[]};

    if( navigator.userAgent.toLowerCase().indexOf('firefox') > -1 ){
      console.log("Firefox detected: SPN buffersize = 512")
      options.buflenSPN = 512
    }

    if( navigator.userAgent.toLowerCase().indexOf('firefox') > -1 ){
      console.log("Firefox detected: SPN buffersize = 512")
      options.buflenSPN = 512
    }

    super(actx, "NAME_PLACEHOLDER", options);
  }

  static importScripts (actx) {
    var origin = "ORIGIN_PLACEHOLDER";

    return new Promise( (resolve) => {
      actx.audioWorklet.addModule(origin + "scripts/NAME_PLACEHOLDER-wam.js").then(() => {
      actx.audioWorklet.addModule(origin + "scripts/wam-processor.js").then(() => {
      actx.audioWorklet.addModule(origin + "scripts/NAME_PLACEHOLDER-awp.js").then(() => {
        resolve();
      }) }) });
    })
  }

  onmessage(msg) {
    //Received the WAM descriptor from the processor - could create an HTML UI here, based on descriptor
    if(msg.type == "descriptor") {
      console.log("got WAM descriptor...");
    }

    //Send Parameter Value From Delegate
    if(msg.verb == "SPVFD") {
      Module.SPVFD(parseInt(msg.prop), parseFloat(msg.data));
    }
    //Set Control Value From Delegate
    else if(msg.verb == "SCVFD") {
      Module.SCVFD(parseInt(msg.prop), parseFloat(msg.data));
    }
    //Send Control Message From Delegate
    else if(msg.verb == "SCMFD") {
      var res = msg.prop.split(":");
      var data = new Uint8Array(msg.data);
      const buffer = Module._malloc(data.length);
      Module.HEAPU8.set(data, buffer);
      Module.SCMFD(parseInt(res[0]), parseInt(res[1]), data.length, buffer);
      Module._free(buffer);
    }
    //Send Arbitrary Message From Delegate
    else if(msg.verb == "SAMFD") {
      var data = new Uint8Array(msg.data);
      const buffer = Module._malloc(data.length);
      Module.HEAPU8.set(data, buffer);
      Module.SAMFD(parseInt(msg.prop), data.length, buffer);
      Module._free(buffer);
    }
    //Send MIDI Message From Delegate
    else if(msg.verb == "SMMFD") {
      var res = msg.prop.split(":");
      Module.SMMFD(parseInt(res[0]), parseInt(res[1]), parseInt(res[2]));
    }
    //Send Sysex Message From Delegate
    else if(msg.verb == "SSMFD") {
      var data = new Uint8Array(msg.data);
      const buffer = Module._malloc(data.length);
      Module.HEAPU8.set(data, buffer);
      Module.SSMFD(parseInt(msg.prop), data.length, buffer);
      Module._free(buffer);
    }
  }
}
