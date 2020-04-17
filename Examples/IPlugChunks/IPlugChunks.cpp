#include "IPlugChunks.h"
#include "IPlug_include_in_plug_src.h"
#include <math.h>

//text body for About Control
const char* pAboutTextBody[] =
{
  " An example of storing data in chunks, and custom IControl classes.",
  "A step sequenced volume control / trance gate.",
  "Using chunks allows you to store arbitary data (e.g. a hidden,",
  "non-automatable parameter, a filepath etc) in the plugin's state,",
  "i.e. when you save a preset to a file or when you save the project in",
  " your host. You need to override SerializeState / UnserializeState",
  "and set PLUG_DOES_STATE_CHUNKS 1 in config.h"
};

IPlugChunks::IPlugChunks(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPrograms))
, mGain(1.0)
, mUIJustOpened(false)
{
  for (int i = 0; i < NUM_SLIDERS; i++)
  {
    mSteps[i] = 0.5;
  }
  
  GetParam(kGain)->InitDouble("Gain", 0.0, -70.0, 12.0, 0.1, "dB");

#if IPLUG_EDITOR // http://bit.ly/2S64BDd
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, 1.);
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    const IBitmap mfrlogo = pGraphics->LoadBitmap(MFRLOGO_FN, 1);
    const IBitmap vst2logo = pGraphics->LoadBitmap(VST2LOGO_FN, 1);
    const IBitmap vst3logo = pGraphics->LoadBitmap(VST3LOGO_FN, 1);
    const IBitmap aulogo = pGraphics->LoadBitmap(AULOGO_FN, 1);
    const IBitmap aaxlogo = pGraphics->LoadBitmap(AAXLOGO_FN, 1);
    
    pGraphics->EnableMouseOver(true);
    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    pGraphics->LoadFont("Arial", "Arial", ETextStyle::Normal);
    
    IColor textcolor = IColor(255, 0, 255, 255);
    IText textprops = IText(16.f, textcolor, "Arial", EAlign::Near, EVAlign::Middle, 0, COLOR_BLACK, textcolor);
    IText textprops2 = IText(18.f, COLOR_BLACK, "Roboto-Regular", EAlign::Center, EVAlign::Middle);
    
    const IVStyle incstyle {
      false, // Show label note:DEFAULT_SHOW_LABEL is true
      false, // Show value note:DEFAULT_SHOW_VALUE is true
      {
        DEFAULT_BGCOLOR, // Background color note:DEFAULT_BGCOLOR is COLOR_TRANSPARENT
        IColor(255, 210, 0, 0), // Foreground color note:DEFAULT_FGCOLOR is COLOR_MID_GRAY
        IColor(255, 255, 100, 100), // Pressed color note:DEFAULT_PRCOLOR is COLOR_LIGHT_GRAY
        IColor(255, 200, 0, 0), // Frame color note:DEFAULT_FRCOLOR is COLOR_DARK_GRAY
        IColor(255, 230, 0, 0), // Highlight color note:DEFAULT_HLCOLOR is COLOR_TRANSLUCENT
        DEFAULT_SHCOLOR, // Shadow color note:DEFAULT_SHCOLOR is IColor(60, 0, 0, 0)
        DEFAULT_X1COLOR, // Extra 1 color note:DEFAULT_X1COLOR is COLOR_BLACK
        DEFAULT_X2COLOR, // Extra 2 color note:DEFAULT_X2COLOR is COLOR_GREEN
        DEFAULT_X3COLOR  // Extra 3 color note:DEFAULT_X3COLOR is COLOR_BLUE
      }, // Colors
      IText(12.f, EAlign::Center) // Label text properties note:DEFAULT_LABEL_TEXT is {DEFAULT_TEXT_SIZE + 5.f, EVAlign::Top} DEFAULT_TEXT_SIZE is 14.f
      // Value text properties note:DEFAULT_VALUE_TEXT is {DEFAULT_TEXT_SIZE, EVAlign::Bottom}
      // Hide cursor note:DEFAULT_HIDE_CURSOR is true
      // Draw frame note:DEFAULT_DRAW_FRAME is true
      // Draw shadows note:DEFAULT_DRAW_SHADOWS is true
      // Emboss note:DEFAULT_EMBOSS is false
      // Roundness note:DEFAULT_ROUNDNESS is 0.f
      // Frame thickness note:DEFAULT_FRAME_THICKNESS is 1.f
      // Shadow offset note:DEFAULT_SHADOW_OFFSET is 3.f
      // Widget frac note:DEFAULT_WIDGET_FRAC is 1.f
      // Angle note:DEFAULT_WIDGET_ANGLE is 0.f (in degrees)
    };
    
    const IVStyle aboutstyle {
      true,
      false,
      {
        DEFAULT_BGCOLOR,
        DEFAULT_FGCOLOR,
        DEFAULT_PRCOLOR,
        DEFAULT_FRCOLOR,
        DEFAULT_HLCOLOR,
        DEFAULT_SHCOLOR,
        DEFAULT_X1COLOR,
        DEFAULT_X2COLOR,
        DEFAULT_X3COLOR
      },
      textprops2
    };
    
    auto incaction = [&](IControl* pCaller) {
      DefaultClickActionFunc(pCaller);
      iplug::IPluginBase::IncrementPreset(true);
    };
    
    auto decaction = [&](IControl* pCaller) {
      DefaultClickActionFunc(pCaller);
      iplug::IPluginBase::IncrementPreset(false);
    };
    
    mIncButton = new IVButtonControl(IRECT(290.f, 10.f, 314.f, 34.f), incaction, "", incstyle, false, false, EVShape::Triangle);
    pGraphics->AttachControl(mIncButton);
    mIncButton->SetAngle(90.f);
    
    mDecButton = new IVButtonControl(IRECT(92.f, 10.f, 116.f, 34.f), decaction, "", incstyle, false, false, EVShape::Triangle);
    pGraphics->AttachControl(mDecButton);
    mDecButton->SetAngle(270.f);
    
    pGraphics->AttachControl(new IVButtonControl(IRECT(10.f, 10.f, 70.f, 34.f), DefaultClickActionFunc, "About", aboutstyle, true, false));
    
    pGraphics->AttachControl(new IPresetMenu(IRECT(112.f, 10.f, 288.f, 28.f), textprops, pSubMenuNames, COLOR_BLACK));
    pGraphics->AttachControl(new IPresetFileMenuDev(IRECT(328.f, 10.f, 346.f, 28.f), pParamNames, COLOR_BLACK, COLOR_LIGHT_GRAY));
    
    mMSlider = new MultiSliderControlV(IRECT(10.f, 190.f, 330.f, 390.f), kDummyParamForMultislider, NUM_SLIDERS, 10, COLOR_BLACK, COLOR_BLUE, textcolor);
    pGraphics->AttachControl(mMSlider);
    
    pGraphics->AttachControl(new IVSliderControl(IRECT(340.f, 190.f, 380.f, 390.f), kGain, "Level"));
    
    mAboutControl = new IAboutControl(0.f, 0.f, PLUG_WIDTH, PLUG_HEIGHT, 5.f, IColor(210, 0, 0, 0), IRECT(10.f, 10.f, 70.f, 34.f), 16.f, 24.f, COLOR_LIGHT_GRAY, "Roboto-Regular");
    pGraphics->AttachControl(mAboutControl);
    mAboutControl->SetHeader(mfrlogo, PLUG_URL_STR, true);
    mAboutControl->SetBody(pAboutTextBody, 7);
    mAboutControl->SetFooter(vst2logo, vst3logo, aulogo, aaxlogo);
  };
