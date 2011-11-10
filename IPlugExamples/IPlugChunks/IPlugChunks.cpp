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
    char presetFilePath[100];
    char* home = getenv("HOME");
    sprintf(presetFilePath, "%s/Desktop/%s", home, "IPlugChunksPreset.txt");
    
    mPlug->DumpPresetBlob(presetFilePath);
  }
  
};

IPlugChunks::IPlugChunks(IPlugInstanceInfo instanceInfo)
:	IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo), mGain(1.)
{
	TRACE;
  
  memset(mSteps, 0, NUM_SLIDERS * sizeof(double));
	
  // Define parameter ranges, display units, labels.
	//arguments are: name, defaultVal, minVal, maxVal, step, label
	GetParam(kGain)->InitDouble("Gain", 0.0, -70.0, 12.0, 0.1, "dB");
	
  MakePresetFromBlob("hellomunkey","AAAAEK5H4T8AAAAghevhPwAAAKBwPeI/AAAAMDMz4z8AAADA9SjkPwAAAFC4HuU/AAAAQOF65D8AAADQzMzkPwAAAFC4HuU/AAAA0KNw5T8AAABgj8LlPwAAAGCPwuU/AAAA4HoU5j8AAABgj8LlPwAAAGBmZuY/AAAAcD0K5z8AAAAAAAAAAA==", 136);
  MakePresetFromBlob("test", "AAAAAAAA8D8AAAAAAADwPwAAAAAAAPA/AAAAAAAA8D8AAAAAAADwPwAAAAAAAPA/AAAA2KNw7T8AAACQwvXoPwAAACCF6+E/AAAAAK5H0T8AAAAAAADQPwAAAACkcM0/AAAAgEfhyj8AAACAPQrHPwAAAAAzM8M/AAAAAD0Ktz8AAAAAAAAAAA==", 136);
  MakePresetFromBlob("mondkfjsdjf", "AAAAEK5H4T8AAAAghevhPwAAAKBwPeI/AAAAMDMz4z8AAAAA4HqEPwAAAADgeoQ/AAAAAOB6hD8AAAAA4HqEPwAAAADgeoQ/AAAAAOB6hD8AAABgj8LlPwAAAADgeoQ/AAAAAOB6hD8AAAAAUriuPwAAAADgeoQ/AAAAAOB6hD8AAMwehUtRwA==", 136);
  MakePresetFromBlob("hello4","AAAAEK5H4T8AAAAghevhPwAAAKBwPeI/AAAAMDMz4z8AAADA9SjkPwAAAFC4HuU/AAAAQOF65D8AAADQzMzkPwAAAFC4HuU/AAAA0KNw5T8AAABgj8LlPwAAAGCPwuU/AAAA4HoU5j8AAABgj8LlPwAAAGBmZuY/AAAAcD0K5z8AAAAAAAAAAA==", 136);
  MakePresetFromBlob("hello5","AAAAEK5H4T8AAAAghevhPwAAAKBwPeI/AAAAMDMz4z8AAADA9SjkPwAAAFC4HuU/AAAAQOF65D8AAADQzMzkPwAAAFC4HuU/AAAA0KNw5T8AAABgj8LlPwAAAGCPwuU/AAAA4HoU5j8AAABgj8LlPwAAAGBmZuY/AAAAcD0K5z8AAAAAAAAAAA==", 136);
  MakePresetFromBlob("hello6","AAAAEK5H4T8AAAAghevhPwAAAKBwPeI/AAAAMDMz4z8AAADA9SjkPwAAAFC4HuU/AAAAQOF65D8AAADQzMzkPwAAAFC4HuU/AAAA0KNw5T8AAABgj8LlPwAAAGCPwuU/AAAA4HoU5j8AAABgj8LlPwAAAGBmZuY/AAAAcD0K5z8AAAAAAAAAAA==", 136);
  MakePresetFromBlob("hello7","AAAAEK5H4T8AAAAghevhPwAAAKBwPeI/AAAAMDMz4z8AAADA9SjkPwAAAFC4HuU/AAAAQOF65D8AAADQzMzkPwAAAFC4HuU/AAAA0KNw5T8AAABgj8LlPwAAAGCPwuU/AAAA4HoU5j8AAABgj8LlPwAAAGBmZuY/AAAAcD0K5z8AAAAAAAAAAA==", 136);
  MakePresetFromBlob("hello8","AAAAEK5H4T8AAAAghevhPwAAAKBwPeI/AAAAMDMz4z8AAADA9SjkPwAAAFC4HuU/AAAAQOF65D8AAADQzMzkPwAAAFC4HuU/AAAA0KNw5T8AAABgj8LlPwAAAGCPwuU/AAAA4HoU5j8AAABgj8LlPwAAAGBmZuY/AAAAcD0K5z8AAAAAAAAAAA==", 136);

	IGraphics* pGraphics = MakeGraphics(this, kW, kH);
	pGraphics->AttachPanelBackground(&COLOR_BLUE);
	
	mMSlider = new MultiSliderControlV(this, IRECT(10, 10, 170, 110), kDummyParamForMultislider, NUM_SLIDERS, 10, &COLOR_WHITE, &COLOR_BLACK, &COLOR_RED);
  pGraphics->AttachControl(mMSlider);
  pGraphics->AttachControl(new IVSliderControl(this, IRECT(200, 10, 220, 110), kGain, 20, &COLOR_WHITE, &COLOR_GREEN));

  pGraphics->AttachControl(new ITempPresetSaveButtonControl(this, IRECT(350, 250, 390, 290)));
  
	AttachGraphics(pGraphics);
}

