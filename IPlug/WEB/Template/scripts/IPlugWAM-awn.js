class IPlugWAMController extends WAMController
{
  constructor (actx, options) {
    options = options || {};
    options.numberOfInputs  = 0;
    options.numberOfOutputs = 1;
    options.outputChannelCount = [1];

    super(actx, "IPlugWAM", options);
  }

  static importScripts (actx) {
    var origin = location.origin + "/";
    return new Promise( (resolve) => {
      actx.audioWorklet.addModule(origin + "scripts/IPlugWAM-WAM.wasm.js").then(() => {
      actx.audioWorklet.addModule(origin + "scripts/IPlugWAM-WAM.js").then(() => {
      actx.audioWorklet.addModule(origin + "scripts/wam-processor.js").then(() => {
      actx.audioWorklet.addModule(origin + "scripts/IPlugWAM-awp.js").then(() => {
        resolve();
      }) }) }) });
    })
  }

  onmessage(msg) {
    if(msg.type == "descriptor") {
      console.log("got WAM descriptor...");
    }
    if(msg.verb == "SCMFD") {
      var res = msg.prop.split(":");
      Module.SCMFD(parseInt(res[0]), parseInt(res[1]), 0);
    }
    else if(msg.verb == "SCVFD") {
      Module.SCVFD(parseInt(msg.prop), parseFloat(msg.data));
    }
    else if(msg.verb == "SMMFD") {
      var res = msg.prop.split(":");
      Module.SMMFD(parseInt(res[0]), parseInt(res[1]), parseInt(res[2]));
    }
  }
}
