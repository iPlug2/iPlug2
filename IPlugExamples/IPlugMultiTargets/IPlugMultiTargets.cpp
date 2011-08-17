#include "IPlugMultiTargets.h"
#include "IPlug_include_in_plug_src.h"
#include "resource.h"

#ifndef OS_IOS
#include "IControl.h"
#include "IKeyboardControl.h"
#endif

const int kNumPrograms = 1;

#define PITCH 440.

#ifndef M_PI
#define M_PI  (3.14159265)
#endif

enum EParams
{
  kGainL = 0,
  kGainR,
 // kMode,
  kNumParams
};

class IKnobMultiControlText : public IKnobControl  
{
private:
  IRECT mTextRECT, mImgRECT;
  IBitmap mBitmap;
  
public:
  IKnobMultiControlText(IPlugBase* pPlug, IRECT pR, int paramIdx, IBitmap* pBitmap, IText* pText)
	:	IKnobControl(pPlug, pR, paramIdx), mBitmap(*pBitmap)
  {
    mText = *pText;
    mTextRECT = IRECT(mRECT.L, mRECT.B-20, mRECT.R, mRECT.B);
    mImgRECT = IRECT(mRECT.L, mRECT.T, &mBitmap);
    mDisablePrompt = false;
	}
	
	~IKnobMultiControlText() {}
	
  bool Draw(IGraphics* pGraphics)
  {
    int i = 1 + int(0.5 + mValue * (double) (mBitmap.N - 1));
    i = BOUNDED(i, 1, mBitmap.N);
    pGraphics->DrawBitmap(&mBitmap, &mImgRECT, i, &mBlend);
    //pGraphics->FillIRect(&COLOR_WHITE, &mTextRECT);
    
    char disp[20];
    mPlug->GetParam(mParamIdx)->GetDisplayForHost(disp);
    
    if (CSTR_NOT_EMPTY(disp)) {
      return pGraphics->DrawIText(&mText, disp, &mTextRECT);
    }
    return true;
  }
  
  bool OnKeyDown(int x, int y, int key)
  {
    switch (key) {
      case KEY_SPACE:
        DBGMSG("space bar handled\n");
        return true;
      default:
        return false;
    }
  }
  
	void OnMouseDown(int x, int y, IMouseMod* pMod)
	{
    if (mTextRECT.Contains(x, y)) PromptUserInput(&mTextRECT);
    else {
      OnMouseDrag(x, y, 0, 0, pMod);
    }
	}
  
  void OnMouseDblClick(int x, int y, IMouseMod* pMod)
  {
#ifdef RTAS_API
    PromptUserInput(&mTextRECT);
#else
    if (mDefaultValue >= 0.0) {
      mValue = mDefaultValue;
      SetDirty();
    }
#endif
  }
  
};

