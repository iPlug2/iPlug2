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
 * @brief This file implements the transposrt bar vector control for the APP wrapper of iplug
 */

#include "IControl.h"
#include "IControls.h"
#include <string>

/*
 
 IVTransportControl
 (C) 2019 Saverio Vigni <s.vigni@hor-net.com>
 
 This software is provided 'as-is', without any express or implied
 warranty. In no event will the authors be held liable for any damages
 arising from the use of this software.
 
 Permission is granted to anyone to use this software for any purpose,
 including commercial applications, and to alter it and redistribute it
 freely, subject to the following restrictions:
 
 1. The origin of this software must not be misrepresented; you must not
 claim that you wrote the original software. If you use this software in a
 product, an acknowledgment in the product documentation would be
 appreciated but is not required.
 2. Altered source versions must be plainly marked as such, and must not be
 misrepresented as being the original software.
 3. This notice may not be removed or altered from any source distribution.
 
 */

/**
 * Implements a transport bar for the APP wrapper of IPlug, when activated with
 * the APP_HAS_TRANSPORT_BAR switch in config.h it is automatiacally added
 * to the plugin GUI
 * Transport bar height can be speicifed with APP_TRANSPORT_BAR_HEIGHT, can be
 * styled with standard IVStyle defining APP_TRANSPORT_BAR_STYLE
 * Optionally the transport bar can provide buttons to open and save projects
 * and another one to create a blank initialization state with:
 * APP_TRANSPORT_BAR_NEW_BUTTON, APP_TRANSPORT_BAR_OPEN_BUTTON, APP_TRANSPORT_BAR_SAVE_BUTTON
 *
 * The transport bar uses the following colors in IVStyle:
 * kBG: the bar background color
 * kON: play button background when playing
 * kOFF: play button background when stopped and other buttons background
 * kX2: BPM box background color
 * kFR: buttons and bpm box frame color
 */
