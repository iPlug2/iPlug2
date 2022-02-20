/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

#include "IPlugEditorDelegate.h"

/** This EditorDelegate communicates... */

BEGIN_IPLUG_NAMESPACE

class CocoaEditorDelegate : public IEditorDelegate
{
public:
  CocoaEditorDelegate(int nParams);
  virtual ~CocoaEditorDelegate();

  void* OpenWindow(void* pParent) override;
  void CloseWindow() override;
  
  bool OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData) override;
  void OnParamChangeUI(int paramIdx, EParamSource source) override;
  void OnMidiMsgUI(const IMidiMsg& msg) override;
  void OnSysexMsgUI(const ISysEx& msg) override;
  void SendControlValueFromDelegate(int ctrlTag, double normalizedValue) override;
  void SendControlMsgFromDelegate(int ctrlTag, int msgTag, int dataSize, const void* pData) override;
  void SendParameterValueFromDelegate(int paramIdx, double value, bool normalized) override;
protected:
  void* mViewController = nullptr;
};

END_IPLUG_NAMESPACE