// the destructor, where you free any memory you allocated
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
    
   if (count >= NUM_SLIDERS) {
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
	IMutexLock lock(this);
	
	switch (paramIdx)
	{
    case kDummyParamForMultislider:
      //could also get entire array
      mMSlider->GetLatestChange(mSteps);
      ModifyCurrentPreset();
      break;
		case kGain:
			mGain = GetParam(kGain)->DBToAmp();
			break;
			
		default:
			break;
	}
}

bool IPlugChunks::SerializeParams(ByteChunk* pChunk)
{
  TRACE;
  WDL_MutexLock lock(&mMutex);
  bool savedOK = true;
  int i;
  double v;
  
  // added this to serialize the slider state
  for (i = 0; i< NUM_SLIDERS && savedOK; i++) {
    v =  mSteps[i];
    savedOK &= (pChunk->Put(&v) > 0);
  }
  
  int n = mParams.GetSize();
  for (i = 0; i < n && savedOK; ++i) {
    IParam* pParam = mParams.Get(i);
    v = pParam->Value();
    savedOK &= (pChunk->Put(&v) > 0);
  }
  return savedOK;
}

int IPlugChunks::UnserializeParams(ByteChunk* pChunk, int startPos)
{
  TRACE;
  WDL_MutexLock lock(&mMutex);
  int i, n = mParams.GetSize(), pos = startPos;
  double v = 0.0;
  
  // added this to unserialize the slider state
  for (i = 0; i< NUM_SLIDERS; i++) {
    v = 0.0;
    pos = pChunk->Get(&v, pos);
    mSteps[i] = v;
  }
  
  for (i = 0; i < n && pos >= 0; ++i) {
    IParam* pParam = mParams.Get(i);
    double v = 0.0;
    Trace(TRACELOC, "%d %s", i, pParam->GetNameForHost());
    pos = pChunk->Get(&v, pos);
    pParam->Set(v);
  }
  
  OnParamReset();
  return pos;
}

bool IPlugChunks::SerializeState(ByteChunk* pChunk)
{
  TRACE;
	IMutexLock lock(this);
	
  int i;
  double v;
  bool savedOK = true;
  // added this to serialize the slider state
  for (i = 0; i< NUM_SLIDERS && savedOK; i++) {
    v =  mSteps[i];
    savedOK &= (pChunk->Put(&v) > 0);
  }
  
  if (savedOK) {
    return IPlugBase::SerializeParams(pChunk);
  }
  else {
    return false;
  }
}

int IPlugChunks::UnserializeState(ByteChunk* pChunk, int startPos)
{
  TRACE;
	IMutexLock lock(this);
	
  // added this to unserialize the slider state
  for (int i = 0; i< NUM_SLIDERS; i++) {
    double v = 0.0;
    startPos = pChunk->Get(&v, startPos);
    mSteps[i] = v;
  }
  
	return IPlugBase::UnserializeParams(pChunk, startPos);
}

/*
bool IPlugChunks::SerializePresets(ByteChunk* pChunk)
{
  TRACE;
  bool savedOK = true;
  int n = mPresets.GetSize();
  for (int i = 0; i < n && savedOK; ++i) {
    IPreset* pPreset = mPresets.Get(i);
    pChunk->PutStr(pPreset->mName);
    pChunk->PutBool(pPreset->mInitialized);
    if (pPreset->mInitialized) {
      savedOK &= (pChunk->PutChunk(&(pPreset->mChunk)) > 0);
    }
  }
  return savedOK;
}

int IPlugChunks::UnserializePresets(ByteChunk* pChunk, int startPos)
{
  TRACE;
  WDL_String name;
  int n = mPresets.GetSize(), pos = startPos;
  for (int i = 0; i < n && pos >= 0; ++i) {
    IPreset* pPreset = mPresets.Get(i);
    pos = pChunk->GetStr(&name, pos);
    strcpy(pPreset->mName, name.Get());
    pos = pChunk->GetBool(&(pPreset->mInitialized), pos);
    if (pPreset->mInitialized) {
      pos = UnserializeParams(pChunk, pos);
      if (pos > 0) {
        pPreset->mChunk.Clear();
        SerializeParams(&(pPreset->mChunk));
      }
    }
  }
  RestorePreset(mCurrentPresetIdx);
  return pos;
}
*/

void IPlugChunks::PresetsChangedByHost()
{
  IMutexLock lock(this);

  if (mMSlider)
    mMSlider->SetState(mSteps);
  
  if(GetGUI())
    GetGUI()->SetAllControlsDirty();
}
