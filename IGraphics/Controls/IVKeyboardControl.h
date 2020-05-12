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
      SnapToMouse(x, y, EDirection::Vertical, mRECT, EValIDs::kHeight);
      mParent->AddTouch(mod.touchID, x, y, mod.touchRadius);
      mParent->SetHit(mod.touchID, this);
    }
    
    void OnMouseUp(float x, float y, const IMouseMod& mod) override
    {
      mPointerDown = false;

      mParent->ReleaseTouch(mod.touchID);
      mParent->ClearHitIfMovedOffKey(mod.touchID, nullptr);
    }
    
    void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override
    {
      mParent->UpdateTouch(mod.touchID, x, y, mod.touchRadius);
      mParent->HitMoved(mod.touchID, this);
      
      SnapToMouse(x, y, EDirection::Vertical, mRECT, EValIDs::kHeight);
      mParent->SendCtrl1(mod.touchID, GetValue(EValIDs::kHeight));
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
  void ClearHitIfMovedOffKey(ITouchID touchId, KeyControl* pKey)
  {
    auto itr = mTouchPrevouslyHit.find(touchId);
    
    //if we found the touch
    if(itr != mTouchPrevouslyHit.end())
    {
      KeyControl* pPrevKey = itr->second;
      
      if(pPrevKey != pKey)
      {
        pPrevKey->SetValue(0., EValIDs::kGate);
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

//    ClearHitIfMovedOffKey(touchId, pKey);
//
//    for(int i=0;i<mKeyControls.GetSize();i++)
//    {
//      KeyControl* pTestKey = mKeyControls.Get(i);
//
//      if(pTestKey->IsHit(pTouch->x, pTouch->y))
//      {
//        SetHit(touchId, pTestKey);
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
    
    for(int i=0;i<MAX_TOUCHES;i++)
    {
      HighlightControl* pNewHightlightControl = new HighlightControl(mRECT.GetCentredInside(10), this, GetRainbow(i%7));
//      pNewHightlightControl->Hide(true);
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

/** Vectorial "wheel" control for pitchbender/modwheel
* @ingroup IControls */
class IWheelControl : public ISliderControlBase
{
  static constexpr int kSpringAnimationTime = 50;
  static constexpr int kNumRungs = 10;
public:
  static constexpr int kMessageTagSetPitchBendRange = 0;

  /** Create a WheelControl
   * @param bounds The control's bounds
   * @param cc A Midi CC to link, defaults to kNoCC which is interpreted as pitch bend */
  IWheelControl(const IRECT& bounds, IMidiMsg::EControlChangeMsg cc = IMidiMsg::EControlChangeMsg::kNoCC, int initBendRange = 12)
  : ISliderControlBase(bounds, kNoParameter, EDirection::Vertical, DEFAULT_GEARING, 40.f)
  , mCC(cc)
  , mPitchBendRange(initBendRange)
  {
    mMenu.AddItem("1 semitone");
    mMenu.AddItem("2 semitones");
    mMenu.AddItem("Fifth");
    mMenu.AddItem("Octave");

    SetValue(cc == IMidiMsg::EControlChangeMsg::kNoCC ? 0.5 : 0.);
    SetWantsMidi(true);
    SetActionFunction([cc](IControl* pControl){
      IMidiMsg msg;
      if(cc == IMidiMsg::EControlChangeMsg::kNoCC) // pitchbend
        msg.MakePitchWheelMsg((pControl->GetValue() * 2.) - 1.);
      else
        msg.MakeControlChangeMsg(cc, pControl->GetValue());
      
      pControl->GetDelegate()->SendMidiMsgFromUI(msg);
    });
  }
  
  void Draw(IGraphics& g) override
  {
    IRECT handleBounds = mRECT.GetPadded(-10.f);
    const float stepSize = handleBounds.H() / (float) kNumRungs;
    g.FillRoundRect(DEFAULT_SHCOLOR, mRECT.GetPadded(-5.f));
    
    if(!g.CheckLayer(mLayer))
    {
      const IRECT layerRect = handleBounds.GetMidVPadded(handleBounds.H() + stepSize);
      
      if(layerRect.W() > 0 && layerRect.H() > 0)
      {
        g.StartLayer(this, layerRect);
        g.DrawGrid(COLOR_BLACK.WithOpacity(0.5f), layerRect, 0.f, stepSize, nullptr, 2.f);
        mLayer = g.EndLayer();
      }
    }
    
    // NanoVG only has 2 stop gradients
    IRECT r = handleBounds.FracRectVertical(0.5, true);
    g.PathRect(r);
    g.PathFill(IPattern::CreateLinearGradient(r, EDirection::Vertical, {{COLOR_BLACK, 0.f},{COLOR_MID_GRAY, 1.f}}));
    r = handleBounds.FracRectVertical(0.51f, false); // slight overlap
    g.PathRect(r);
    g.PathFill(IPattern::CreateLinearGradient(r, EDirection::Vertical, {{COLOR_MID_GRAY, 0.f},{COLOR_BLACK, 1.f}}));

    const float value = static_cast<float>(GetValue());
    const float y = (handleBounds.H() - (stepSize)) * value;
    const float triangleRamp = std::fabs(value-0.5f) * 2.f;
    
    g.DrawBitmap(mLayer->GetBitmap(), handleBounds, 0, (int) y);
  
    const IRECT cutoutBounds = handleBounds.GetFromBottom(stepSize).GetTranslated(0, -y);
    g.PathRect(cutoutBounds);
    g.PathFill(IPattern::CreateLinearGradient(cutoutBounds, EDirection::Vertical,
    {
      //TODO: this can be improved
      {COLOR_BLACK.WithContrast(iplug::Lerp(0.f, 0.5f, triangleRamp)), 0.f},
      {COLOR_BLACK.WithContrast(iplug::Lerp(0.5f, 0.f, triangleRamp)), 1.f}
    }));
    
    g.DrawVerticalLine(COLOR_BLACK, cutoutBounds, 0.f);
    g.DrawVerticalLine(COLOR_BLACK, cutoutBounds, 1.f);
    g.DrawRect(COLOR_BLACK, handleBounds);
  }
  
  void OnMidi(const IMidiMsg& msg) override
  {
    if(mCC == IMidiMsg::EControlChangeMsg::kNoCC)
    {
      if(msg.StatusMsg() == IMidiMsg::kPitchWheel)
      {
        SetValue((msg.PitchWheel() + 1.) * 0.5);
        SetDirty(false);
      }
    }
    else
    {
      if(msg.ControlChangeIdx() == mCC)
      {
        SetValue(msg.ControlChange(mCC));
        SetDirty(false);
      }
    }
  }
  
  void OnMouseWheel(float x, float y, const IMouseMod &mod, float d) override
  {
    /* NO-OP */
  }

  void OnPopupMenuSelection(IPopupMenu* pSelectedMenu, int) override
  {
    if(pSelectedMenu) 
    {
      switch (pSelectedMenu->GetChosenItemIdx()) 
      {
        case 0: mPitchBendRange = 1; break;
        case 1: mPitchBendRange = 2; break;
        case 2: mPitchBendRange = 7; break;
        case 3:
        default:
          mPitchBendRange = 12; break;
      }
      
      GetDelegate()->SendArbitraryMsgFromUI(kMessageTagSetPitchBendRange, GetTag(), sizeof(int), &mPitchBendRange);
    }
  }
  
  void OnMouseDown(float x, float y, const IMouseMod &mod) override
  {
    if(mod.R && mCC == IMidiMsg::EControlChangeMsg::kNoCC)
    {
      switch (mPitchBendRange) 
      {
        case 1: mMenu.CheckItemAlone(0); break;
        case 2: mMenu.CheckItemAlone(1); break;
        case 7: mMenu.CheckItemAlone(2); break;
        case 12: mMenu.CheckItemAlone(3); break;
        default:
          break;
      }
      
      GetUI()->CreatePopupMenu(*this, mMenu, x, y);
    }
    else
      ISliderControlBase::OnMouseDown(x, y, mod);
  }
  
  void OnMouseUp(float x, float y, const IMouseMod &mod) override
  {
    if(mCC == IMidiMsg::EControlChangeMsg::kNoCC) // pitchbend
    {
      double startValue = GetValue();
      SetAnimation([startValue](IControl* pCaller) {
        pCaller->SetValue(iplug::Lerp(startValue, 0.5, Clip(pCaller->GetAnimationProgress(), 0., 1.)));
        if(pCaller->GetAnimationProgress() > 1.) {
          pCaller->SetDirty(true);
          pCaller->OnEndAnimation();
          return;
        }
      }, kSpringAnimationTime);
    }
    
    ISliderControlBase::OnMouseUp(x, y, mod);
  }
  
private:
  IPopupMenu mMenu;
  int mPitchBendRange;
  IMidiMsg::EControlChangeMsg mCC;
  ILayerPtr mLayer;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
