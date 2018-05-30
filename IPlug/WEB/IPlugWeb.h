/*
 ==============================================================================
 
 This file is part of the iPlug 2 library
 
 Oli Larkin et al. 2018 - https://www.olilarkin.co.uk
 
 iPlug 2 is an open source library subject to commercial or open-source
 licensing.
 
 The code included in this file is provided under the terms of the WDL license
 - https://www.cockos.com/wdl/
 
 ==============================================================================
 */

#ifndef _IPLUGAPI_
#define _IPLUGAPI_

#include "IPlugAPIBase.h"
#include <emscripten/val.h>

/** Used to pass various instance info to the API class */
struct IPlugInstanceInfo
{};

/**  */
class IPlugWeb : public IPlugAPIBase
{
public:
  IPlugWeb(IPlugInstanceInfo instanceInfo, IPlugConfig config)
  : IPlugAPIBase(config, kAPIWEB)
  {}

  //IPlugAPIBase
//  void BeginInformHostOfParamChange(int idx) override {};
//  void InformHostOfParamChange(int idx, double normalizedValue) override {};
//  void EndInformHostOfParamChange(int idx) override {};
//  void InformHostOfProgramChange() override {};
  EHost GetHost() override { return EHost::kHostWWW; }
//  void ResizeGraphics() override {};
//  void HostSpecificInit() override {};
  
  //IEditorDelegate
  void SetParameterValueFromUI(int paramIdx, double value) override;
//  void BeginInformHostOfParamChangeFromUI(int paramIdx) override;
//  void EndInformHostOfParamChangeFromUI(int paramIdx) override;
  
  #ifndef NO_IGRAPHICS
  //IGraphicsEditorDelegate
  void AttachGraphics(IGraphics* pGraphics) override;
  #endif  
};

IPlugWeb* MakePlug();

#endif
