#pragma once

#include "MidiSynth.h"
#include "Oscillator.h"

#define SYNTH_BLOCKSIZE 32

class IPlugEffectDSP
{
private:  
#pragma mark - Voice
  class IPlugEffectVoice : public MidiSynth::Voice
  {
  public:
    IPlugEffectVoice()
    : mOsc(0.75)
    {
    }
    
    bool GetBusy() const override { return mGain > 0.; }
    bool GetReleased() const override { return true; }
    
    void Trigger(double level, bool retrigger) override
    {
      mGain = level;
      mOsc.Reset();
    }
    
    void Release() override
    {
      mGain = 0.;
    }
      
    void Kill(bool hard) override
    {
      mGain = 0.;
    }
    
    void ProcessSamples(sample** inputs, sample** outputs, int nInputs, int nOutputs, int startIdx, int nFrames, double pitchBend) override
    {
      const double freqCPS = 440. * pow(2., (mBasePitch - 69.) / 12.);
      
      for(auto s = startIdx; s < startIdx + nFrames; s++)
      {
        outputs[0][s] += mOsc.Process(freqCPS) * mGain;
      }
    }
    
    void SetSampleRate(double sampleRate) override
    {
      mOsc.SetSampleRate(sampleRate);
    }

  private:
    FastSinOscillator<sample> mOsc;
    double mGain = 0.;
  };

public:
#pragma mark -
  IPlugEffectDSP(int nVoices)
  {
    for (auto i = 0; i < nVoices; i++)
    {
      mSynth.AddVoice(new IPlugEffectVoice());
    }
  }
  
  void ProcessBlock(sample** inputs, sample** outputs, int nOutputs, int nFrames)
  {
    for(auto i = 0; i < nOutputs;i++)
      memset(outputs[i], 0, nFrames * sizeof(sample));

    mSynth.ProcessBlock(inputs, outputs, 0, nOutputs, nFrames);
  }
  
  void Reset(double sampleRate, int blockSize)
  {
    mSynth.SetSampleRateAndBlockSize(sampleRate, blockSize);
    mSynth.Reset();
  }

  void ProcessMidiMsg(const IMidiMsg& msg)
  {
    mSynth.AddMidiMsgToQueue(msg);
  }

  MidiSynth mSynth { MidiSynth::kPolyModePoly, SYNTH_BLOCKSIZE };
};
