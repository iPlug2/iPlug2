class IPLUGWAM extends WAMController
{
  constructor (actx, options) {
    options = options || {};
    options.numberOfInputs  = 2;
    options.numberOfOutputs = 1;
    options.outputChannelCount = [2];

    super(actx, "IPLUGWAM", options);
  }

  static importScripts (actx) {
    return new Promise( (resolve) => {
      actx.audioWorklet.addModule("IPlugEffect-WAM.js").then(() => {
      actx.audioWorklet.addModule("loader.js").then(() => {
      actx.audioWorklet.addModule("wam-processor.js").then(() => {
      actx.audioWorklet.addModule("IPlugWAM-awp.js").then(() => {
        setTimeout( function () { resolve(); }, 500);
      }) }) }) });
    })
  }
}
