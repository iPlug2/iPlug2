#ifndef __IPLUGEFFECT__
#define __IPLUGEFFECT__

#include "IPlug_include_in_plug_hdr.h"
#include "IPlugChunks_controls.h"

/*

 IPlugChunks - an example of storing data in chunks, and custom IControl classes

 A step sequenced volume control / tarnce gate

 Using chunks allows you to store arbitary data (e.g. a hidden, non-automatable parameter, a filepath etc) in the plugin's state,
 i.e. when you save a preset to a file or when you save the project in your host

 You need to override SerializeState / UnserializeState and set PLUG_DOES_STATE_CHUNKS 1 in resource.h

 // WARNING - I'm not happy with how the multislider data is shared with the high priority thread
 // need to rethink that
 
*/


#define NUM_SLIDERS 16
#define BEAT_DIV 4 // semiquavers

class IPlugChunks : public IPlug
{
public:

  IPlugChunks(IPlugInstanceInfo instanceInfo);
  ~IPlugChunks();

  void Reset();
  void OnParamChange(int paramIdx);

  void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames);

  bool SerializeState(ByteChunk* pChunk);
  int UnserializeState(ByteChunk* pChunk, int startPos);
  bool CompareState(const unsigned char* incomingState, int startPos);
  
  void PresetsChangedByHost();

private:

  double mSteps[NUM_SLIDERS];

  double mGain;
  unsigned long mCount, mPrevCount;

  MultiSliderControlV *mMSlider;
};

#endif
