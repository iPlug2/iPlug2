#if WINDOWS_VERSION
  #include "Mac2Win.H"
#endif

#ifndef __IPLUGPPROCESS_RTAS__
#define __IPLUGPPROCESS_RTAS__

#include "FicPluginConnections.h"
#include "IPlugProcess.h"
#include "CEffectProcessRTAS.h"
#include "CEffectProcessMuSh.h"
#include "CPluginControl_LinearGain.h"

class IPlugProcessRTAS : public IPlugProcess, public CEffectProcessRTAS
{
public:
  IPlugProcessRTAS(OSType type);
  virtual ~IPlugProcessRTAS(void) {}
  
  virtual ComponentResult SetChunk(OSType chunkID, SFicPlugInChunk *chunk);
  virtual ComponentResult GetChunk(OSType chunkID, SFicPlugInChunk *chunk);
  virtual ComponentResult GetChunkSize(OSType chunkID, long *size);
  virtual ComponentResult IsControlAutomatable(long aControlIndex, short *aItIsP);
  
  int GetBlockSize() { return mBlockSize; }
  
  double GetTempo();
  
protected:
  virtual void GetMetersFromDSPorRTAS(long *allMeters, bool *clipIndicators); 
  virtual void RenderAudio(float** inputs, float** outputs, long frames);
  
private:
  void HandleMIDI();
  OSType mPluginID;
  int mBlockSize;

// TODO: Meters? 
// SFloat32 mMeterVal[EffectLayerDef::MAX_NUM_CONNECTIONS];
// SFloat32 mMeterMin[EffectLayerDef::MAX_NUM_CONNECTIONS];
};

#endif  // __IPLUGPPROCESS_RTAS__
