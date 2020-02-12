/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @ingroup Controls
 * @copydoc IVKeyboardControl
 */

#include "IControl.h"
#include "IPlugMidi.h"

#include <map>

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

class IVKeyboardControl : public IControl , public IMultiTouchControlBase
{
  enum class GlideMode { Glissando, Pitch };
  enum class KeyLayoutMode { Contiguous, Piano };
  enum EValIDs { kGate = 0, kHeight, kRadius };
  
  static const IColor DEFAULT_BK_COLOR;
  static const IColor DEFAULT_WK_COLOR;
  static const IColor DEFAULT_PK_COLOR;
  static const IColor DEFAULT_FR_COLOR;
  static const IColor DEFAULT_HK_COLOR;
  
  class KeyControl : public IControl
  {
  public:
    KeyControl(const IRECT& bounds, int idx, bool isSharp, IVKeyboardControl* pParent)
    : IControl(bounds)
    , mIdx(idx)
    , mIsSharp(isSharp)
    , mParent(pParent)
    {
      mDblAsSingleClick = true;
      SetNVals(2);
    }
    
    void Draw(IGraphics& g) override
    {
      g.FillRect(GetValue(EValIDs::kGate) > 0.5 ? COLOR_BLACK : COLOR_WHITE, mRECT);
      g.DrawLine(COLOR_BLACK, mRECT.R-1, mRECT.T, mRECT.R-1, mRECT.B);
      
      if(mPointerDown)
        g.DrawHorizontalLine(COLOR_RED, mRECT, GetValue(EValIDs::kHeight));
      WDL_String str;
      str.SetFormatted(3, "%i", mIdx);
      g.DrawText(mText, str.Get(), mRECT);
    }
    
    void OnMouseDown(float x, float y, const IMouseMod& mod) override
    {
      mPointerDown = true;
      //parent sets this one dirty
      mParent->AddTouch(mod.touchID, x, y, mod.touchRadius);
      mParent->SetHit(mod.touchID, this);
      SnapToMouse(x, y, EDirection::Vertical, mRECT, EValIDs::kHeight);
    }
    
    void OnMouseUp(float x, float y, const IMouseMod& mod) override
    {
      mPointerDown = false;

      mParent->ReleaseTouch(mod.touchID);
      mParent->ClearHitIfMovedOffkey(mod.touchID, nullptr);
    }
    
    void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override
    {
      if(mParent->GetMPEEnabled())
      {
        mParent->UpdateTouch(mod.touchID, x, y, mod.touchRadius);
        mParent->HitMoved(mod.touchID, this);
        
        SnapToMouse(x, y, EDirection::Vertical, mRECT, EValIDs::kHeight);
        mParent->SendCtrl1(mod.touchID, GetValue(EValIDs::kHeight));
      }
    }
    
    void OnTouchCancelled(float x, float y, const IMouseMod& mod) override
    {
      mParent->ClearAllTouches();
    }
    
    int GetIdx() const
    {
      return mIdx;
    }
    
  private:
    bool mPointerDown = false;
    bool mIsSharp;
    int mIdx;
    IVKeyboardControl* mParent;
  };
  
  class HighlightControl : public IControl
  {
  public:
    HighlightControl(const IRECT& bounds, IVKeyboardControl* pParent, const IColor& color)
    : IControl(bounds)
    , mColor(color)
    {
      mIgnoreMouse = true;
    }
    
    void Draw(IGraphics& g) override
    {
      g.FillEllipse(mColor, mRECT);
    }
  private:
    IColor mColor;
  };
  
#pragma mark -
public:
  IVKeyboardControl(const IRECT& bounds, int minNote = 48, int maxNote = 60, bool roundedKeys = false,
                    const IColor& WK_COLOR = DEFAULT_WK_COLOR,
                    const IColor& BK_COLOR = DEFAULT_BK_COLOR,
                    const IColor& PK_COLOR = DEFAULT_PK_COLOR,
                    const IColor& FR_COLOR = DEFAULT_FR_COLOR,
                    const IColor& HK_COLOR = DEFAULT_HK_COLOR)
  : IControl(bounds)
  {
    SetNoteRange(minNote, maxNote);
    SetWantsMidi(true);
//    SetWantsMultiTouch(true);
    mIgnoreMouse = true;
    SetTargetRECT(IRECT());
  }
  
  void OnInit() override
  {
    GetUI()->AttachControl(mBackGroundControl = new IPanelControl(mRECT, COLOR_BLACK));
    CreateKeys(true);
  }
  
  void OnAttached() override
  {
//    CreateHighlights();
  }
  
  void Draw(IGraphics& g) override
  {
  }
  
  void SendMidiNoteMsg(int key, int velocity)
  {
    IMidiMsg msg;

    auto nn = 60 + key;

    if(velocity > 0)
      msg.MakeNoteOnMsg(nn, velocity, 0);
    else
      msg.MakeNoteOffMsg(nn, 0);

    GetDelegate()->SendMidiMsgFromUI(msg);
  }

  /** Sets a keys state to on, if its not allready, storing an ptr to the keycontrol in a map, keyed by the touch identifier
   * @param touchID The touch identifier
   * @param pKey ptr to the key being set on */
  void SetHit(ITouchID touchId, KeyControl* pKey)
  {
    if(pKey->GetValue(EValIDs::kGate) < 0.5)
    {
      pKey->SetValue(1., EValIDs::kGate);
      pKey->SetDirty(false);
      mTouchPrevouslyHit[touchId] = pKey;
      SendMidiNoteMsg(pKey->GetIdx(), 1 + static_cast<int>(pKey->GetValue(EValIDs::kHeight) * 126));
    }
  }
  
