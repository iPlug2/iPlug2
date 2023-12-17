#include "IPlugSRC.h"
#include "IPlug_include_in_plug_src.h"

IPlugSRC::IPlugSRC(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
, mRealtimeResampler(48000)
{
  GetParam(kGain)->InitGain("Volume");
}

void IPlugSRC::OnReset()
{
  mRealtimeResampler.Reset(GetSampleRate(), GetBlockSize());
  SetLatency(mRealtimeResampler.GetLatency());
}

void IPlugSRC::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kGain)->DBToAmp();
  const int nChans = NOutChansConnected();
  mRealtimeResampler.ProcessBlock(inputs, outputs, nFrames,
  [nChans, gain](sample** inputs, sample** outputs, int nFrames){
    for (int s = 0; s < nFrames; s++) {
      for (int c = 0; c < nChans; c++) {
        outputs[c][s] = inputs[c][s] * gain;
      }
    }
  });
}
