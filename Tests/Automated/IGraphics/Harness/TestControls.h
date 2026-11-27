/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @brief Controls and helpers shared by the IGraphics tests
 */

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "IControl.h"
#include "IGraphicsHeadless.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** A control that records every event that IGraphics delivers to it */
class ProbeControl : public IControl
{
public:
  struct Event
  {
    std::string mName;
    float mX = 0.f;
    float mY = 0.f;
    float mDX = 0.f; // the wheel delta for "wheel"
    float mDY = 0.f;
    IMouseMod mMod;
    int mVK = 0;
    std::string mStr; // dropped string, key utf8 or text entry result
  };

  ProbeControl(const IRECT& bounds, int paramIdx = kNoParameter, IActionFunction actionFunc = nullptr)
  : IControl(bounds, paramIdx, actionFunc)
  {}

  ProbeControl(const IRECT& bounds, const std::initializer_list<int>& params)
  : IControl(bounds, params)
  {}

  void Draw(IGraphics& g) override
  {
    mNumDraws++;
    mClipDuringDraw = static_cast<IGraphicsHeadless&>(g).mClipRect;
    g.FillRect(mColor, mRECT);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override { Log("down", x, y, 0.f, 0.f, mod); IControl::OnMouseDown(x, y, mod); }
  void OnMouseUp(float x, float y, const IMouseMod& mod) override { Log("up", x, y, 0.f, 0.f, mod); }
  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override { Log("drag", x, y, dX, dY, mod); }
  void OnMouseDblClick(float x, float y, const IMouseMod& mod) override { Log("dblclick", x, y, 0.f, 0.f, mod); IControl::OnMouseDblClick(x, y, mod); }
  void OnMouseWheel(float x, float y, const IMouseMod& mod, float d) override { Log("wheel", x, y, d, 0.f, mod); }
  void OnMouseOver(float x, float y, const IMouseMod& mod) override { Log("over", x, y, 0.f, 0.f, mod); IControl::OnMouseOver(x, y, mod); }
  void OnMouseOut() override { if (mLogMouseOut) Log("out"); IControl::OnMouseOut(); }
  void OnTouchCancelled(float x, float y, const IMouseMod& mod) override { Log("cancel", x, y, 0.f, 0.f, mod); }

  bool OnKeyDown(float x, float y, const IKeyPress& key) override
  {
    Log("keydown", x, y).mVK = key.VK;
    mEvents.back().mStr = key.utf8;
    return mHandlesKeys;
  }

  bool OnKeyUp(float x, float y, const IKeyPress& key) override
  {
    Log("keyup", x, y).mVK = key.VK;
    mEvents.back().mStr = key.utf8;
    return mHandlesKeys;
  }

  void OnDrop(const char* str) override { Log("drop").mStr = str; }

  void OnDropMultiple(const std::vector<const char*>& paths) override
  {
    Event& event = Log("dropmultiple");

    for (auto path : paths)
    {
      event.mStr += path;
      event.mStr += ";";
    }
  }

  void OnResize() override { mNumResizes++; }
  void OnRescale() override { mNumRescales++; }
  void OnInit() override { mNumInits++; }
  void OnAttached() override { mNumAttaches++; }
  void OnGUIIdle() override { mNumIdles++; }

  void OnMsgFromDelegate(int msgTag, int dataSize, const void* pData) override
  {
    const uint8_t* pBytes = static_cast<const uint8_t*>(pData);
    mLastMsgTag = msgTag;
    mLastMsgData.assign(pBytes, pBytes + dataSize);
    mNumMsgs++;
  }

  void OnMidi(const IMidiMsg& msg) override { mMidi.push_back(msg); }

  void CreateContextMenu(IPopupMenu& contextMenu) override
  {
    for (auto& item : mContextMenuItems)
      contextMenu.AddItem(item.c_str());
  }

  void OnContextSelection(int itemSelected) override { mContextSelection = itemSelected; mNumContextSelections++; }

  void OnPopupMenuSelection(IPopupMenu* pSelectedMenu, int valIdx) override
  {
    mNumPopupSelections++;
    mPopupSelection = pSelectedMenu ? pSelectedMenu->GetChosenItemIdx() : -1;
    mPopupSelectionMenu = pSelectedMenu;
    mPopupSelectionText = pSelectedMenu && pSelectedMenu->GetChosenItem() ? pSelectedMenu->GetChosenItem()->GetText() : "";
    mPopupSelectionValIdx = valIdx;
    IControl::OnPopupMenuSelection(pSelectedMenu, valIdx);
  }

  void OnTextEntryCompletion(const char* str, int valIdx) override { Log("textentry").mStr = str; }

  int GetValIdxForPos(float x, float y) const override
  {
    return mValIdxForPos > kNoValIdx ? mValIdxForPos : IControl::GetValIdxForPos(x, y);
  }

  /** Makes IGraphics deliver double clicks to this control as a second mouse down */
  void SetDblAsSingleClick(bool enable) { mDblAsSingleClick = enable; }

  int Count(const char* name) const
  {
    int n = 0;
    for (auto& event : mEvents)
      n += (event.mName == name);
    return n;
  }

  /** @return the last event with the given name, or nullptr */
  const Event* Last(const char* name) const
  {
    for (auto itr = mEvents.rbegin(); itr != mEvents.rend(); ++itr)
    {
      if (itr->mName == name)
        return &(*itr);
    }
    return nullptr;
  }

  /** @return the names of the events so far, separated by spaces */
  std::string Names() const
  {
    std::string names;
    for (auto& event : mEvents)
    {
      if (!names.empty())
        names += " ";
      names += event.mName;
    }
    return names;
  }

  void ClearEvents() { mEvents.clear(); }

  std::vector<Event> mEvents;
  std::vector<IMidiMsg> mMidi;
  std::vector<std::string> mContextMenuItems;
  IColor mColor = COLOR_RED;
  IRECT mClipDuringDraw;
  bool mHandlesKeys = false;
  bool mLogMouseOut = true;
  int mValIdxForPos = kNoValIdx;
  int mNumDraws = 0;
  int mNumResizes = 0;
  int mNumRescales = 0;
  int mNumInits = 0;
  int mNumAttaches = 0;
  int mNumIdles = 0;
  int mNumMsgs = 0;
  int mLastMsgTag = -1;
  std::vector<uint8_t> mLastMsgData;
  int mContextSelection = -2;
  int mNumContextSelections = 0;
  int mPopupSelection = -2;
  int mPopupSelectionValIdx = -2;
  IPopupMenu* mPopupSelectionMenu = nullptr;
  std::string mPopupSelectionText;
  int mNumPopupSelections = 0;

private:
  Event& Log(const char* name, float x = 0.f, float y = 0.f, float dX = 0.f, float dY = 0.f, const IMouseMod& mod = IMouseMod())
  {
    Event event;
    event.mName = name;
    event.mX = x; event.mY = y; event.mDX = dX; event.mDY = dY;
    event.mMod = mod;
    mEvents.push_back(event);
    return mEvents.back();
  }
};

/** The first 24 bytes of a PNG file with the given dimensions (signature + IHDR length/type/width/height), which is
 * all that the headless backend reads in order to make a bitmap */
inline std::vector<uint8_t> MakePNGHeader(int w, int h)
{
  std::vector<uint8_t> data = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A, 0, 0, 0, 13, 'I', 'H', 'D', 'R'};

  for (int v : {w, h})
  {
    data.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
    data.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
    data.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    data.push_back(static_cast<uint8_t>(v & 0xFF));
  }

  return data;
}

/** Ends a control's animation as the animation function would when the time is up. Animations run on the real clock,
 * so the tests complete them explicitly rather than waiting. A no-op if no animation is running. */
inline void FinishAnimation(IControl* pControl)
{
  if (pControl->GetAnimationFunction())
    pControl->OnEndAnimation();
}

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
