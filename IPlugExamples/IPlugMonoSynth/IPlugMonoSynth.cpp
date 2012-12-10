#include "IPlugMonoSynth.h"
#include "IPlug_include_in_plug_src.h"
#include "resource.h"

#include "IControl.h"
#include "IKeyboardControl.h"


const int kNumPrograms = 8;

#define PITCH 440.

#ifndef M_PI
#define M_PI  (3.14159265)
#endif

enum EParams
{
  kGainL = 0,
  kGainR,
  kMode,
  kNumParams
};

IPlugMonoSynth::IPlugMonoSynth(IPlugInstanceInfo instanceInfo)
  : IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo),
    mGainL(1.),
    mGainR(1.),
    mNoteGain(0.),
    mPhase(0),
    mSampleRate(44100.),
    mFreq(440.),
    mNumKeys(0),
    mKey(-1),
    mPrevL(0.0),
    mPrevR(0.0)

{
  TRACE;

  memset(mKeyStatus, 0, 128 * sizeof(bool));

  //arguments are: name, defaultVal, minVal, maxVal, step, label
  GetParam(kGainL)->InitDouble("GainL", -12.0, -70.0, 12.0, 0.1, "dB");
  GetParam(kGainR)->InitDouble("GainR", -12.0, -70.0, 12.0, 0.1, "dB");
  GetParam(kMode)->InitEnum("Mode", 0, 6);
  GetParam(kMode)->SetDisplayText(0, "a");
  GetParam(kMode)->SetDisplayText(1, "b");
  GetParam(kMode)->SetDisplayText(2, "c");
  GetParam(kMode)->SetDisplayText(3, "d");
  GetParam(kMode)->SetDisplayText(4, "e");
  GetParam(kMode)->SetDisplayText(5, "f");

  IGraphics* pGraphics = MakeGraphics(this, kWidth, kHeight);
  pGraphics->AttachBackground(BG_ID, BG_FN);

  IBitmap knob = pGraphics->LoadIBitmap(KNOB_ID, KNOB_FN, kKnobFrames);
  IText text = IText(14);
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

  //MakePreset("preset 1", ... );
  MakeDefaultPreset((char *) "-", kNumPrograms);
}

IPlugMonoSynth::~IPlugMonoSynth() {}

void IPlugMonoSynth::ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames)
{
  // Mutex is already locked for us.
//  double* in1 = inputs[0];
//  double* in2 = inputs[1];
  double* out1 = outputs[0];
  double* out2 = outputs[1];
  double peakL = 0.0, peakR = 0.0;

  GetTime(&mTimeInfo);

  IKeyboardControl* pKeyboard = (IKeyboardControl*) mKeyboard;

  if (pKeyboard->GetKey() != mKey)
  {
    IMidiMsg msg;

    if (mKey >= 0)
    {
      msg.MakeNoteOffMsg(mKey + 48, 0, 0);
      mMidiQueue.Add(&msg);
    }

    mKey = pKeyboard->GetKey();

    if (mKey >= 0)
    {
      msg.MakeNoteOnMsg(mKey + 48, pKeyboard->GetVelocity(), 0, 0);
      mMidiQueue.Add(&msg);
    }
  }

  for (int offset = 0; offset < nFrames; ++offset, /*++in1, ++in2,*/ ++out1, ++out2)
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

    peakL = IPMAX(peakL, fabs(*out1));
    peakR = IPMAX(peakR, fabs(*out2));
  }

  const double METER_ATTACK = 0.6, METER_DECAY = 0.05;
  double xL = (peakL < mPrevL ? METER_DECAY : METER_ATTACK);
  double xR = (peakR < mPrevR ? METER_DECAY : METER_ATTACK);

  peakL = peakL * xL + mPrevL * (1.0 - xL);
  peakR = peakR * xR + mPrevR * (1.0 - xR);

  mPrevL = peakL;
  mPrevR = peakR;

  if (GetGUI())
  {
    GetGUI()->SetControlFromPlug(mMeterIdx_L, peakL);
    GetGUI()->SetControlFromPlug(mMeterIdx_R, peakR);
  }

  mMidiQueue.Flush(nFrames);
}

void IPlugMonoSynth::Reset()
{
  TRACE;
  IMutexLock lock(this);

  mPhase = 0;
  mNoteGain = 0.;
  mSampleRate = GetSampleRate();
  mMidiQueue.Resize(GetBlockSize());
}

void IPlugMonoSynth::OnParamChange(int paramIdx)
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

void IPlugMonoSynth::ProcessMidiMsg(IMidiMsg* pMsg)
{
  int status = pMsg->StatusMsg();
  int velocity = pMsg->Velocity();

  switch (status)
  {
    case IMidiMsg::kNoteOn:
    case IMidiMsg::kNoteOff:
      // filter only note messages
      if (status == IMidiMsg::kNoteOn && velocity)
      {
        mKeyStatus[pMsg->NoteNumber()] = true;
        mNumKeys += 1;
      }
      else
      {
        mKeyStatus[pMsg->NoteNumber()] = false;
        mNumKeys -= 1;
      }
      break;
    default:
      return; // if !note message, nothing gets added to the queue
  }

  mKeyboard->SetDirty();
  mMidiQueue.Add(pMsg);
}

// Should return non-zero if one or more keys are playing.
int IPlugMonoSynth::GetNumKeys()
{
  IMutexLock lock(this);
  return mNumKeys;
}

// Should return true if the specified key is playing.
bool IPlugMonoSynth::GetKeyStatus(int key)
{
  IMutexLock lock(this);
  return mKeyStatus[key];
}

//Called by the standalone wrapper if someone clicks about
bool IPlugMonoSynth::HostRequestingAboutBox()
{
  IMutexLock lock(this);
  if(GetGUI())
  {
    // get the IBitmapOverlay to show
    mAboutBox->SetValueFromPlug(1.);
    mAboutBox->Hide(false);
  }
  return true;
}
