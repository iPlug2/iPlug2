#include "IPlugCLITest.h"
#include "IPlug_include_in_plug_src.h"

#if IPLUG_EDITOR
#include "IControls.h"
#endif

IPlugCLITest::IPlugCLITest(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kGain)->InitDouble("Gain", 100., 0., 100.0, 0.1, "%");
  GetParam(kFrequency)->InitDouble("Frequency", 440., 20., 20000., 1., "Hz");
  GetParam(kAttack)->InitDouble("Attack", 10., 1., 1000., 1., "ms");
  GetParam(kDecay)->InitDouble("Decay", 200., 1., 5000., 1., "ms");

#if IPLUG_EDITOR
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };

  mLayoutFunc = [&](IGraphics* pGraphics) {
    if (pGraphics->NControls())
      return;

    // Load a system font
    pGraphics->LoadFont("Roboto-Regular", "Roboto", ETextStyle::Normal);

    // Dark background
    pGraphics->AttachPanelBackground(COLOR_BLACK);

    // Mosaic of colored rectangles
    pGraphics->AttachControl(new ILambdaControl(pGraphics->GetBounds(),
      [](ILambdaControl* pCaller, IGraphics& g, IRECT& r) {
        const int cols = 12;
        const int rows = 8;
        const float cellW = r.W() / cols;
        const float cellH = r.H() / rows;

        for (int row = 0; row < rows; row++)
        {
          for (int col = 0; col < cols; col++)
          {
            // Create varied colors based on position
            float hue = (float)(row * cols + col) / (rows * cols);
            float sat = 0.7f + 0.3f * std::sin(col * 0.5f);
            float lum = 0.4f + 0.3f * std::cos(row * 0.7f);

            IColor color = IColor::FromHSLA(hue, sat, lum);

            IRECT cell(r.L + col * cellW, r.T + row * cellH,
                       r.L + (col + 1) * cellW, r.T + (row + 1) * cellH);

            // Add some padding between cells
            cell.Pad(-2.f);

            g.FillRect(color, cell);
          }
        }
      }, kNoParameter, false));
  };
#endif
}

#if IPLUG_DSP
void IPlugCLITest::OnReset()
{
  mPhase = 0.0;
  mEnvValue = 0.0;
  mEnvStage = 0;
  mNote = -1;

  // Calculate envelope rates based on sample rate
  double sr = GetSampleRate();
  double attackMs = GetParam(kAttack)->Value();
  double decayMs = GetParam(kDecay)->Value();
  mAttackRate = 1.0 / (attackMs * sr / 1000.0);
  mDecayRate = 1.0 / (decayMs * sr / 1000.0);
}

void IPlugCLITest::ProcessMidiMsg(const IMidiMsg& msg)
{
  int status = msg.StatusMsg();

  switch (status)
  {
    case IMidiMsg::kNoteOn:
    {
      int note = msg.NoteNumber();
      int velocity = msg.Velocity();
      if (velocity > 0)
      {
        mNote = note;
        mVelocity = velocity / 127.0;
        mFreq = 440.0 * std::pow(2.0, (note - 69) / 12.0);
        mEnvStage = 1; // Attack
      }
      else
      {
        if (note == mNote)
          mEnvStage = 2; // Decay
      }
      break;
    }
    case IMidiMsg::kNoteOff:
    {
      int note = msg.NoteNumber();
      if (note == mNote)
        mEnvStage = 2; // Decay
      break;
    }
    default:
      break;
  }
}

void IPlugCLITest::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kGain)->Value() / 100.0;
  const double freq = GetParam(kFrequency)->Value();
  const double sr = GetSampleRate();
  const int nChans = NOutChansConnected();

  // Update envelope rates in case parameters changed
  double attackMs = GetParam(kAttack)->Value();
  double decayMs = GetParam(kDecay)->Value();
  mAttackRate = 1.0 / (attackMs * sr / 1000.0);
  mDecayRate = 1.0 / (decayMs * sr / 1000.0);

  for (int s = 0; s < nFrames; s++)
  {
    // Update envelope
    if (mEnvStage == 1) // Attack
    {
      mEnvValue += mAttackRate;
      if (mEnvValue >= 1.0)
      {
        mEnvValue = 1.0;
        mEnvStage = 0; // Hold until note off
      }
    }
    else if (mEnvStage == 2) // Decay
    {
      mEnvValue -= mDecayRate;
      if (mEnvValue <= 0.0)
      {
        mEnvValue = 0.0;
        mEnvStage = 0;
        mNote = -1;
      }
    }

    // Generate sine wave with envelope
    double useFreq = (mNote >= 0) ? mFreq : freq;
    double osc = std::sin(mPhase * 2.0 * 3.14159265358979323846);
    mPhase += useFreq / sr;
    if (mPhase >= 1.0)
      mPhase -= 1.0;

    // Apply envelope and velocity for synth mode, or just gain for effect mode
    double sample;
    if (mNote >= 0 || mEnvValue > 0.0)
    {
      // Synth mode: generate tone with envelope
      sample = osc * mEnvValue * mVelocity * gain;
    }
    else if (inputs != nullptr && inputs[0] != nullptr)
    {
      // Effect mode: pass through input with gain
      sample = inputs[0][s] * gain;
    }
    else
    {
      // No input, no notes: output oscillator at set frequency
      sample = osc * gain;
    }

    for (int c = 0; c < nChans; c++)
    {
      outputs[c][s] = sample;
    }
  }
}
#endif
