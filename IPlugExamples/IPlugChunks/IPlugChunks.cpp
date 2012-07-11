#include "IPlugChunks.h"
#include "IPlug_include_in_plug_src.h"
#include "IControl.h"
#include "resource.h"
#include <math.h>

// The number of presets/programs
const int kNumPrograms = 8;

// An enumerated list for all the parameters of the plugin
enum EParams
{
  kGain = 0,
  kNumParams, // the last element is used to store the total number of parameters
};

const int kDummyParamForMultislider = -2;

enum ELayout
{
  kW = 400,
  kH = 300
};

class ITempPresetSaveButtonControl : public IPanelControl
{
public:
  ITempPresetSaveButtonControl(IPlugBase *pPlug, IRECT pR)
    : IPanelControl(pPlug, pR, &COLOR_RED) {}

  void OnMouseDown(int x, int y, IMouseMod* pMod)
  {
    WDL_String presetFilePath;
    mPlug->GetGUI()->DesktopPath(&presetFilePath);
#ifdef OS_WIN
    presetFilePath.Append("\\IPlugChunksPreset.txt");
#else //OSX
    presetFilePath.Append("IPlugChunksPreset.txt");
#endif
    mPlug->DumpPresetBlob(presetFilePath.Get());
  }

};

class PresetFunctionsMenu : public IPanelControl
{
public:
  PresetFunctionsMenu(IPlugBase *pPlug, IRECT pR)
    : IPanelControl(pPlug, pR, &COLOR_BLUE)
  {}

  bool Draw(IGraphics* pGraphics)
  {
    pGraphics->FillIRect(&COLOR_WHITE, &mRECT);

    int ax = mRECT.R - 8;
    int ay = mRECT.T + 4;
    int bx = ax + 4;
    int by = ay;
    int cx = ax + 2;
    int cy = ay + 2;

    pGraphics->FillTriangle(&COLOR_GRAY, ax , ay, bx, by, cx, cy, &mBlend);

    return true;
  }

  void OnMouseDown(int x, int y, IMouseMod* pMod)
  {
    if (pMod->L)
    {
      doPopupMenu();
    }

    Redraw(); // seems to need this
    SetDirty();
  }

  void doPopupMenu()
  {
    IPopupMenu menu;

    IGraphics* gui = mPlug->GetGUI();

    menu.AddItem("Save Program...");
    menu.AddItem("Save Bank...");
    menu.AddSeparator();
    menu.AddItem("Load Program...");
    menu.AddItem("Load Bank...");

    if(gui->CreateIPopupMenu(&menu, &mRECT))
    {
      int itemChosen = menu.GetChosenItemIdx();

      //printf("chosen %i /n", itemChosen);
      switch (itemChosen)
      {
        case 0: //Save Program
          char disp[MAX_PRESET_NAME_LEN];
          strcpy(disp, mPlug->GetPresetName(mPlug->GetCurrentPresetIdx()));
          mPlug->SaveProgramAsFXP(disp);
          break;
        case 1: //Save Bank
          mPlug->SaveBankAsFXB("IPlugChunks Bank");
          break;
        case 3: //Load Preset
          mPlug->LoadProgramFromFXP();
          break;
        case 4: // Load Bank
          mPlug->LoadBankFromFXB();
          break;
        default:
          break;
      }
    }
  }
};

