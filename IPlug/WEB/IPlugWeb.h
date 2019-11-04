/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#ifndef _IPLUGAPI_
#define _IPLUGAPI_

#include "IPlugAPIBase.h"
#include <emscripten/val.h>

BEGIN_IPLUG_NAMESPACE

/** Used to pass various instance info to the API class */
struct InstanceInfo
{};

/** This is used for the UI "editor" - controller side of a WAM or remote editors that communicate with desktop iPlug plug-ins via web sockets
 * @ingroup APIClasses */
class IPlugWeb : public IPlugAPIBase
{
public:
  IPlugWeb(const InstanceInfo& info, const Config& config);

  //IEditorDelegate  
  void SendParameterValueFromUI(int paramIdx, double value) override;
//  void BeginInformHostOfParamChangeFromUI(int paramIdx) override; // TODO: as soon as we actually have a WAM host these are needed
//  void EndInformHostOfParamChangeFromUI(int paramIdx) override; // TODO: as soon as we actually have a WAM host these are needed
  void SendMidiMsgFromUI(const IMidiMsg& msg) override;
  void SendSysexMsgFromUI(const ISysEx& msg) override;
  void SendArbitraryMsgFromUI(int messageTag, int controlTag = kNoTag, int dataSize = 0, const void* pData = nullptr) override;

private:
  WDL_String mWAMCtrlrJSObjectName;
  IByteChunk mSPVFUIBuf;
  IByteChunk mSMMFUIBuf;
  IByteChunk mSSMFUIBuf;
  IByteChunk mSAMFUIBuf;
};

IPlugWeb* MakePlug(const InstanceInfo& info);

END_IPLUG_NAMESPACE

#endif
