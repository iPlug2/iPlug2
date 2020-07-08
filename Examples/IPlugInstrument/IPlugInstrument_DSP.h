#pragma once

#include "MidiSynth.h"
#include "Oscillator.h"
#include "ADSREnvelope.h"
#include "Smoothers.h"
#include "LFO.h"

using namespace iplug;

enum EModulations
{
  kModGainSmoother = 0,
  kModSustainSmoother,
  kModLFO,
  kNumModulations,
};

template<typename T>
class IPlugInstrumentDSP
{
public:
#pragma mark - Voice
  class Voice : public SynthVoice
  {
  public:
    Voice()
    : mAMPEnv("gain", [&](){ mOSC.Reset(); }) // capture ok on RT thread?
    {
//      DBGMSG("new Voice: %i control inputs.\n", static_cast<int>(mInputs.size()));
    }

    bool GetBusy() const override
    {
      return mAMPEnv.GetBusy();
    }

    void Trigger(double level, bool isRetrigger) override
    {
      mOSC.Reset();
      
      if(isRetrigger)
        mAMPEnv.Retrigger(level);
      else
        mAMPEnv.Start(level);
    }
    
    void Release() override
    {
      mAMPEnv.Release();
    }

    void ProcessSamplesAccumulating(T** inputs, T** outputs, int nInputs, int nOutputs, int startIdx, int nFrames) override
    {
      // inputs to the synthesizer can just fetch a value every block, like this:
//      double gate = mInputs[kVoiceControlGate].endValue;
      double pitch = mInputs[kVoiceControlPitch].endValue;
      double pitchBend = mInputs[kVoiceControlPitchBend].endValue;

      // or write the entire control ramp to a buffer, like this, to get sample-accurate ramps:
      mInputs[kVoiceControlTimbre].Write(mTimbreBuffer.Get(), startIdx, nFrames);
      
      // convert from "1v/oct" pitch space to frequency in Hertz
      double osc1Freq = 440. * pow(2., pitch + pitchBend + inputs[kModLFO][0]);
      
      // make sound output for each output channel
      for(auto i = startIdx; i < startIdx + nFrames; i++)
      {
        float noise = mTimbreBuffer.Get()[i] * Rand();
        // an MPE synth can use pressure here in addition to gain
        outputs[0][i] += (mOSC.Process(osc1Freq) + noise) * mAMPEnv.Process(inputs[kModSustainSmoother][i]) * mGain;
        outputs[1][i] = outputs[0][i];
      }
    }

    void SetSampleRateAndBlockSize(double sampleRate, int blockSize) override
    {
      mOSC.SetSampleRate(sampleRate);
      mAMPEnv.SetSampleRate(sampleRate);
      
      mTimbreBuffer.Resize(blockSize);
    }

    void SetProgramNumber(int pgm) override
    {
      //TODO:
    }

    // this is called by the VoiceAllocator to set generic control values.
    void SetControl(int controlNumber, float value) override
    {
      //TODO:
    }

  public:
    FastSinOscillator<T> mOSC;
    ADSREnvelope<T> mAMPEnv;

  private:
    WDL_TypedBuf<float> mTimbreBuffer;

    // noise generator for test
    uint32_t mRandSeed = 0;
    
    // return single-precision floating point number on [-1, 1]
    float Rand()
    {
      mRandSeed = mRandSeed * 0x0019660D + 0x3C6EF35F;
      uint32_t temp = ((mRandSeed >> 9) & 0x007FFFFF) | 0x3F800000;
      return (*reinterpret_cast<float*>(&temp))*2.f - 3.f;
    }

  };

public:
#pragma mark -
  IPlugInstrumentDSP(int nVoices)
  {
    for (auto i = 0; i < nVoices; i++)
    {
      // add a voice to Zone 0.
      mSynth.AddVoice(new Voice(), 0);
    }

    // some MidiSynth API examples:
    // mSynth.SetKeyToPitchFn([](int k){return (k - 69.)/24.;}); // quarter-tone scale
    // mSynth.SetNoteGlideTime(0.5); // portamento
  }

  void ProcessBlock(T** inputs, T** outputs, int nOutputs, int nFrames, double qnPos = 0., bool transportIsRunning = false, double tempo = 120.)
  {
    // clear outputs
    for(auto i = 0; i < nOutputs; i++)
    {
      memset(outputs[i], 0, nFrames * sizeof(T));
    }
    
    mParamSmoother.ProcessBlock(mParamsToSmooth, mModulations.GetList(), nFrames);
    mLFO.ProcessBlock(mModulations.GetList()[kModLFO], nFrames, qnPos, transportIsRunning, tempo);
    mSynth.ProcessBlock(mModulations.GetList(), outputs, 0, nOutputs, nFrames);
    
    for(int s=0; s < nFrames;s++)
    {
      T smoothedGain = mModulations.GetList()[kModGainSmoother][s];
      outputs[0][s] *= smoothedGain;
      outputs[1][s] *= smoothedGain;
    }
  }

  void Reset(double sampleRate, int blockSize)
  {
    mSynth.SetSampleRateAndBlockSize(sampleRate, blockSize);
    mSynth.Reset();
    mLFO.SetSampleRate(sampleRate);
    mModulationsData.Resize(blockSize * kNumModulations);
    mModulations.Empty();
    
    for(int i = 0; i < kNumModulations; i++)
    {
      mModulations.Add(mModulationsData.Get() + (blockSize * i));
    }
  }

  void ProcessMidiMsg(const IMidiMsg& msg)
  {
    mSynth.AddMidiMsgToQueue(msg);
  }

  void SetParam(int paramIdx, double value)
  {
    using EEnvStage = ADSREnvelope<sample>::EStage;
    
    switch (paramIdx) {
      case kParamNoteGlideTime:
        mSynth.SetNoteGlideTime(value / 1000.);
        break;
      case kParamGain:
        mParamsToSmooth[kModGainSmoother] = (T) value / 100.;
        break;
      case kParamSustain:
        mParamsToSmooth[kModSustainSmoother] = (T) value / 100.;
        break;
      case kParamAttack:
      case kParamDecay:
      case kParamRelease:
      {
        EEnvStage stage = static_cast<EEnvStage>(EEnvStage::kAttack + (paramIdx - kParamAttack));
        mSynth.ForEachVoice([stage, value](SynthVoice& voice) {
          dynamic_cast<IPlugInstrumentDSP::Voice&>(voice).mAMPEnv.SetStageTime(stage, value);
        });
        break;
      }
      case kParamLFODepth:
        mLFO.SetScalar(value / 100.);
        break;
      case kParamLFORateTempo:
        mLFO.SetQNScalarFromDivision(static_cast<int>(value));
        break;
      case kParamLFORateHz:
        mLFO.SetFreqCPS(value);
        break;
      case kParamLFORateMode:
        mLFO.SetRateMode(value > 0.5);
        break;
      case kParamLFOShape:
        mLFO.SetShape(static_cast<int>(value));
        break;
      default:
        break;
    }
  }
  
public:
  MidiSynth mSynth { VoiceAllocator::kPolyModePoly, MidiSynth::kDefaultBlockSize };
  WDL_TypedBuf<T> mModulationsData; // Sample data for global modulations (e.g. smoothed sustain)
  WDL_PtrList<T> mModulations; // Ptrlist for global modulations
  LogParamSmooth<T, kNumModulations> mParamSmoother;
  sample mParamsToSmooth[kNumModulations];
  LFO<T> mLFO;
};