IPlugChunks::IPlugChunks(IPlugInstanceInfo instanceInfo)
  : IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo), mGain(1.)
{
  TRACE;
  
  memset(mSteps, 0, NUM_SLIDERS*sizeof(double));

  // Define parameter ranges, display units, labels.
  //arguments are: name, defaultVal, minVal, maxVal, step, label
  GetParam(kGain)->InitDouble("Gain", 0.0, -70.0, 12.0, 0.1, "dB");

  MakePresetFromBlob("Ramp Up", "AAAAAJqZqT8AAAAAmpm5PwAAAIA9Csc/AAAAAAAA0D8AAABA4XrUPwAAAIDC9dg/AAAAwMzM3D8AAAAQ16PgPwAAALBH4eI/AAAA0MzM5D8AAADwUbjmPwAAAAjXo+g/AAAAKFyP6j8AAADMzMzsPwAAAOxRuO4/AAAAAAAA8D8AAAAAAAC8Pg==", 136);
  MakePresetFromBlob("Ramp Down", "AAAA7FG47j8AAABI4XrsPwAAALBH4eo/AAAAGK5H6T8AAABwPQrnPwAAANDMzOQ/AAAAwB6F4z8AAAAghevhPwAAAAB7FN4/AAAAgOtR2D8AAABAuB7VPwAAAACuR9E/AAAAgEfhyj8AAAAAhevBPwAAAABSuK4/AAAAAOB6hD8AAAAAAAC8Pg==", 136);
  MakePresetFromBlob("Triangle", "AAAAAIXrwT8AAACAR+HKPwAAAEBcj9I/AAAAgBSu1z8AAADA9SjcPwAAABDXo+A/AAAAsEfh4j8AAABQuB7lPwAAAGBmZuY/AAAAMDMz4z8AAAAAAADgPwAAAMD1KNw/AAAAQI/C1T8AAAAArkfRPwAAAICPwsU/AAAAAJqZuT8AAAAAAAAAAA==", 136);
  MakePresetFromBlob("Inv Triangle", "AAAAAAAA8D8AAABQuB7tPwAAAKBwPeo/AAAAcD0K5z8AAABA4XrkPwAAAJDC9eA/AAAAwEfh2j8AAABAj8LVPwAAAECPwtU/AAAAwMzM3D8AAAAghevhPwAAANDMzOQ/AAAAgBSu5z8AAACYmZnpPwAAAFyPwu0/AAAAAAAA8D8AAAAAAAAAAA==", 136);
  MakePresetFromBlob("Da Endz", "AAAAAAAA8D8AAAAA4HqEPwAAAADgeoQ/AAAAAOB6hD8AAAAA4HqEPwAAAADgeoQ/AAAAAOB6hD8AAAAA4HqEPwAAAADgeoQ/AAAAAOB6hD8AAAAA4HqEPwAAAADgeoQ/AAAAAOB6hD8AAAAA4HqEPwAAAADgeoQ/AAAAAAAA8D8AAAAAAAAAAA==", 136);
  MakePresetFromBlob("Alternate", "AAAAAAAA8D8AAAAA4HqEPwAAAAAAAPA/AAAAAOB6hD8AAAAAAADwPwAAAADgeoQ/AAAAAAAA8D8AAAAA4HqEPwAAAAAAAPA/AAAAAOB6hD8AAAAAAADwPwAAAADgeoQ/AAAAAAAA8D8AAAAA4HqEPwAAAAAAAPA/AAAAAOB6hD8AAAAAAAAAAA==", 136);
  MakePresetFromBlob("Alt Ramp Down", "AAAAAAAA8D8AAAAA4HqEPwAAALgehes/AAAAAOB6hD8AAACI61HoPwAAAADgeoQ/AAAAQArX4z8AAAAA4HqEPwAAAAAAAOA/AAAAAOB6hD8AAABAuB7VPwAAAADgeoQ/AAAAAKRwzT8AAAAA4HqEPwAAAAAzM8M/AAAAAOB6hD8AAAAAAAAAAA==", 136);
  MakePresetFromBlob("Alt Ramp Up", "AAAAgJmZyT8AAAAA4HqEPwAAAIBmZtY/AAAAAOB6hD8AAAAAKVzfPwAAAADgeoQ/AAAAMFyP4j8AAAAA4HqEPwAAAEDheuQ/AAAAAOB6hD8AAADwKFznPwAAAADgeoQ/AAAAIIXr6T8AAAAA4HqEPwAAANijcO0/AAAAAOB6hD8AAAAAAAAAAA==", 136);

  IGraphics* pGraphics = MakeGraphics(this, kW, kH);
  pGraphics->AttachPanelBackground(&COLOR_BLUE);

  mMSlider = new MultiSliderControlV(this, IRECT(10, 10, 170, 110), kDummyParamForMultislider, NUM_SLIDERS, 10, &COLOR_WHITE, &COLOR_BLACK, &COLOR_RED);

  pGraphics->AttachControl(mMSlider);
  pGraphics->AttachControl(new IVSliderControl(this, IRECT(200, 10, 220, 110), kGain, 20, &COLOR_WHITE, &COLOR_GREEN));

  //pGraphics->AttachControl(new ITempPresetSaveButtonControl(this, IRECT(350, 250, 390, 290)));
  pGraphics->AttachControl(new PresetFunctionsMenu(this, IRECT(350, 250, 390, 290)));

  AttachGraphics(pGraphics);
  
  // call RestorePreset(0) here which will initialize the multislider in the gui and the mSteps array
  RestorePreset(0);
}

