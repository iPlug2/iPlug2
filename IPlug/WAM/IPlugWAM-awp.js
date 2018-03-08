class IPLUGWAM_AWP extends AudioWorkletGlobalScope.WAMProcessor
{
  constructor(options) { super(options); }
}

registerProcessor("IPlugWAM", IPLUGWAM_AWP);