#endif
  //Initialize Presets
  CreatePresets();
}

IPlugChunks::~IPlugChunks(){}

#if IPLUG_DSP
void IPlugChunks::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  double* in1 = inputs[0];
  double* in2 = inputs[1];
  double* out1 = outputs[0];
  double* out2 = outputs[1];
  
  int samplesPerBeat = (int) GetSamplesPerBeat();
  int samplePos = (int) GetSamplePos();
  
  int count = (int)mCount;
  int prevcount = (int)mPrevCount;
  
  for (int s = 0; s < nFrames; ++s, ++in1, ++in2, ++out1, ++out2)
  {
    int mod = (samplePos + s) % (samplesPerBeat * BEAT_DIV);
    
    count = mod / (samplesPerBeat / BEAT_DIV);
    
    if (count >= NUM_SLIDERS)
    {
      count = 0;
    }
    
    if (count != prevcount)
    {//if (GetGUI())
      if (GetUI())
      {
        mMSlider->SetHighlight(count);
      }
    }
    
    prevcount = count;
    
    *out1 = *in1 * mSteps[count] * mGain;
    *out2 = *in2 * mSteps[count] * mGain;
  }
  
  mCount = count;
  mPrevCount = prevcount;
  /*const double gain = GetParam(kGain)->Value() / 100.;
  const int nChans = NOutChansConnected();
  
  for (int s = 0; s < nFrames; s++) {
    for (int c = 0; c < nChans; c++) {
      outputs[c][s] = inputs[c][s] * gain;
    }
  }*/
}

