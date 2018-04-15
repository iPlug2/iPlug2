#ifndef _IPLUGAPI_
#define _IPLUGAPI_

#include "IPlugBase.h"
#include <emscripten/val.h>

/** Used to pass various instance info to the API class */
struct IPlugInstanceInfo
{};

/**  */
class IPlugWEB : public IPlugBase
{
public:
  IPlugWEB(IPlugInstanceInfo instanceInfo, IPlugConfig config)
  : IPlugBase(config, kAPIWEB)
  {}

  //IPlugBase
  void BeginInformHostOfParamChange(int idx) override {};
  void InformHostOfParamChange(int idx, double normalizedValue) override {};
  void EndInformHostOfParamChange(int idx) override {};
  void InformHostOfProgramChange() override {};
  EHost GetHost() override { return EHost::kHostWWW; }
  void ResizeGraphics() override {};
  void HostSpecificInit() override {};

  //This can go if IPlugProcessor Goes
  virtual void ProcessBlock(float** inputs, float** outputs, int nFrames) {};
  virtual void ProcessMidiMsg(const IMidiMsg& msg) {};
  inline int NOutChansConnected() {};
  
  //IDelegate
  void SetParameterValueFromUI(int paramIdx, double value) override;
  void BeginInformHostOfParamChangeFromUI(int paramIdx) override;
  void EndInformHostOfParamChangeFromUI(int paramIdx) override;
  
  #ifndef NO_IGRAPHICS
  //IGraphicsDelegate
  void AttachGraphics(IGraphics* pGraphics) override;
  #endif  
};

IPlugWEB* MakePlug();

#endif