IPlugChunks::~IPlugChunks() {}

void IPlugChunks::ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames)
{
  // Mutex is already locked for us.

  double* in1 = inputs[0];
  double* in2 = inputs[1];
  double* out1 = outputs[0];
  double* out2 = outputs[1];

  int samplesPerBeat = (int) GetSamplesPerBeat();
  int samplePos = (int) GetSamplePos();

  int count = mCount;
  int prevcount = mPrevCount;

  for (int s = 0; s < nFrames; ++s, ++in1, ++in2, ++out1, ++out2)
  {
    int mod = (samplePos + s) % (samplesPerBeat * BEAT_DIV);

    count = mod / (samplesPerBeat / BEAT_DIV);

    if (count >= NUM_SLIDERS)
    {
      count = 0;
    }

    if (count != prevcount)
    {
      if (GetGUI())
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

}

void IPlugChunks::Reset()
{
  TRACE;
  IMutexLock lock(this);

  mCount = 0;
  mPrevCount = ULONG_MAX;
}

void IPlugChunks::OnParamChange(int paramIdx)
{
  TRACE;

  IMutexLock lock(this);

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

// this over-ridden method is called when the host is trying to store the plug-in state and needs to get the current data from your algorithm
bool IPlugChunks::SerializeState(ByteChunk* pChunk)
{
  TRACE;
  IMutexLock lock(this);
  double v;

  // serialize the multi-slider state state before serializing the regular params
  for (int i = 0; i< NUM_SLIDERS; i++)
  {
    v = mSteps[i];
    pChunk->Put(&v);
  }

  return IPlugBase::SerializeParams(pChunk); // must remember to call SerializeParams at the end
}

// this over-ridden method is called when the host is trying to load the plug-in state and you need to unpack the data into your algorithm
int IPlugChunks::UnserializeState(ByteChunk* pChunk, int startPos)
{
  TRACE;
  IMutexLock lock(this);
  double v = 0.0;

  // unserialize the multi-slider state before unserializing the regular params
  for (int i = 0; i< NUM_SLIDERS; i++)
  {
    v = 0.0;
    startPos = pChunk->Get(&v, startPos);
    mSteps[i] = v;
  }

  // update values in control, will set dirty
  if(mMSlider)
    mMSlider->SetState(mSteps);

  return IPlugBase::UnserializeParams(pChunk, startPos); // must remember to call UnserializeParams at the end
}

bool IPlugChunks::CompareState(const unsigned char* incomingState, int startPos)
{
  bool isEqual = true;
  const double* data = (const double*) incomingState;
  startPos = NUM_SLIDERS * sizeof(double);
  isEqual = (memcmp(data, mSteps, startPos) == 0);
  isEqual &= IPlugBase::CompareState(incomingState, startPos); // fuzzy compare regular params
  
  return isEqual;
}

void IPlugChunks::PresetsChangedByHost()
{
  TRACE;
  IMutexLock lock(this);

  if(mMSlider)
    mMSlider->SetState(mSteps);

  if(GetGUI())
    GetGUI()->SetAllControlsDirty();
}
