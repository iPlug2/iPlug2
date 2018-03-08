class IPLUGWAM extends WAMController
{
  constructor (actx, options) {
    options = options || {};
    options.numberOfInputs  = 2;
    options.numberOfOutputs = 2;
    options.outputChannelCount = [2];

    super(actx, "IPLUGWAM", options);
  }

  static importScripts (actx) {
    return new Promise( (resolve) => {
      actx.audioWorklet.addModule("IPlugWAM.wasm.js").then(() => {
      actx.audioWorklet.addModule("loader.js").then(() => {
      actx.audioWorklet.addModule("wam-processor.js").then(() => {
      actx.audioWorklet.addModule("IPlugWAM-awp.js").then(() => {
        setTimeout( function () { resolve(); }, 500);
      }) }) }) });
    })
  }
}
