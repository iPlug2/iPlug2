#if WINDOWS_VERSION
#include "Mac2Win.H"
#endif

#ifndef __CTEMPLATENOUIPPROCESS_AS__
#define __CTEMPLATENOUIPPROCESS_AS__

#include "FicPluginConnections.h"
#include "CTemplateNoUIProcess.h"
#include "TemplateDefs.h"

#include "CEffectProcessAS.h"

class IPlugProcessAS : public CTemplateNoUIProcess, public CEffectProcessAS
{
  public:
    IPlugProcessAS(void);
    virtual ~IPlugProcessAS(void);

    virtual void UpdateControlValueInAlgorithm(long aControlIndex);
    virtual void SetViewOrigin (Point anOrigin);

  protected:
    double mGain[EffectLayerDef::MAX_NUM_CONNECTIONS];

    // We use these to track the meter values
    SFloat32 mMeterVal[EffectLayerDef::MAX_NUM_CONNECTIONS];
    SFloat32 mMeterMin[EffectLayerDef::MAX_NUM_CONNECTIONS];

  protected:
    virtual void GetMetersFromDSPorRTAS(long *allMeters, bool *clipIndicators); 
    virtual UInt32 ProcessAudio(bool isMasterBypassed);

};

CEffectProcess* NewIPlugProcessAS();

#endif  // __CTEMPLATENOUIPPROCESS_AS__