void IPlugChunks::Reset()
{
  mCount = 0;
  mPrevCount = ULONG_MAX;
}

void IPlugChunks::OnParamChange(int paramIdx)
{
  switch (paramIdx)
  {
    case kDummyParamForMultislider:
      mMSlider->GetLatestChange(mSteps);
      break;
    case kGain:
      mGain = GetParam(kGain)->DBToAmp();
      break;
    default:
      break;
  }
}

void IPlugChunks::OnIdle()
{
  if(GetUI() != nullptr)
  {
    if(!mUIJustOpened)
    {
      mMSlider->SetState(mSteps);
      mMSlider->SetHighlight((int)mCount);
      mUIJustOpened = true;
    }
  }
  else
    mUIJustOpened = false;
}

void IPlugChunks::AddDumpedPresets(int* n)
{
  MakePresetFromBlob("Ramp Up", "AAAAAJqZqT8AAAAAmpm5PwAAAIA9Csc/AAAAAAAA0D8AAABA4XrUPwAAAIDC9dg/AAAAwMzM3D8AAAAQ16PgPwAAALBH4eI/AAAA0MzM5D8AAADwUbjmPwAAAAjXo+g/AAAAKFyP6j8AAADMzMzsPwAAAOxRuO4/AAAAAAAA8D8AAAAAAAC8Pg==", 136);
  ++*n;
  MakePresetFromBlob("Ramp Down", "AAAA7FG47j8AAABI4XrsPwAAALBH4eo/AAAAGK5H6T8AAABwPQrnPwAAANDMzOQ/AAAAwB6F4z8AAAAghevhPwAAAAB7FN4/AAAAgOtR2D8AAABAuB7VPwAAAACuR9E/AAAAgEfhyj8AAAAAhevBPwAAAABSuK4/AAAAAOB6hD8AAAAAAAC8Pg==", 136);
  ++*n;
  MakePresetFromBlob("Triangle", "AAAAAIXrwT8AAACAR+HKPwAAAEBcj9I/AAAAgBSu1z8AAADA9SjcPwAAABDXo+A/AAAAsEfh4j8AAABQuB7lPwAAAGBmZuY/AAAAMDMz4z8AAAAAAADgPwAAAMD1KNw/AAAAQI/C1T8AAAAArkfRPwAAAICPwsU/AAAAAJqZuT8AAAAAAAAAAA==", 136);
  ++*n;
  MakePresetFromBlob("Inv Triangle", "AAAAAAAA8D8AAABQuB7tPwAAAKBwPeo/AAAAcD0K5z8AAABA4XrkPwAAAJDC9eA/AAAAwEfh2j8AAABAj8LVPwAAAECPwtU/AAAAwMzM3D8AAAAghevhPwAAANDMzOQ/AAAAgBSu5z8AAACYmZnpPwAAAFyPwu0/AAAAAAAA8D8AAAAAAAAAAA==", 136);
  ++*n;
  MakePresetFromBlob("Da Endz", "AAAAAAAA8D8AAAAA4HqEPwAAAADgeoQ/AAAAAOB6hD8AAAAA4HqEPwAAAADgeoQ/AAAAAOB6hD8AAAAA4HqEPwAAAADgeoQ/AAAAAOB6hD8AAAAA4HqEPwAAAADgeoQ/AAAAAOB6hD8AAAAA4HqEPwAAAADgeoQ/AAAAAAAA8D8AAAAAAAAAAA==", 136);
  ++*n;
  MakePresetFromBlob("Alternate", "AAAAAAAA8D8AAAAA4HqEPwAAAAAAAPA/AAAAAOB6hD8AAAAAAADwPwAAAADgeoQ/AAAAAAAA8D8AAAAA4HqEPwAAAAAAAPA/AAAAAOB6hD8AAAAAAADwPwAAAADgeoQ/AAAAAAAA8D8AAAAA4HqEPwAAAAAAAPA/AAAAAOB6hD8AAAAAAAAAAA==", 136);
  ++*n;
  MakePresetFromBlob("Alt Ramp Down", "AAAAAAAA8D8AAAAA4HqEPwAAALgehes/AAAAAOB6hD8AAACI61HoPwAAAADgeoQ/AAAAQArX4z8AAAAA4HqEPwAAAAAAAOA/AAAAAOB6hD8AAABAuB7VPwAAAADgeoQ/AAAAAKRwzT8AAAAA4HqEPwAAAAAzM8M/AAAAAOB6hD8AAAAAAAAAAA==", 136);
  ++*n;
  MakePresetFromBlob("Alt Ramp Up", "AAAAgJmZyT8AAAAA4HqEPwAAAIBmZtY/AAAAAOB6hD8AAAAAKVzfPwAAAADgeoQ/AAAAMFyP4j8AAAAA4HqEPwAAAEDheuQ/AAAAAOB6hD8AAADwKFznPwAAAADgeoQ/AAAAIIXr6T8AAAAA4HqEPwAAANijcO0/AAAAAOB6hD8AAAAAAAAAAA==", 136);
  ++*n;
}

