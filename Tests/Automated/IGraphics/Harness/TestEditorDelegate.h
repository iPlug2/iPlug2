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
    mLastResizeNeededPlatformResize = needsPlatformResize;
    SetEditorSize(viewWidth, viewHeight);
    return true;
  }

  void OnUIOpen() override { mNumUIOpens++; IGEditorDelegate::OnUIOpen(); }
  void OnUIClose() override { mNumUICloses++; IGEditorDelegate::OnUIClose(); }

  void LayoutUI(IGraphics* pGraphics) override
  {
    mNumLayouts++;
    IGEditorDelegate::LayoutUI(pGraphics);
  }

  void OnParamChangeUI(int paramIdx, EParamSource source) override { mParamChangesUI.push_back(paramIdx); }

  /** Replace the layout function (takes effect the next time the UI is opened or laid out) */
  void SetLayoutFunc(LayoutFunc layoutFunc) { mLayoutFunc = layoutFunc; }

  /** Replace the function that creates the graphics, e.g. to open at a different size */
  void SetMakeGraphicsFunc(std::function<IGraphics*()> func) { mMakeGraphicsFunc = func; }

  /** @return the number of recorded parameter events of the given type for a parameter (or for all, if paramIdx < 0) */
  int CountParamEvents(ParamEvent::EType type, int paramIdx = -1) const
  {
    int n = 0;
    for (auto& event : mParamEvents)
      n += (event.mType == type && (paramIdx < 0 || event.mIdx == paramIdx));
    return n;
  }

  /** @return the last value sent from the UI for the parameter, or -1 if none was */
  double LastParamValue(int paramIdx) const
  {
    for (auto itr = mParamEvents.rbegin(); itr != mParamEvents.rend(); ++itr)
    {
      if (itr->mType == ParamEvent::EType::Value && itr->mIdx == paramIdx)
        return itr->mNormalizedValue;
    }
    return -1.;
  }

  void ClearRecorded()
  {
    mParamEvents.clear();
    mMessages.clear();
    mMidiFromUI.clear();
    mParamChangesUI.clear();
  }

  std::vector<ParamEvent> mParamEvents;
  std::vector<Message> mMessages;
  std::vector<IMidiMsg> mMidiFromUI;
  std::vector<int> mParamChangesUI; // the parameters OnParamChangeUI() was called for
  int mNumResizeRequests = 0;
  bool mLastResizeNeededPlatformResize = false;
  int mNumUIOpens = 0;
  int mNumUICloses = 0;
  int mNumLayouts = 0;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