IPlugMultiTargets::IPlugMultiTargets(IPlugInstanceInfo instanceInfo)
: IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo), 
  mGainL(1.),
  mGainR(1.),
  mNoteGain(0.),
  mPhase(0),
  mSampleRate(44100.),
  mFreq(440.),
  mNumKeys(0),
  mKey(-1)
{
  TRACE;
  
  memset(mKeyStatus, 0, 128 * sizeof(bool));
    
  //arguments are: name, defaultVal, minVal, maxVal, step, label
  GetParam(kGainL)->InitDouble("GainL", -12.0, -70.0, 12.0, 0.1, "dB");
  GetParam(kGainR)->InitDouble("GainR", -12.0, -70.0, 12.0, 0.1, "dB");
//  GetParam(kMode)->InitEnum("Mode", 0, 6);
//  GetParam(kMode)->SetDisplayText(0, "a");
//  GetParam(kMode)->SetDisplayText(1, "b");
//  GetParam(kMode)->SetDisplayText(2, "c");
//  GetParam(kMode)->SetDisplayText(3, "d");
//  GetParam(kMode)->SetDisplayText(4, "e");
//  GetParam(kMode)->SetDisplayText(5, "f");
  
#ifndef OS_IOS
  IGraphics* pGraphics = MakeGraphics(this, kWidth, kHeight);
  pGraphics->AttachBackground(BG_ID, BG_FN);
  
  IBitmap knob = pGraphics->LoadIBitmap(KNOB_ID, KNOB_FN, kKnobFrames);
  IText text = IText(14);
  
  pGraphics->AttachControl(new IKnobMultiControlText(this, IRECT(kGainX, kGainY, kGainX + 48, kGainY + 48 + 20), kGainL, &knob, &text));
  pGraphics->AttachControl(new IKnobMultiControlText(this, IRECT(kGainX + 75, kGainY, kGainX + 48 + 75, kGainY + 48 + 20), kGainR, &knob, &text));
  
  IBitmap regular = pGraphics->LoadIBitmap(WHITE_KEY_ID, WHITE_KEY_FN, 6);
  IBitmap sharp   = pGraphics->LoadIBitmap(BLACK_KEY_ID, BLACK_KEY_FN);
  
  //                    C#     D#          F#      G#      A#
  int coords[12] = { 0, 7, 12, 20, 24, 36, 43, 48, 56, 60, 69, 72 };
  mKeyboard = new IKeyboardControl(this, kKeybX, kKeybY, 48, 5, &regular, &sharp, coords);
  
  pGraphics->AttachControl(mKeyboard);
  
  IBitmap about = pGraphics->LoadIBitmap(ABOUTBOX_ID, ABOUTBOX_FN);
  mAboutBox = new IBitmapOverlayControl(this, 100, 100, &about, IRECT(540, 250, 680, 290));
  pGraphics->AttachControl(mAboutBox);
  AttachGraphics(pGraphics);
#endif
  //MakePreset("preset 1", ... );
  MakeDefaultPreset((char *) "-", kNumPrograms);
}

IPlugMultiTargets::~IPlugMultiTargets() {}

#ifdef OS_IOS
void IPlugMultiTargets::ProcessSingleReplacing(float** inputs, float** outputs, int nFrames)
{
  // Mutex is already locked for us.
  float* in1 = inputs[0];
  float* in2 = inputs[1];
  float* out1 = outputs[0];
  float* out2 = outputs[1];
  
  for (int s = 0; s < nFrames; ++s, ++in1, ++in2, ++out1, ++out2)
  {
    *out1 = sinf( 2.f * M_PI * mFreq * mPhase / mSampleRate ) * mGainLSmoother.Process(mGainL * mNoteGain);
    *out2 = sinf( 2.f * M_PI * mFreq * 1.01f * (mPhase++) / mSampleRate ) * mGainRSmoother.Process(mGainR * mNoteGain);
  } 
}
#else
void IPlugMultiTargets::ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames)
{
  // Mutex is already locked for us.
  double* in1 = inputs[0];
  double* in2 = inputs[1];
  double* out1 = outputs[0];
  double* out2 = outputs[1];
  
  IKeyboardControl* pKeyboard = (IKeyboardControl*) mKeyboard;
  
  if (pKeyboard->GetKey() != mKey)
  {
    IMidiMsg msg;
    
    if (mKey >= 0) 
      msg.MakeNoteOffMsg(mKey + 48, 0, 0);
    
    mMidiQueue.Add(&msg);
    
    mKey = pKeyboard->GetKey();
    
    if (mKey >= 0) 
      msg.MakeNoteOnMsg(mKey + 48, pKeyboard->GetVelocity(), 0, 0);
    
    mMidiQueue.Add(&msg);
  }
  
  for (int offset = 0; offset < nFrames; ++offset, ++in1, ++in2, ++out1, ++out2)
  {
    while (!mMidiQueue.Empty())
    {
      IMidiMsg* pMsg = mMidiQueue.Peek();
      if (pMsg->mOffset > offset) break;
      
      // TODO: make this work on win sa
      #if !defined(OS_WIN) && !defined(SA_API)
        SendMidiMsg(pMsg);
      #endif
      
      int status = pMsg->StatusMsg();
      
      switch (status)
      {
        case IMidiMsg::kNoteOn:
        case IMidiMsg::kNoteOff:
        {
          int velocity = pMsg->Velocity();
          // Note On
          if (status == IMidiMsg::kNoteOn && velocity)
          {
            mNote = pMsg->NoteNumber();
            mFreq = 440. * pow(2., (mNote - 69.) / 12.);
            mNoteGain = velocity / 127.;
          }
          // Note Off
          else // if (status == IMidiMsg::kNoteOff || !velocity)
          {
            if (pMsg->NoteNumber() == mNote)
              mNote = -1;
            
            mNoteGain = 0.;
          }
          break;
        }
      }
      
      mMidiQueue.Remove();
    }
    
    *out1 = sin( 2. * M_PI * mFreq * mPhase / mSampleRate ) * mGainLSmoother.Process(mGainL * mNoteGain);
    *out2 = sin( 2. * M_PI * mFreq * 1.01 * (mPhase++) / mSampleRate ) * mGainRSmoother.Process(mGainR * mNoteGain);
    
  }
  
  mMidiQueue.Flush(nFrames);
}
#endif