  /** Clears key previously linked to a touch identifier
   * @param touchID The touch identifier
   * @param pKey ptr to the key being set on */
  void ClearHitIfMovedOffkey(ITouchID touchId, KeyControl* pKey)
  {
    auto itr = mTouchPrevouslyHit.find(touchId);
    
    //if we found the touch
    if(itr != mTouchPrevouslyHit.end())
    {
      KeyControl* pPrevKey = itr->second;
      
      if(pPrevKey != pKey)
      {
        pPrevKey->SetValue(0.);
        pPrevKey->SetDirty(false);
        SendMidiNoteMsg(pPrevKey->GetIdx(), 0);
        mTouchPrevouslyHit[touchId] = pKey;
      }
    }
  }
  
  /** Update when touch changes location, if touch moves to new key, make sure old key sends a note-off and new key sends a note-on
   * @param touchID The touch identifier
   * @param pKey ptr to the key being set on */
  void HitMoved(ITouchID touchId, KeyControl* pKey)
  {
    TrackedTouch* pTouch = GetTouchWithIdentifier(touchId);

    if(pTouch)
    {
      auto radius = pTouch->radius;
      
      //loop over keycontrols and set dirty if touch is inside
      for(int i=0;i<mKeyControls.GetSize();i++)
      {
        KeyControl* pTestKey = mKeyControls.Get(i);
        
        if(pTestKey->IsHit(pTouch->x-radius, pTouch->y-radius))
          pTestKey->SetDirty();
        
        if(pTestKey->IsHit(pTouch->x+radius, pTouch->y+radius))
          pTestKey->SetDirty();
      }
    }
//      ClearHitIfMovedOffkey(idx, pKey);
//
//      for(int i=0;i<mKeyControls.GetSize();i++)
//      {
//        KeyControl* pTestKey = mKeyControls.Get(i);
//
//        if(pTestKey->IsHit(pTouch->x, pTouch->y))
//        {
//          SetHit(idx, pTestKey);
//        }
//      }
//    }
  }
  
  void OnMidi(const IMidiMsg& msg) override
  {
    switch (msg.StatusMsg())
    {
      case IMidiMsg::kNoteOn:
        SetNote(msg.NoteNumber(), (msg.Velocity() != 0));
        break;
      case IMidiMsg::kNoteOff:
        SetNote(msg.NoteNumber(), false);
        break;
      case IMidiMsg::kControlChange:
        if(msg.ControlChangeIdx() == IMidiMsg::kAllNotesOff)
          ClearNotes();
        break;
      default: break;
    }

    SetDirty(false);
  }
  
  void OnResize() override
  {
    mBackGroundControl->SetTargetAndDrawRECTs(mRECT);
    
    for (int i=0; i<mKeyControls.GetSize(); i++)
    {
      mKeyControls.Get(i)->SetTargetAndDrawRECTs(mRECT.GetGridCell(i, 1, mKeyControls.GetSize()));
    }
  }
  
  /** Sets a note state, if the keyboard contains that note
   * @param note MIDI pitch to toggle
   * @param on If the note should be on */
  void SetNote(int note, bool on)
  {
    //    if (noteNum < mMinNote || noteNum > mMaxNote) return;
    //    SetKeyIsPressed(noteNum - mMinNote, played);
  }
  
  /** Set all notes off */
  void ClearNotes()
  {
    //    memset(mPressedKeys.Get(), 0, mPressedKeys.GetSize() * sizeof(bool));
    //    SetDirty(false);
  }
  
  /** @param min The minimum note the keyboard should display
   * @param max The maximum note the keyboard should display */
  void SetNoteRange(int min, int max)
  {
    if (min < 0 || max < 0)
      return;
    
    if (min < max)
    {
      mMinNote = min;
      mMaxNote = max;
    }
    else
    {
      mMinNote = max;
      mMaxNote = min;
    }
  }
  
  void CreateHighlights()
  {
    IGraphics* pGraphics = GetUI();
    
    for(int i=0;i<10;i++)
    {
      HighlightControl* pNewHightlightControl = new HighlightControl(mRECT.GetCentredInside(10), this, GetRainbow(i%7));
      pNewHightlightControl->Hide(true);
      pGraphics->AttachControl(pNewHightlightControl);
      mHighlightControls.Add(pNewHightlightControl);
    }
  }
  
  /** @param maintainWidth Set true if keys should resize to fit parent bounds */
  void CreateKeys(bool maintainWidth)
  {
    IGraphics* pGraphics = GetUI();
    
    int numWhiteKeys = mMaxNote-mMinNote;
    
    for(int i=0; i< numWhiteKeys;i++)
    {
      KeyControl* pNewKeyControl = new KeyControl(mRECT.GetGridCell(i, 1, numWhiteKeys), i, false, this);
      pGraphics->AttachControl(pNewKeyControl);
      mKeyControls.Add(pNewKeyControl);
    }
  }

  void SendCtrl1(ITouchID touchID, double value)
  {
    auto touch = GetTouchWithIdentifier(touchID);
    
    IMidiMsg msg;
    msg.MakePitchWheelMsg(value, touch->index);
    GetDelegate()->SendMidiMsgFromUI(msg);
  }
  
  void EnableMPE(bool enable)
  {
    mMPEMode = enable;
  }
  
  bool GetMPEEnabled() const
  {
    return mMPEMode;
  }
  
protected:
  bool mMPEMode = false;
  WDL_PtrList<KeyControl> mKeyControls;
  WDL_PtrList<HighlightControl> mHighlightControls;
  IPanelControl* mBackGroundControl;
  bool mKeysAreContiguous = false;
  int mMinNote, mMaxNote;
  ILayerPtr mLayer;
  std::map<ITouchID, KeyControl*> mTouchPrevouslyHit; // assoc array linking touch IDs to KeyControl last hit by that touches x,y
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
