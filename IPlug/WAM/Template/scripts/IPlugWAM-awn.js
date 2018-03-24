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
    return new Promise( (resolve) => {
      actx.audioWorklet.addModule("scripts/IPlugEffect-WAM.wasm.js").then(() => {
      actx.audioWorklet.addModule("scripts/loader.js").then(() => {
      actx.audioWorklet.addModule("scripts/wamsdk/wamsdk/wam-processor.js").then(() => {
      actx.audioWorklet.addModule("scripts/IPlugWAM-awp.js").then(() => {
        setTimeout( function () { resolve(); }, 500);
      }) }) }) });
    })
  }
}
