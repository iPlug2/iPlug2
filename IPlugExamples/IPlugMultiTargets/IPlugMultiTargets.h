#ifndef __IPLUGMULTITARGETS__
#define __IPLUGMULTITARGETS__

#include "IPlug_include_in_plug_hdr.h"
#include "IMidiQueue.h"

#ifndef OS_IOS
#include "IControl.h"
#endif

// http://www.musicdsp.org/archive.php?classid=3#257

class CParamSmooth
{
public:
  CParamSmooth() { a = 0.99; b = 1. - a; z = 0.; };
  ~CParamSmooth() {};
  inline double Process(double in) { z = (in * b) + (z * a); return z; }
private:
  double a, b, z;
};

class IPlugMultiTargets : public IPlug
{
public:

  IPlugMultiTargets(IPlugInstanceInfo instanceInfo);
  ~IPlugMultiTargets();

  void Reset();
  void OnParamChange(int paramIdx);

#ifdef OS_IOS
  void ProcessSingleReplacing(float** inputs, float** outputs, int nFrames);
#else
  void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames);
  bool HostRequestingAboutBox();
#endif

  int GetNumKeys();
  bool GetKeyStatus(int key);
  void ProcessMidiMsg(IMidiMsg* pMsg);

private:
#ifndef OS_IOS
  IBitmapOverlayControl* mAboutBox;
  IControl* mKeyboard;
  int mMeterIdx_L, mMeterIdx_R;
#endif

  IMidiQueue mMidiQueue;

  int mNumKeys; // how many keys are being played (via midi)
  bool mKeyStatus[128]; // array of on/off for each key

  int mPhase;
  int mNote;
  int mKey;

  double mGainL, mGainR;
  double mSampleRate;
  double mFreq;
  double mNoteGain;
  double mPrevL, mPrevR;

  ITimeInfo mTimeInfo;

  CParamSmooth mGainLSmoother, mGainRSmoother;
};

#ifndef OS_IOS
enum ELayout
{
  kWidth = GUI_WIDTH,  // width of plugin window
  kHeight = GUI_HEIGHT, // height of plugin window

  kKeybX = 1,
  kKeybY = 233,

  kGainX = 100,
  kGainY = 100,
  kKnobFrames = 60
};
#endif
#endif //__IPLUGMULTITARGETS__
