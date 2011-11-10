#ifndef __IPLUGEFFECT__
#define __IPLUGEFFECT__

#include "IPlug_include_in_plug_hdr.h"
#include "IVecSliders.h"

/*
 
 IPlugChunks - an example of storing data in chunks, and custom IControl classes
 
 A step sequenced volume control / tarnce gate
 
 
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
  
  bool SerializeParams(ByteChunk* pChunk);
  int UnserializeParams(ByteChunk* pChunk, int startPos);
  
  //bool SerializePresets(ByteChunk* pChunk);
  //int UnserializePresets(ByteChunk* pChunk, int startPos);
  
  void PresetsChangedByHost();
  
private:
  
  double mSteps[NUM_SLIDERS];
  
  double mGain;
	unsigned long mCount, mPrevCount;
	
	MultiSliderControlV *mMSlider;
};

#endif
