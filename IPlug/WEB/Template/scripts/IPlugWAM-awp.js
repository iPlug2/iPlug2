class IPlugWAM_AWP extends AudioWorkletGlobalScope.WAMProcessor
{
  constructor(options) {
    options = options || {}
    options.mod = AudioWorkletGlobalScope.WAM.IPlug;
    super(options);
    this.numOutChannels = [1];
  }
}

registerProcessor("IPlugWAM", IPlugWAM_AWP);
