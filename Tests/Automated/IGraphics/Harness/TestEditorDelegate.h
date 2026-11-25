/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @brief A stand-alone IGEditorDelegate for testing IGraphics without a plug-in class
 */

#include <functional>
#include <vector>

#include "IGraphicsEditorDelegate.h"
#include "IGraphicsHeadless.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** Owns nParams parameters and a headless IGraphics, and records what the UI sends to it */
class TestEditorDelegate : public IGEditorDelegate
{
public:
  struct ParamEvent
  {
    enum class EType { Begin, Value, End };
    EType mType;
    int mIdx;
    double mNormalizedValue;
  };

  struct Message
  {
    int mMsgTag;
    int mCtrlTag;
    std::vector<uint8_t> mData;
  };

  using LayoutFunc = std::function<void(IGraphics*)>;

  TestEditorDelegate(int nParams, int w = 400, int h = 300, LayoutFunc layoutFunc = nullptr)
  : IGEditorDelegate(nParams)
  {
    SetEditorSize(w, h);
    mMakeGraphicsFunc = [this, w, h]() { return new IGraphicsHeadless(*this, w, h); };
    mLayoutFunc = layoutFunc;
  }

  ~TestEditorDelegate()
  {
    CloseUI();
  }

  /** Create the IGraphics and run the layout function, as when a host opens the editor */
  IGraphicsHeadless* OpenUI()
  {
    OpenWindow(nullptr);
    return UI();
  }

  void CloseUI()
  {
    if (GetUI())
      CloseWindow();
  }

  IGraphicsHeadless* UI() { return static_cast<IGraphicsHeadless*>(GetUI()); }

  void BeginInformHostOfParamChangeFromUI(int paramIdx) override { mParamEvents.push_back({ParamEvent::EType::Begin, paramIdx, 0.}); }

  void SendParameterValueFromUI(int paramIdx, double normalizedValue) override
  {
    mParamEvents.push_back({ParamEvent::EType::Value, paramIdx, normalizedValue});
    IGEditorDelegate::SendParameterValueFromUI(paramIdx, normalizedValue);
  }

  void EndInformHostOfParamChangeFromUI(int paramIdx) override { mParamEvents.push_back({ParamEvent::EType::End, paramIdx, 0.}); }

  void SendArbitraryMsgFromUI(int msgTag, int ctrlTag, int dataSize, const void* pData) override
  {
    const uint8_t* pBytes = static_cast<const uint8_t*>(pData);
    mMessages.push_back({msgTag, ctrlTag, std::vector<uint8_t>(pBytes, pBytes + dataSize)});
  }

  void SendMidiMsgFromUI(const IMidiMsg& msg) override { mMidiFromUI.push_back(msg); }

  bool EditorResizeFromUI(int viewWidth, int viewHeight, bool needsPlatformResize) override
  {
    mNumResizeRequests++;
    SetEditorSize(viewWidth, viewHeight);
    return true;
  }

  void OnUIOpen() override { mNumUIOpens++; IGEditorDelegate::OnUIOpen(); }
  void OnUIClose() override { mNumUICloses++; IGEditorDelegate::OnUIClose(); }

  std::vector<ParamEvent> mParamEvents;
  std::vector<Message> mMessages;
  std::vector<IMidiMsg> mMidiFromUI;
  int mNumResizeRequests = 0;
  int mNumUIOpens = 0;
  int mNumUICloses = 0;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
