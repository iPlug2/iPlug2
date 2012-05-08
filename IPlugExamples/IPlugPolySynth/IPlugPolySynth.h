#ifndef __IPLUGPOLYSYNTH__
#define __IPLUGPOLYSYNTH__

#include "IPlug_include_in_plug_hdr.h"
#include "IMidiQueue.h"
#include "IPlugPolySynthDSP.h"

#define MAX_VOICES 16
#define ATTACK_DEFAULT 5.
#define DECAY_DEFAULT 20.
#define RELEASE_DEFAULT 500.
#define TIME_MIN 2.
#define TIME_MAX 5000.

class IPlugPolySynth : public IPlug
{
public:

  IPlugPolySynth(IPlugInstanceInfo instanceInfo);
  ~IPlugPolySynth();

  void Reset();
  void OnParamChange(int paramIdx);

  void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames);
  bool HostRequestingAboutBox();

  int GetNumKeys();
  bool GetKeyStatus(int key);
  void ProcessMidiMsg(IMidiMsg* pMsg);
  void NoteOnOff(IMidiMsg* pMsg);

private:

  void NoteOnOffPoly(IMidiMsg* pMsg);
  int FindFreeVoice();

  IBitmapOverlayControl* mAboutBox;
  IControl* mKeyboard;

  IMidiQueue mMidiQueue;

  int mActiveVoices;
  int mKey;
  int mNumHeldKeys;
  bool mKeyStatus[128]; // array of on/off for each key

  double mSampleRate;

  CVoiceState mVS[MAX_VOICES];
  CWTOsc* mOsc;
  CADSREnvL* mEnv;
  double* mTable;
};

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

#endif //__IPLUGPOLYSYNTH__
