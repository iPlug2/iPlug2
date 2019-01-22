#pragma once

#include "MidiSynth.h"
#include "Oscillator.h"
#include "ADSREnvelope.h"

class IPlugInstrumentDSP
{
private:
#pragma mark - Voice
  class IPlugInstrumentVoice : public SynthVoice
  {
  public:
    IPlugInstrumentVoice()
    : mOsc1(0.75)
    , mADSR1("gain")
    {
      DBGMSG("new Voice: %i control inputs.\n", static_cast<int>(mInputs.size()));
    }

    ~IPlugInstrumentVoice()
    {
    }

    bool GetBusy() const override
    {
      return mADSR1.GetBusy();;
    }

    void Trigger(double level, bool isRetrigger) override
    {
      mOsc1.Reset();
      
      if(isRetrigger)
        mADSR1.Retrigger(level);
      else
        mADSR1.Start(level);
    }
    
    void Release() override
    {
      mADSR1.Release();
    }

    void ProcessSamplesAccumulating(sample** inputs, sample** outputs, int nInputs, int nOutputs, int startIdx, int nFrames) override
    {
      // inputs to the synthesizer can just fetch a value every block, like this:
      double gate = mInputs[kVoiceControlGate].endValue;
      double pitch = mInputs[kVoiceControlPitch].endValue;
      double pitchBend = mInputs[kVoiceControlPitchBend].endValue;

      // or write the entire control ramp to a buffer, like this, to get sample-accurate ramps:
      mInputs[kVoiceControlTimbre].Write(mTimbreBuffer, startIdx, nFrames);

      // convert from "1v/oct" pitch space to frequency in Hertz
      double osc1Freq = 440. * pow(2., pitch + pitchBend);

      // add a second oscillator, up a fifth
      double osc2Freq = osc1Freq * 3. / 2.;

      // make sound output for each output channel
      for(auto i = startIdx; i < startIdx + nFrames; i++)
      {
        float noise = mTimbreBuffer[i] * Rand();

        // an MPE synth can use pressure here in addition to gain
        outputs[0][i] += (mOsc1.Process(osc1Freq) + mOsc2.Process(osc2Freq) * mOsc2Gain + noise) * mADSR1.Process(1.) * mGain;
        outputs[1][i] = outputs[0][i];
      }
    }

    void SetSampleRate(double sampleRate) override
    {
      mOsc1.SetSampleRate(sampleRate);
      mOsc2.SetSampleRate(sampleRate);
    }

    void SetProgramNumber(int pgm) override
    {
      // just set osc2 gain based on the program number.
      // in a real synth this is where we would load a program from disk.
      switch(pgm)
      {
        case 0:
        default:
          mOsc2Gain = 0.;
          break;
        case 1:
          mOsc2Gain = 1.;
          break;
      }
    }

    // this is called by the VoiceAllocator to set generic control values.
    void SetControl(int controlNumber, float value) override
    {
      DBGMSG("setting control %i to value %f\n", controlNumber, value);
    }

  private:
    FastSinOscillator<sample> mOsc1;
    FastSinOscillator<sample> mOsc2;
    ADSREnvelope<sample> mADSR1;

    float mOsc2Gain = 0.;

    // would be allocated dynamically in a real example
    static constexpr int kMaxBlockSize = 1024;
    float mTimbreBuffer[kMaxBlockSize];

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
      mSynth.AddVoice(new IPlugInstrumentVoice(), 0);
    }

    // some MidiSynth API examples:
    // mSynth.SetKeyToPitchFn([](int k){return (k - 69.)/24.;}); // quarter-tone scale
    // mSynth.SetNoteGlideTime(0.5); // portamento
  }

  void ProcessBlock(sample** inputs, sample** outputs, int nOutputs, int nFrames)
  {
    // clear outputs
    for(auto i = 0; i < nOutputs; i++)
    {
      memset(outputs[i], 0, nFrames * sizeof(sample));
    }

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

public:
  MidiSynth mSynth { VoiceAllocator::kPolyModePoly, MidiSynth::kDefaultBlockSize };
};
