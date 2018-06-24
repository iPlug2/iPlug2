class IPlugWAM_AWP extends AudioWorkletGlobalScope.WAMProcessor
{
  constructor(options) {
    options = options || {}
    options.mod = AudioWorkletGlobalScope.WAM.IPlug;
    super(options);
  }
}

registerProcessor("IPlugWAM", IPlugWAM_AWP);
