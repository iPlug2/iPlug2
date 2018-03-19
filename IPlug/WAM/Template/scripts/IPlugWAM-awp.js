class IPlugWAM_AWP extends AudioWorkletGlobalScope.WAMProcessor
{
  constructor(options) { super(options); }
}

registerProcessor("IPlugWAM", IPlugWAM_AWP);
