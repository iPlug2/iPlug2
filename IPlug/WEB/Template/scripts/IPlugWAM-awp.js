/* Declares the NAME_PLACEHOLDER Audio Worklet Processor */

class NAME_PLACEHOLDER_AWP extends AudioWorkletGlobalScope.WAMProcessor
{
  constructor(options) {
    options = options || {}
    options.mod = AudioWorkletGlobalScope.WAM.NAME_PLACEHOLDER;
    super(options);
  }
}

registerProcessor("NAME_PLACEHOLDER", NAME_PLACEHOLDER_AWP);