class IVTransportControl : public IControl
                         , public IVectorBase
{
public:
  
  enum EMsgTags {
    bpm = 0,
    play,
    open,
    save,
    blank,
    numMsgTags
  };
  
  IVTransportControl(const IRECT& bounds, const IVStyle& style = DEFAULT_STYLE)
  : IControl(bounds)
  , IVectorBase(style)
  {
  
    // bpm width is 2 times height
    mBpmRect = IRECT(bounds.L,bounds.T,bounds.L+bounds.H()*3, bounds.B);
    mBpmRect.ScaleAboutCentre(0.65);
    mBpmRect.Translate(8,0);
    
    // play button like bpm box
    mPlayBtnRect = IRECT(mBpmRect.L, mBpmRect.T, mBpmRect.R*0.8, mBpmRect.B);
    mPlayBtnRect.Translate(mBpmRect.W() + mBpmRect.W()*0.3, 0);
    
    // open, save and new buttons
    mSaveBtnRect = IRECT(mRECT.R, mPlayBtnRect.T, mRECT.R + mPlayBtnRect.W(), mPlayBtnRect.B);
    mSaveBtnRect.Translate(-1*(mPlayBtnRect.W()+8), 0);
    mOpenBtnRect = mSaveBtnRect.GetTranslated(-1*(8+mSaveBtnRect.W()), 0);
    mNewBtnRect = mOpenBtnRect.GetTranslated(-1*(8+mSaveBtnRect.W()), 0);
    
    mStyle.valueText.mAlign = EAlign::Center;
    mStyle.valueText.mVAlign = EVAlign::Middle;
    
    AttachIControl(this, "");
  }
  
  /**
   * Sets the BPM to be displayed in the tempo box of the transport bar
   * @param bpm the double value of the tempo to be displayed
   */
  void SetBPM(double bpm)
  {
    if (mBPMVal != bpm) {
      mBPMVal = bpm;
      SetDirty(false);
    }
  }
  
  /**
   * Toggles on or off the play mode of the transport bar displaying the play
   * arrow or the stop square symbols depending on the playing status
   * @param play true if the transposrt is playing, false if it's not
   */
  void TogglePlay(bool play)
  {
    if (mPlay != play) {
      mPlay = play;
      SetDirty(false);
    }
  }
  
  void OnMouseOver(float x, float y, const IMouseMod& mod) override
  {
    mMouseOverPlay = false;
    mMouseOverNew = false;
    mMouseOverOpen = false;
    mMouseOverSave = false;
    if(mPlayBtnRect.Contains(x,y)) {
      mMouseOverPlay = true;
      SetDirty(false);
    } else if (mNewBtnRect.Contains(x, y)) {
      mMouseOverNew = true;
      SetDirty(false);
    } else if (mOpenBtnRect.Contains(x,y)) {
      mMouseOverOpen = true;
      SetDirty(false);
    } else if(mSaveBtnRect.Contains(x,y)) {
      mMouseOverSave = true;
      SetDirty(false);
    }
  }
  
  /**
   * Handles clicks on the various active area of the transport bar (buttons and
   * tempo box, overrides the standard IControl method
   * @param x The X coordinate of the mouse event
   * @param y The Y coordinate of the mouse event
   * @param mod A struct indicating which modifier keys are held for the event
   */
  void OnMouseUp(float x, float y, const IMouseMod& mod) override
  {
    if (mBpmRect.Contains(x,y)) {
      std::string bpm = std::to_string(mBPMVal).substr(0, std::to_string(mBPMVal).find(".") + 3);
      GetUI()->CreateTextEntry(*this, mStyle.valueText, mBpmRect, bpm.c_str(), EMsgTags::bpm);
    } else if (mPlayBtnRect.Contains(x,y)) {
      TogglePlay(!mPlay);
      GetDelegate()->SendArbitraryMsgFromUI(EMsgTags::play, mTag, sizeof(bool), &mPlay);
#if APP_TRANSPORT_BAR_OPEN_BUTTON
    } else if (mOpenBtnRect.Contains(x,y)) {
      WDL_String fileName;
      GetUI()->PromptForFile(fileName, mPath, EFileAction::Open, "fxp");
      mFileName = fileName;
      GetDelegate()->SendArbitraryMsgFromUI(EMsgTags::open, mTag, sizeof(mFileName.Get()), mFileName.Get());
#endif
#if APP_TRANSPORT_BAR_SAVE_BUTTON
    } else if (mSaveBtnRect.Contains(x,y)) {
      GetUI()->PromptForFile(mFileName, mPath, EFileAction::Save, "fxp");
      GetDelegate()->SendArbitraryMsgFromUI(EMsgTags::save, mTag, sizeof(mFileName.Get()), mFileName.Get());
#endif
#if APP_TRANSPORT_BAR_NEW_BUTTON
    } else if (mNewBtnRect.Contains(x,y)) {
      bool newProject = true;
      GetDelegate()->SendArbitraryMsgFromUI(EMsgTags::blank, mTag, sizeof(bool), &newProject);
#endif
    }
  }

  /**
   * Responds to the changes in the tempo box informing the delegate of the
   * changed tempo
   * @param str A CString with the inputted text
   * @param valIdx An index that indicates which of the controls vals the text completion relates to
   */
  void OnTextEntryCompletion(const char* str, int valIdx) override
  {
    switch (valIdx) {
      case EMsgTags::bpm:
        mBPMVal = std::atoi(str);
        GetDelegate()->SendArbitraryMsgFromUI(EMsgTags::bpm, mTag, sizeof(int), &mBPMVal);
        break;
    }
    SetDirty(false);
  }
  
  /**
   * Draw the transport bar to the graphics context.
   * @param g The graphics context to which the transport bar belongs.
   */
  void Draw(IGraphics& g) override
  {
    
    DrawBackGround(g, mRECT);
    
    g.StartLayer(mRECT);
    
    // draw BPM box
    float cR = GetRoundedCornerRadius(mBpmRect);
    g.FillRoundRect(GetColor(kX2), mBpmRect, cR);
    if(mStyle.drawFrame)
      g.DrawRoundRect(GetColor(kFR), mBpmRect, cR, nullptr, mStyle.frameThickness);
    
    // draw BPM value
    std::string bpm = std::to_string(mBPMVal).substr(0, std::to_string(mBPMVal).find(".") + 3);
    IRECT txtRealSize;
    mStyle.valueText.mSize = mBpmRect.W() * 0.8;
    g.MeasureText(mStyle.valueText, bpm.c_str(), txtRealSize);
    while(txtRealSize.H() >= mBpmRect.H() * 0.7) {
      mStyle.valueText.mSize *= 0.9;
      g.MeasureText(mStyle.valueText, bpm.c_str(), txtRealSize);
    }
    g.DrawText(mStyle.valueText, bpm.c_str(), mBpmRect);
    
    // draw play / stop button
    DrawPressableRectangle(g, mPlayBtnRect, mPlay, mMouseOverPlay);
    IRECT symbolRect = mPlayBtnRect.GetCentredInside(mPlayBtnRect.H()*0.5, mPlayBtnRect.H()*0.5);
    if(mPlay) {
      g.FillRect(mStyle.valueText.mFGColor, symbolRect);
    } else {
      g.FillTriangle(mStyle.valueText.mFGColor, symbolRect.L, symbolRect.T, symbolRect.L, symbolRect.B, symbolRect.R, symbolRect.MH());
    }

#if APP_TRANSPORT_BAR_OPEN_BUTTON
    // draw open button
    DrawPressableRectangle(g, mOpenBtnRect, false, mMouseOverOpen);
    mStyle.valueText.mSize = mOpenBtnRect.W() * 0.8;
    g.MeasureText(mStyle.valueText, "open", txtRealSize);
    while(txtRealSize.H() >= mOpenBtnRect.H() * 0.7) {
      mStyle.valueText.mSize *= 0.9;
      g.MeasureText(mStyle.valueText, "open", txtRealSize);
    }
    g.DrawText(mStyle.valueText, "open", mOpenBtnRect);
#endif
#if APP_TRANSPORT_BAR_SAVE_BUTTON
    // draw save button
    DrawPressableRectangle(g, mSaveBtnRect, false, mMouseOverSave);
    mStyle.valueText.mSize = mSaveBtnRect.W() * 0.8;
    g.MeasureText(mStyle.valueText, "save", txtRealSize);
    while(txtRealSize.H() >= mSaveBtnRect.H() * 0.7) {
      mStyle.valueText.mSize *= 0.9;
      g.MeasureText(mStyle.valueText, "save", txtRealSize);
    }
    g.DrawText(mStyle.valueText, "save", mSaveBtnRect);
#endif
#if APP_TRANSPORT_BAR_NEW_BUTTON
    // draw new button
    DrawPressableRectangle(g, mNewBtnRect, false, mMouseOverNew);
    mStyle.valueText.mSize = mNewBtnRect.W() * 0.8;
    g.MeasureText(mStyle.valueText, "new", txtRealSize);
    while(txtRealSize.H() >= mNewBtnRect.H() * 0.7) {
      mStyle.valueText.mSize *= 0.9;
      g.MeasureText(mStyle.valueText, "new", txtRealSize);
    }
    g.DrawText(mStyle.valueText, "new", mNewBtnRect);
#endif
    
    ILayerPtr buttons = g.EndLayer();
    IBlend blend = IBlend(EBlend::Default);
    if(mGrayed) {
      blend = IBlend(EBlend::Default, GRAYED_ALPHA);
    }
    g.DrawLayer(buttons, &blend);
  }

private:
  
  double mBPMVal = 0.0;
  bool mPlay = false;
  bool mMouseOverPlay = false;
  bool mMouseOverNew = false;
  bool mMouseOverOpen = false;
  bool mMouseOverSave = false;
  IRECT mBpmRect;
  IRECT mPlayBtnRect;
  IRECT mOpenBtnRect;
  IRECT mSaveBtnRect;
  IRECT mNewBtnRect;
  WDL_String mFileName;
  WDL_String mPath;
};