void IPlugMultiTargets::Reset()
{
  TRACE;
  IMutexLock lock(this);
  
  mPhase = 0;
  mNoteGain = 0.;
  mSampleRate = GetSampleRate();
  mMidiQueue.Resize(GetBlockSize());
}

void IPlugMultiTargets::OnParamChange(int paramIdx)
{
  IMutexLock lock(this);
  
  switch (paramIdx)
  {
    case kGainL:
      mGainL = GetParam(kGainL)->DBToAmp();
      break;
    case kGainR:
      mGainR = GetParam(kGainR)->DBToAmp();
      break;
    default:
      break;
  }
}

void IPlugMultiTargets::ProcessMidiMsg(IMidiMsg* pMsg)
{
#ifdef OS_IOS
  // Handle the MIDI message.
  int status = pMsg->StatusMsg();
  int velocity = pMsg->Velocity();
  
  switch (status)
  {
    case IMidiMsg::kNoteOn:
    case IMidiMsg::kNoteOff:
      // filter only note messages
      if (status == IMidiMsg::kNoteOn && velocity)
      {
        printf("note on\n");
        mPhase = 0;
        mNoteGain = 1.;
      }
      else
      {
        printf("note off\n");
        mNoteGain = 0.;
      }
      break;
    default:
      return;
  }
#else
  
  int status = pMsg->StatusMsg();
  
  // filter only note messages
  switch (status)
  {
    case IMidiMsg::kNoteOn:
      mKeyStatus[pMsg->NoteNumber()] = true;
      mNumKeys += 1;
      break;
    case IMidiMsg::kNoteOff:
      mKeyStatus[pMsg->NoteNumber()] = false;
      mNumKeys -= 1;
      break;
    default:
      return; // if !note message, nothing gets added to the queue
  }
  
  mKeyboard->SetDirty();
  mMidiQueue.Add(pMsg);
#endif
}

// Should return non-zero if one or more keys are playing.
int IPlugMultiTargets::GetNumKeys()
{
  IMutexLock lock(this);
  return mNumKeys;
}

// Should return true if the specified key is playing.
bool IPlugMultiTargets::GetKeyStatus(int key)
{
  IMutexLock lock(this);
  return mKeyStatus[key];
}

#ifndef OS_IOS
//Called by the standalone wrapper if someone clicks about
bool IPlugMultiTargets::HostRequestingAboutBox()
{
  IMutexLock lock(this);
  if(GetGUI()) {
    // get the IBitmapOverlay to show
    mAboutBox->SetValueFromPlug(1.);
    mAboutBox->Hide(false);
  }
  return true;
}
#endif