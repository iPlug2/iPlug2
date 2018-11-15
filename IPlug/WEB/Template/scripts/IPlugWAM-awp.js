class NAME_PLACEHOLDER_AWP extends AudioWorkletGlobalScope.WAMProcessor
{
  constructor(options) {
    options = options || {}
    options.mod = AudioWorkletGlobalScope.WAM.IPlug;
    super(options);
  }
}

registerProcessor("NAME_PLACEHOLDER", NAME_PLACEHOLDER_AWP);
