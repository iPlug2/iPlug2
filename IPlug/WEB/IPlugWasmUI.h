/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#ifndef _IPLUG_HYBRID_UI_
#define _IPLUG_HYBRID_UI_

#include "IPlugAPIBase.h"
#include <emscripten/val.h>

BEGIN_IPLUG_NAMESPACE

/** Used to pass various instance info to the API class */
struct InstanceInfo
{};

/** Hybrid UI class - IGraphics side for split DSP/UI builds.
 * This is the UI module that works with IPlugWasmDSP.
 * It receives messages from DSP via JavaScript callbacks and
 * sends messages to DSP via the controller's port.postMessage.
 *
 * Build flags: -DIPLUG_EDITOR=1
 *
 * @ingroup APIClasses */
class IPlugWasmUI : public IPlugAPIBase
{
public:
  IPlugWasmUI(const InstanceInfo& info, const Config& config);

  // IEditorDelegate - send to DSP via controller
  void SendParameterValueFromUI(int paramIdx, double value) override;
  void SendMidiMsgFromUI(const IMidiMsg& msg) override;
  void SendSysexMsgFromUI(const ISysEx& msg) override;
  void SendArbitraryMsgFromUI(int msgTag, int ctrlTag = kNoTag, int dataSize = 0, const void* pData = nullptr) override;

  /** Plug-ins that override OnIdle() must call the base class! */
  void OnIdle() override { SendDSPIdleTick(); }

  // Called from JS when DSP sends messages (via controller)
  // These are exposed via EMSCRIPTEN_BINDINGS

private:
  /** Sends a tick message to DSP via controller */
  void SendDSPIdleTick();

  WDL_String mControllerName;
};

IPlugWasmUI* MakePlug(const InstanceInfo& info);

END_IPLUG_NAMESPACE

#endif // _IPLUG_HYBRID_UI_
