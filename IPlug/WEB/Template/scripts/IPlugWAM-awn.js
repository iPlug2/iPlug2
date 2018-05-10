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
}
