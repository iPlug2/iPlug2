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
  IPlugWeb(IPlugInstanceInfo instanceInfo, IPlugConfig config);
  EHost GetHost() override { return EHost::kHostWWW; }

  //IEditorDelegate  
  void SendParameterValueFromUI(int paramIdx, double value) override;
//  void BeginInformHostOfParamChangeFromUI(int paramIdx) override; // TODO: as soon as we actually have a WAM host these are needed
//  void EndInformHostOfParamChangeFromUI(int paramIdx) override; // TODO: as soon as we actually have a WAM host these are needed
  void SendMidiMsgFromUI(const IMidiMsg& msg) override;
  void SendSysexMsgFromUI(const ISysEx& msg) override;
  void SendArbitraryMsgFromUI(int messageTag, int dataSize = 0, const void* pData = nullptr) override;

private:
  WDL_String mWAMCtrlrJSObjectName;
  #if WEBSOCKET_CLIENT
  IByteChunk mSPVFUIBuf;
  IByteChunk mSMMFUIBuf;
  IByteChunk mSSMFUIBuf;
  IByteChunk mSAMFUIBuf;
  #endif
};

IPlugWeb* MakePlug();

#endif