// this over-ridden method is called when the host is trying to store the plug-in state and needs to get the current data from your algorithm
bool IPlugChunks::SerializeState(IByteChunk &chunk) const
{
  double v;
  
  // serialize the multi-slider state state before serializing the regular params
  for (int i = 0; i< NUM_SLIDERS; i++)
  {
    v = mSteps[i];
    chunk.Put(&v);
  }
  
  return SerializeParams(chunk); // must remember to call SerializeParams at the end
}

// this over-ridden method is called when the host is trying to load the plug-in state and you need to unpack the data into your algorithm
int IPlugChunks::UnserializeState(const IByteChunk &chunk, int startPos)
{
  double v = 0.0;
  
  // unserialize the multi-slider state before unserializing the regular params
  for (int i = 0; i< NUM_SLIDERS; i++)
  {
    v = 0.0;
    startPos = chunk.Get(&v, startPos);
    mSteps[i] = v;
  }
  
  // If UI is open (control exists), update values in control and set dirty
  if(GetUI())
    mMSlider->SetState(mSteps);
  
  return UnserializeParams(chunk, startPos); // must remember to call UnserializeParams at the end
}

bool IPlugChunks::CompareState(const uint8_t* pIncomingState, int startPos) const
{
  bool isEqual = true;
  const double* data = (const double*) pIncomingState;
  startPos = NUM_SLIDERS * sizeof(double);
  isEqual = (memcmp(data, mSteps, startPos) == 0);
  isEqual &= IPlugAPIBase::CompareState(pIncomingState, startPos); // fuzzy compare regular params
  
  return isEqual;
}

void IPlugChunks::PresetsChangedByHost()
{
  if(GetUI())
  {
    // If UI is open (control exists), update values in control and set dirty
    mMSlider->SetState(mSteps);
    GetUI()->SetAllControlsDirty();
  }
}
#endif
