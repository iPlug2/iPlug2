/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include "IGraphics.h"

#define NANOSVG_IMPLEMENTATION
#pragma warning(disable:4244) // float conversion
#include "nanosvg.h"

#if defined VST3_API
#include "pluginterfaces/base/ustring.h"
#include "IPlugVST3.h"
using VST3_API_BASE = iplug::IPlugVST3;
#elif defined VST3C_API
#include "pluginterfaces/base/ustring.h"
#include "IPlugVST3_Controller.h"
#include "IPlugVST3_View.h"
using VST3_API_BASE = iplug::IPlugVST3Controller;
#endif

#include "IPlugParameter.h"
#include "IPlugPluginBase.h"

#include "IControl.h"
#include "IControls.h"
#include "IGraphicsLiveEdit.h"
#include "IFPSDisplayControl.h"
#include "ICornerResizerControl.h"
#include "IPopupMenuControl.h"
#include "ITextEntryControl.h"
#include "IBubbleControl.h"

using namespace iplug;
using namespace igraphics;

static StaticStorage<APIBitmap> sBitmapCache;
static StaticStorage<SVGHolder> sSVGCache;

IGraphics::IGraphics(IGEditorDelegate& dlg, int w, int h, int fps, float scale)
: mDelegate(&dlg)
, mWidth(w)
, mHeight(h)
, mDrawScale(scale)
, mMinScale(scale / 2)
, mMaxScale(scale * 2)
{
  mFPS = (fps > 0 ? fps : DEFAULT_FPS);
    
  StaticStorage<APIBitmap>::Accessor bitmapStorage(sBitmapCache);
  bitmapStorage.Retain();
  StaticStorage<SVGHolder>::Accessor svgStorage(sSVGCache);
  svgStorage.Retain();
}

IGraphics::~IGraphics()
{
#ifdef IGRAPHICS_IMGUI
  mImGuiRenderer = nullptr;
#endif
  
  // N.B. - the OS levels have destructed, so we can't show/hide the cursor
  // Thus, this prevents a call to a pure virtual in ReleaseMouseCapture
    
  mCursorHidden = false;
  RemoveAllControls();
    
  StaticStorage<APIBitmap>::Accessor bitmapStorage(sBitmapCache);
  bitmapStorage.Release();
  StaticStorage<SVGHolder>::Accessor svgStorage(sSVGCache);
  svgStorage.Release();
}

void IGraphics::SetScreenScale(float scale)
{
  mScreenScale = scale;
  int windowWidth = WindowWidth() * GetPlatformWindowScale();
  int windowHeight = WindowHeight() * GetPlatformWindowScale();
    
  PlatformResize(GetDelegate()->EditorResizeFromUI(windowWidth, windowHeight, true));
  ForAllControls(&IControl::OnRescale);
  SetAllControlsDirty();
  DrawResize();
}

void IGraphics::Resize(int w, int h, float scale, bool needsPlatformResize)
{
  GetDelegate()->ConstrainEditorResize(w, h);
  
  scale = Clip(scale, mMinScale, mMaxScale);
  
  if (w == Width() && h == Height() && scale == GetDrawScale()) return;
  
  //DBGMSG("resize %i, resize %i, scale %f\n", w, h, scale);
  ReleaseMouseCapture();

  mDrawScale = scale;
  mWidth = w;
  mHeight = h;
  
  if (mCornerResizer)
    mCornerResizer->OnRescale();

  int windowWidth = WindowWidth() * GetPlatformWindowScale();
  int windowHeight = WindowHeight() * GetPlatformWindowScale();
    
  PlatformResize(GetDelegate()->EditorResizeFromUI(windowWidth, windowHeight, needsPlatformResize));
  ForAllControls(&IControl::OnResize);
  SetAllControlsDirty();
  DrawResize();
  
  if(mLayoutOnResize)
    GetDelegate()->LayoutUI(this);
}

void IGraphics::SetLayoutOnResize(bool layoutOnResize)
{
  mLayoutOnResize = layoutOnResize;
}

void IGraphics::RemoveControlWithTag(int ctrlTag)
{
  mControls.DeletePtr(GetControlWithTag(ctrlTag));
  mCtrlTags.erase(ctrlTag);
  SetAllControlsDirty();
}

void IGraphics::RemoveControls(int fromIdx)
{
  int idx = NControls()-1;
  while (idx >= fromIdx)
  {
    IControl* pControl = GetControl(idx);
    
    if(ControlIsCaptured(pControl))
      ReleaseMouseCapture();

    if(pControl == mMouseOver)
      ClearMouseOver();

    if(pControl == mInTextEntry)
      mInTextEntry = nullptr;

    if(pControl == mInPopupMenu)
      mInPopupMenu = nullptr;
    
    if(pControl->GetTag() > kNoTag)
      mCtrlTags.erase(pControl->GetTag());
    
    mControls.Delete(idx--, true);
  }
  
  SetAllControlsDirty();
}

void IGraphics::RemoveControl(int idx)
{
  RemoveControl(GetControl(idx));
}

void IGraphics::RemoveControl(IControl* pControl)
{
  if(ControlIsCaptured(pControl))
    ReleaseMouseCapture();
  
  if(pControl == mMouseOver)
    ClearMouseOver();
  
  if(pControl == mInTextEntry)
    mInTextEntry = nullptr;
  
  if(pControl == mInPopupMenu)
    mInPopupMenu = nullptr;
  
  if(pControl->GetTag() > kNoTag)
    mCtrlTags.erase(pControl->GetTag());
  
  mControls.DeletePtr(pControl, true);
  
  SetAllControlsDirty();
}

void IGraphics::RemoveAllControls()
{
  ReleaseMouseCapture();
  ClearMouseOver();

  mPopupControl = nullptr;
  mTextEntryControl = nullptr;
  mCornerResizer = nullptr;
  mPerfDisplay = nullptr;
    
#ifndef NDEBUG
  mLiveEdit = nullptr;
#endif
  
  mBubbleControls.Empty(true);
  
  mCtrlTags.clear();
  mControls.Empty(true);
}

void IGraphics::SetControlPosition(int idx, float x, float y)
{
  IControl* pControl = GetControl(idx);
  pControl->SetPosition(x, y);
  if (!pControl->IsHidden())
    SetAllControlsDirty();
}

void IGraphics::SetControlSize(int idx, float w, float h)
{
  IControl* pControl = GetControl(idx);
  pControl->SetSize(w, h);
  if (!pControl->IsHidden())
    SetAllControlsDirty();
}

void IGraphics::SetControlBounds(int idx, const IRECT& r)
{
  IControl* pControl = GetControl(idx);
  pControl->SetTargetAndDrawRECTs(r);
  if (!pControl->IsHidden())
    SetAllControlsDirty();
}

void IGraphics::SetControlValueAfterTextEdit(const char* str)
{
  if (!mInTextEntry)
    return;
    
  const IParam* pParam = mTextEntryValIdx > kNoValIdx ? mInTextEntry->GetParam(mTextEntryValIdx) : nullptr;

  if (pParam)
  {
    const double v = pParam->StringToValue(str);
    mInTextEntry->SetValueFromUserInput(pParam->ToNormalized(v), mTextEntryValIdx);
  }
  else
  {
    mInTextEntry->OnTextEntryCompletion(str, mTextEntryValIdx);
  }

  mInTextEntry = nullptr;
}

void IGraphics::SetControlValueAfterPopupMenu(IPopupMenu* pMenu)
{
  if (!mInPopupMenu)
    return;
  
  if (mIsContextMenu)
    mInPopupMenu->OnContextSelection(pMenu ? pMenu->GetChosenItemIdx() : -1);
  else
    mInPopupMenu->OnPopupMenuSelection(!pMenu || pMenu->GetChosenItemIdx() == -1 ? nullptr : pMenu, mPopupMenuValIdx);
  
  int nVals = mInPopupMenu->NVals();

  for (int v = 0; v < nVals; v++)
  {
    int paramIdx = mInPopupMenu->GetParamIdx(v);
    
    if (paramIdx > kNoParameter)
    {
      GetDelegate()->EndInformHostOfParamChangeFromUI(paramIdx);
    }
  }
  
  mInPopupMenu = nullptr;
}

void IGraphics::AttachBackground(const char* fileName)
{
  IControl* pBG = new IBitmapControl(0, 0, LoadBitmap(fileName, 1, false), kNoParameter, EBlend::Default);
  pBG->SetDelegate(*GetDelegate());
  mControls.Insert(0, pBG);
}

void IGraphics::AttachSVGBackground(const char* fileName)
{
  IControl* pBG = new ISVGControl(GetBounds(), LoadSVG(fileName), true);
  pBG->SetDelegate(*GetDelegate());
  mControls.Insert(0, pBG);
}

void IGraphics::AttachPanelBackground(const IPattern& color)
{
  IControl* pBG = new IPanelControl(GetBounds(), color);
  pBG->SetDelegate(*GetDelegate());
  mControls.Insert(0, pBG);
}

IControl* IGraphics::AttachControl(IControl* pControl, int ctrlTag, const char* group)
{
  if(ctrlTag > kNoTag)
  {
    auto result = mCtrlTags.insert(std::make_pair(ctrlTag, pControl));
    assert(result.second && "AttachControl failed: ctrl tags must be unique");
    
    if (!result.second)
      return nullptr;
  }
  
  pControl->SetDelegate(*GetDelegate());
  pControl->SetGroup(group);
  mControls.Add(pControl);
    
  pControl->OnAttached();
  return pControl;
}

void IGraphics::AttachCornerResizer(EUIResizerMode sizeMode, bool layoutOnResize, const IColor& color, const IColor& mouseOverColor, const IColor& dragColor, float size)
{
  AttachCornerResizer(new ICornerResizerControl(GetBounds(), size, color, mouseOverColor, dragColor), sizeMode, layoutOnResize);
}

void IGraphics::AttachCornerResizer(ICornerResizerControl* pControl, EUIResizerMode sizeMode, bool layoutOnResize)
{
  assert(!mCornerResizer); // only want one corner resizer

  std::unique_ptr<ICornerResizerControl> control(pControl);
    
  if (!mCornerResizer)
  {
    mCornerResizer.swap(control);
    mGUISizeMode = sizeMode;
    mLayoutOnResize = layoutOnResize;
    mCornerResizer->SetDelegate(*GetDelegate());
  }
}

void IGraphics::AttachBubbleControl(const IText& text)
{
  IBubbleControl* pControl = new IBubbleControl(text);
  AttachBubbleControl(pControl);
}

void IGraphics::AttachBubbleControl(IBubbleControl* pControl)
{
  pControl->SetDelegate(*GetDelegate());
  mBubbleControls.Add(pControl);
}

void IGraphics::AttachPopupMenuControl(const IText& text, const IRECT& bounds)
{
  if (!mPopupControl)
  {
    mPopupControl = std::make_unique<IPopupMenuControl>(kNoParameter, text, IRECT(), bounds);
    mPopupControl->SetDelegate(*GetDelegate());
  }
}

void IGraphics::RemovePopupMenuControl()
{
  mPopupControl = nullptr;
}

void IGraphics::AttachTextEntryControl()
{
  if (!mTextEntryControl)
  {
    mTextEntryControl = std::make_unique<ITextEntryControl>();
    mTextEntryControl->SetDelegate(*GetDelegate());
  }
}

void IGraphics::RemoveTextEntryControl()
{
  mTextEntryControl = nullptr;
}

void IGraphics::ShowBubbleControl(IControl* pCaller, float x, float y, const char* str, EDirection dir, IRECT minimumContentBounds)
{
  assert(mBubbleControls.GetSize() && "No bubble controls attached");
  
  if(MultiTouchEnabled())
  {
    std::vector<ITouchID> touchIDsForCaller;
    GetTouches(pCaller, touchIDsForCaller);
    std::vector<IBubbleControl*> availableBubbleControls;
    int nBubbleControls = mBubbleControls.GetSize();
    
    if(touchIDsForCaller.size() == 1)
    {
      ITouchID touchID = touchIDsForCaller[0];
      
      // first search to see if this touch matches existing bubble controls
      for(int i=0;i<nBubbleControls;i++)
      {
        IBubbleControl* pBubbleControl = mBubbleControls.Get(i);
        if(pBubbleControl->GetTouchID() == touchID)
        {
          pBubbleControl->ShowBubble(pCaller, x, y, str, dir, minimumContentBounds, touchID);
          return;
        }
        else
          availableBubbleControls.push_back(pBubbleControl);
      }

      if(availableBubbleControls.size())
      {
        // this works but why?
        static int whichBubbleControl = 0;
        availableBubbleControls[whichBubbleControl++]->ShowBubble(pCaller, x, y, str, dir, minimumContentBounds, touchID);
        whichBubbleControl %= nBubbleControls;
      }
    }
//    else
//    {
//      assert(0 && "multi-touch controls with bubble controls not yet supported!");
//    }
  }
  else
    mBubbleControls.Get(0)->ShowBubble(pCaller, x, y, str, dir, minimumContentBounds);
}

void IGraphics::ShowFPSDisplay(bool enable)
{
  if (enable)
  {
    if (!mPerfDisplay)
    {
      mPerfDisplay = std::make_unique<IFPSDisplayControl>(GetBounds().GetPadded(-10).GetFromTLHC(200, 50));
      mPerfDisplay->SetDelegate(*GetDelegate());
    }
  }
  else
  {
    mPerfDisplay = nullptr;
    ClearMouseOver();
  }

  SetAllControlsDirty();
}

IControl* IGraphics::GetControlWithTag(int ctrlTag) const
{
  const auto it = mCtrlTags.find(ctrlTag);

  if (it != mCtrlTags.end())
  {
    return it->second;
  }
  else
  {
    assert("There is no control attached with this tag");
    return nullptr;
  }
}

void IGraphics::HideControl(int paramIdx, bool hide)
{
  ForMatchingControls(&IControl::Hide, paramIdx, hide);
}

void IGraphics::DisableControl(int paramIdx, bool disable)
{
  ForMatchingControls(&IControl::SetDisabled, paramIdx, disable);
}

void IGraphics::ForControlWithParam(int paramIdx, std::function<void(IControl& control)> func)
{
  for (auto c = 0; c < NControls(); c++)
  {
    IControl* pControl = GetControl(c);

    if (pControl->LinkedToParam(paramIdx) > kNoValIdx)
    {
      func(*pControl);
      // Could be more than one, don't break until we check them all.
    }
  }
}

void IGraphics::ForControlInGroup(const char* group, std::function<void(IControl& control)> func)
{
  for (auto c = 0; c < NControls(); c++)
  {
    IControl* pControl = GetControl(c);

    if (CStringHasContents(pControl->GetGroup()))
    {
      if (strcmp(pControl->GetGroup(), group) == 0)
        func(*pControl);
      // Could be more than one, don't break until we check them all.
    }
  }
}

void IGraphics::ForStandardControlsFunc(std::function<void(IControl& control)> func)
{
  for (auto c = 0; c < NControls(); c++)
    func(*GetControl(c));
}

void IGraphics::ForAllControlsFunc(std::function<void(IControl& control)> func)
{
  ForStandardControlsFunc(func);
  
  if (mPerfDisplay)
    func(*mPerfDisplay);
  
#ifndef NDEBUG
  if (mLiveEdit)
    func(*mLiveEdit);
#endif
  
  if (mCornerResizer)
    func(*mCornerResizer);
  
  if (mTextEntryControl)
    func(*mTextEntryControl);
  
  if (mPopupControl)
    func(*mPopupControl);
  
  if (mBubbleControls.GetSize())
  {
    for(int i = 0;i<mBubbleControls.GetSize();i++)
    {
      func(*mBubbleControls.Get(i));
    }
  }
}

template<typename T, typename... Args>
void IGraphics::ForAllControls(T method, Args... args)
{
  ForAllControlsFunc([method, args...](IControl& control) { (control.*method)(args...); });
}

template<typename T, typename... Args>
void IGraphics::ForMatchingControls(T method, int paramIdx, Args... args)
{
  ForControlWithParam(paramIdx, [method, args...](IControl& control) { (control.*method)(args...); });
}

void IGraphics::SetAllControlsDirty()
{
  ForAllControls(&IControl::SetDirty, false, -1);
}

void IGraphics::SetAllControlsClean()
{
  ForAllControls(&IControl::SetClean);
}

void IGraphics::AssignParamNameToolTips()
{
  auto func = [](IControl& control)
  {
    if (control.GetParamIdx() > kNoParameter)
      control.SetTooltip(control.GetParam()->GetName());
  };
  
  ForStandardControlsFunc(func);
}

void IGraphics::UpdatePeers(IControl* pCaller, int callerValIdx) // TODO: this could be really slow
{
  double value = pCaller->GetValue(callerValIdx);
  int paramIdx = pCaller->GetParamIdx(callerValIdx);
    
  auto func = [pCaller, paramIdx, value](IControl& control)
  {
    int valIdx = control.LinkedToParam(paramIdx);

    // Not actually called from the delegate, but we don't want to push the updates back to the delegate
    if ((valIdx > kNoValIdx) && (&control != pCaller))
    {
      control.SetValueFromDelegate(value, valIdx);
    }
  };
    
  ForStandardControlsFunc(func);
}

void IGraphics::PromptUserInput(IControl& control, const IRECT& bounds, int valIdx)
{
  assert(valIdx > kNoValIdx);
  
  const IParam* pParam = control.GetParam(valIdx);

  if(pParam)
  {
    IParam::EParamType type = pParam->Type();
    const int nDisplayTexts = pParam->NDisplayTexts();
    WDL_String currentText;

    if ( type == IParam::kTypeEnum || (type == IParam::kTypeBool && nDisplayTexts))
    {
      pParam->GetDisplay(currentText);
      mPromptPopupMenu.Clear();

      // Fill the menu
      for (int i = 0; i < nDisplayTexts; ++i)
      {
        const char* str = pParam->GetDisplayText(i);
        // TODO: what if two parameters have the same text?
        if (!strcmp(str, currentText.Get())) // strings are equal
          mPromptPopupMenu.AddItem( new IPopupMenu::Item(str, IPopupMenu::Item::kChecked), -1 );
        else // not equal
          mPromptPopupMenu.AddItem( new IPopupMenu::Item(str), -1 );
        
        mPromptPopupMenu.SetRootTitle(pParam->GetName());
      }

      CreatePopupMenu(control, mPromptPopupMenu, bounds, valIdx);
    }
    // TODO: what if there are Int/Double Params with a display text e.g. -96db = "mute"
    else // type == IParam::kTypeInt || type == IParam::kTypeDouble
    {
      pParam->GetDisplay(currentText, false);
      
      if(control.GetPromptShowsParamLabel())
      {
        currentText.Append(" ");
        currentText.Append(pParam->GetLabel());
      }
      
      CreateTextEntry(control, control.GetText(), bounds, currentText.Get(), valIdx);
    }
  }
}

void IGraphics::DrawText(const IText& text, const char* str, const IRECT& bounds, const IBlend* pBlend)
{
  if (!str || str[0] == '\0')
    return;
    
  DoDrawText(text, str, bounds, pBlend);
}

float IGraphics::MeasureText(const IText& text, const char* str, IRECT& bounds) const
{
  if (!str || str[0] == '\0')
    return 0.f;
    
  return DoMeasureText(text, str, bounds);
}

void IGraphics::DrawText(const IText& text, const char* str, float x, float y, const IBlend* pBlend)
{
  IRECT bounds = { x, y, x, y };
  DrawText(text, str, bounds, pBlend);
}

void IGraphics::DrawBitmap(const IBitmap& bitmap, const IRECT& bounds, int bmpState, const IBlend* pBlend)
{
  int srcX = 0;
  int srcY = 0;

  bmpState = Clip(bmpState, 1, bitmap.N());

  if (bitmap.N() > 1 && bmpState > 1)
  {
    if (bitmap.GetFramesAreHorizontal())
    {
      srcX = bitmap.W() * (bmpState - 1) / bitmap.N();
    }
    else
    {
      srcY = bitmap.H() * (bmpState - 1) / bitmap.N();
    }
  }

  return DrawBitmap(bitmap, bounds, srcX, srcY, pBlend);
}

void IGraphics::DrawBitmapedText(const IBitmap& bitmap, const IRECT& bounds, IText& text, IBlend* pBlend, const char* str, bool vCenter, bool multiline, int charWidth, int charHeight, int charOffset)
{
  if (CStringHasContents(str))
  {
    int stringLength = (int) strlen(str);

    float basicYOffset = 0.;
    float basicXOffset = 0.;

    if (vCenter)
      basicYOffset = bounds.T + ((bounds.H() - charHeight) / 2.f);
    else
      basicYOffset = bounds.T;

    if (text.mAlign == EAlign::Center)
      basicXOffset = bounds.L + ((bounds.W() - (stringLength * charWidth)) / 2.f);
    else if (text.mAlign == EAlign::Near)
      basicXOffset = bounds.L;
    else if (text.mAlign == EAlign::Far)
      basicXOffset = bounds.R - (stringLength * charWidth);

    int widthAsOneLine = charWidth * stringLength;

    int nLines;
    int stridx = 0;

    int nCharsThatFitIntoLine;

    if(multiline)
    {
      if (widthAsOneLine > bounds.W())
      {
        nCharsThatFitIntoLine = int(bounds.W() / (float)charWidth);
        nLines = int(float(widthAsOneLine) / bounds.W()) + 1;
      }
      else // line is shorter than width of bounds
      {
        nCharsThatFitIntoLine = stringLength;
        nLines = 1;
      }
    }
    else
    {
      nCharsThatFitIntoLine = int(bounds.W() / (float) charWidth);
      nLines = 1;
    }

    for (int line=0; line<nLines; line++)
    {
      float yOffset = basicYOffset + line * charHeight;

      for (int linepos=0; linepos<nCharsThatFitIntoLine; linepos++)
      {
        if (str[stridx] == '\0') return;

        int frameOffset = (int) str[stridx++] - 31; // calculate which frame to look up

        float xOffset = ((float) linepos * ((float) charWidth + (float) charOffset)) + basicXOffset;    // calculate xOffset for character we're drawing
        IRECT charRect = IRECT(xOffset, yOffset, xOffset + charWidth, yOffset + charHeight);
        DrawBitmap(bitmap, charRect, frameOffset, pBlend);
      }
    }
  }
}

void IGraphics::DrawVerticalLine(const IColor& color, const IRECT& bounds, float x, const IBlend* pBlend, float thickness)
{
  x = Clip(x, 0.0f, 1.0f);
  float xi = bounds.L + int(x * (bounds.R - bounds.L));
  return DrawVerticalLine(color, xi, bounds.T, bounds.B, pBlend, thickness);
}

void IGraphics::DrawHorizontalLine(const IColor& color, const IRECT& bounds, float y, const IBlend* pBlend, float thickness)
{
  y = Clip(y, 0.0f, 1.0f);
  float yi = bounds.B - (y * (float) (bounds.B - bounds.T));
  return DrawHorizontalLine(color, yi, bounds.L, bounds.R, pBlend, thickness);
}

void IGraphics::DrawVerticalLine(const IColor& color, float xi, float yLo, float yHi, const IBlend* pBlend, float thickness)
{
  DrawLine(color, xi, yLo, xi, yHi, pBlend, thickness);
}

void IGraphics::DrawHorizontalLine(const IColor& color, float yi, float xLo, float xHi, const IBlend* pBlend, float thickness)
{
  DrawLine(color, xLo, yi, xHi, yi, pBlend, thickness);
}

void IGraphics::DrawRadialLine(const IColor& color, float cx, float cy, float angle, float rMin, float rMax, const IBlend* pBlend, float thickness)
{
  float data[2][2];
  RadialPoints(angle, cx, cy, rMin, rMax, 2, data);
  DrawLine(color, data[0][0], data[0][1], data[1][0], data[1][1], pBlend, thickness);
}

void IGraphics::PathRadialLine(float cx, float cy, float angle, float rMin, float rMax)
{
  float data[2][2];
  RadialPoints(angle, cx, cy, rMin, rMax, 2, data);
  PathLine(data[0][0], data[0][1], data[1][0], data[1][1]);
}

bool IGraphics::IsDirty(IRECTList& rects)
{
  if (mDisplayTickFunc)
    mDisplayTickFunc();

  ForAllControlsFunc([](IControl& control) { control.Animate(); } );

  bool dirty = false;
    
  auto func = [&dirty, &rects](IControl& control)
  {
    if (control.IsDirty())
    {
      // N.B padding outlines for single line outlines
      rects.Add(control.GetRECT().GetPadded(0.75));
      dirty = true;
    }
  };
    
  ForAllControlsFunc(func);

#ifdef USE_IDLE_CALLS
  if (dirty)
  {
    mIdleTicks = 0;
  }
  else if (++mIdleTicks > IDLE_TICKS)
  {
    OnGUIIdle();
    mIdleTicks = 0;
  }
#endif

#if defined IGRAPHICS_IMGUI
#if defined IGRAPHICS_GL || defined IGRAPHICS_SKIA && !defined IGRAPHICS_CPU
  if (mImGuiRenderer && mImGuiRenderer->GetDrawFunc())
  {
    rects.Add(IRECT(0,0,1,1));
    return true;
  }
#endif
#endif

  return dirty;
}

void IGraphics::BeginFrame()
{
  if(mPerfDisplay)
  {
    const double timestamp = GetTimestamp();
    const double timeDiff = timestamp - mPrevTimestamp;
    mPerfDisplay->Update((float) timeDiff);
    mPrevTimestamp = timestamp;
  }
}

// Draw a control in a region if it needs to be drawn
void IGraphics::DrawControl(IControl* pControl, const IRECT& bounds, float scale)
{
  if (pControl && (!pControl->IsHidden() || pControl == GetControl(0)))
  {
    // N.B. Padding allows single line outlines on controls
    IRECT controlBounds = pControl->GetRECT().GetPadded(0.75).GetPixelAligned(scale);
    IRECT clipBounds = bounds.Intersect(controlBounds);

    if (clipBounds.W() <= 0.0 || clipBounds.H() <= 0)
      return;
    
    PrepareRegion(clipBounds);
    pControl->Draw(*this);
#ifdef AAX_API
    pControl->DrawPTHighlight(*this);
#endif

#ifndef NDEBUG
    if (mShowControlBounds)
    {
      PrepareRegion(clipBounds);
      DrawRect(CONTROL_BOUNDS_COLOR, pControl->GetRECT());
    }
#endif
    
    CompleteRegion(clipBounds);
  }
}

void IGraphics::Draw(const IRECT& bounds, float scale)
{
  ForAllControlsFunc([this, bounds, scale](IControl& control) { DrawControl(&control, bounds, scale); });

#ifndef NDEBUG
  if (mShowAreaDrawn)
  {
    PrepareRegion(bounds);
    static IColor c;
    c.Randomise(50);
    FillRect(c, bounds);
    CompleteRegion(bounds);
  }
#endif
}

void IGraphics::Draw(IRECTList& rects)
{
  if (!rects.Size())
    return;
  
  float scale = GetBackingPixelScale();
    
  BeginFrame();
    
  if (mStrict)
  {
    IRECT r = rects.Bounds();
    r.PixelAlign(scale);
    Draw(r, scale);
  }
  else
  {
    rects.PixelAlign(scale);
    rects.Optimize();

    for (auto i = 0; i < rects.Size(); i++)
      Draw(rects.Get(i), scale);
  }
  
  EndFrame();
}

void IGraphics::SetStrictDrawing(bool strict)
{
  mStrict = strict;
  SetAllControlsDirty();
}

void IGraphics::OnMouseDown(const std::vector<IMouseInfo>& points)
{
//  Trace("IGraphics::OnMouseDown", __LINE__, "x:%0.2f, y:%0.2f, mod:LRSCA: %i%i%i%i%i", x, y, mod.L, mod.R, mod.S, mod.C, mod.A);

  bool singlePoint = points.size() == 1;
  
#ifdef IGRAPHICS_IMGUI
  if(mImGuiRenderer && singlePoint)
  {
    IControl* pControl = GetMouseControl(points[0].x, points[0].y, true);

    bool cornerResizer = false;
    if(mCornerResizer.get() != nullptr)
      cornerResizer = pControl == mCornerResizer.get();

    if(!cornerResizer && mImGuiRenderer->OnMouseDown(points[0].x, points[0].y, points[0].ms))
    {
      ReleaseMouseCapture();
      return;
    }
  }
#endif

  if(singlePoint)
  {
    mMouseDownX = points[0].x;
    mMouseDownY = points[0].y;
  }

  for (auto& point : points)
  {
    float x = point.x;
    float y = point.y;
    const IMouseMod& mod = point.ms;
    
    IControl* pCapturedControl = GetMouseControl(x, y, true, false, mod.touchID);
    
    if (pCapturedControl)
    {
      int nVals = pCapturedControl->NVals();
      int valIdx = pCapturedControl->GetValIdxForPos(x, y);
      int paramIdx = pCapturedControl->GetParamIdx((valIdx > kNoValIdx) ? valIdx : 0);

#ifdef AAX_API
        if (mAAXViewContainer && paramIdx > kNoParameter)
        {
            auto GetAAXModifiersFromIMouseMod = [](const IMouseMod& mod) {
                uint32_t modifiers = 0;
                
                if (mod.A) modifiers |= AAX_eModifiers_Option; // ALT Key on Windows, ALT/Option key on mac
                
#ifdef OS_WIN
                if (mod.C) modifiers |= AAX_eModifiers_Command;
#else
                if (mod.C) modifiers |= AAX_eModifiers_Control;
                if (mod.R) modifiers |= AAX_eModifiers_Command;
#endif
                if (mod.S) modifiers |= AAX_eModifiers_Shift;
                if (mod.R) modifiers |= AAX_eModifiers_SecondaryButton;
                
                return modifiers;
            };
            
            uint32_t aaxModifiersForPT = GetAAXModifiersFromIMouseMod(mod);
#ifdef OS_WIN
            // required to get start/windows and alt keys
            uint32_t aaxModifiersFromPT = 0;
            mAAXViewContainer->GetModifiers(&aaxModifiersFromPT);
            aaxModifiersForPT |= aaxModifiersFromPT;
#endif
            WDL_String paramID;
            paramID.SetFormatted(32, "%i", paramIdx+1);
            
            if (mAAXViewContainer->HandleParameterMouseDown(paramID.Get(), aaxModifiersForPT) == AAX_SUCCESS)
            {
                return; // event handled by PT
            }
        }
#endif

#ifndef IGRAPHICS_NO_CONTEXT_MENU
      if (mod.R && paramIdx > kNoParameter)
      {
        ReleaseMouseCapture();
        PopupHostContextMenuForParam(pCapturedControl, paramIdx, x, y);
        return;
      }
#endif

      for (int v = 0; v < nVals; v++)
      {
        if (pCapturedControl->GetParamIdx(v) > kNoParameter)
          GetDelegate()->BeginInformHostOfParamChangeFromUI(pCapturedControl->GetParamIdx(v));
      }

      pCapturedControl->OnMouseDown(x, y, mod);
    }
  }
}

void IGraphics::OnMouseUp(const std::vector<IMouseInfo>& points)
{
//  Trace("IGraphics::OnMouseUp", __LINE__, "x:%0.2f, y:%0.2f, mod:LRSCA: %i%i%i%i%i", x, y, mod.L, mod.R, mod.S, mod.C, mod.A);
  
  if (ControlIsCaptured())
  {
    for (auto& point : points)
    {
      float x = point.x;
      float y = point.y;
      const IMouseMod& mod = point.ms;
      auto itr = mCapturedMap.find(mod.touchID);
      
      if(itr != mCapturedMap.end())
      {
        IControl* pCapturedControl = itr->second;
      
        pCapturedControl->OnMouseUp(x, y, mod);
      
        int nVals = pCapturedControl->NVals();

        for (int v = 0; v < nVals; v++)
        {
          if (pCapturedControl->GetParamIdx(v) > kNoParameter)
            GetDelegate()->EndInformHostOfParamChangeFromUI(pCapturedControl->GetParamIdx(v));
        }
        
        mCapturedMap.erase(itr);
      }
    }
  }

  if (mResizingInProcess)
  {
    EndDragResize();
  }
  
#ifdef IGRAPHICS_IMGUI
  if(mImGuiRenderer && points.size() == 1)
  {
    if(mImGuiRenderer->OnMouseUp(points[0].x, points[0].y, points[0].ms))
    {
      ReleaseMouseCapture();
      return;
    }
  }
#endif
    
  if (points.size() == 1 && !points[0].ms.IsTouch())
    OnMouseOver(points[0].x, points[0].y, points[0].ms);
}

void IGraphics::OnTouchCancelled(const std::vector<IMouseInfo>& points)
{
  if (ControlIsCaptured())
  {
    //work out which of mCapturedMap controls the cancel relates to
    for (auto& point : points)
    {
      float x = point.x;
      float y = point.y;
      const IMouseMod& mod = point.ms;
      
      auto itr = mCapturedMap.find(mod.touchID);
      
      if(itr != mCapturedMap.end())
      {
        IControl* pCapturedControl = itr->second;
        pCapturedControl->OnTouchCancelled(x, y, mod);
        mCapturedMap.erase(mod.touchID); // remove from captured list
        
        //        DBGMSG("DEL - NCONTROLS captured = %lu\n", mCapturedMap.size());
      }
    }
  }
}

bool IGraphics::OnMouseOver(float x, float y, const IMouseMod& mod)
{
  Trace("IGraphics::OnMouseOver", __LINE__, "x:%0.2f, y:%0.2f, mod:LRSCA: %i%i%i%i%i",
        x, y, mod.L, mod.R, mod.S, mod.C, mod.A);
  
#ifdef IGRAPHICS_IMGUI
  if(mImGuiRenderer)
    mImGuiRenderer->OnMouseMove(x, y, mod);
#endif
  
  // N.B. GetMouseControl handles which controls can receive mouseovers
  IControl* pControl = GetMouseControl(x, y, false, true);
    
  if (pControl != mMouseOver)
  {
    if (mMouseOver)
      mMouseOver->OnMouseOut();
    
    mMouseOver = pControl;
  }

  if (mMouseOver)
    mMouseOver->OnMouseOver(x, y, mod);

  return pControl;
}

void IGraphics::OnMouseOut()
{
  Trace("IGraphics::OnMouseOut", __LINE__, "");

  // Store the old cursor type so this gets restored when the mouse enters again
  mCursorType = SetMouseCursor(ECursor::ARROW);
  ForAllControls(&IControl::OnMouseOut);
  ClearMouseOver();
}

void IGraphics::OnMouseDrag(const std::vector<IMouseInfo>& points)
{
  Trace("IGraphics::OnMouseDrag:", __LINE__, "x:%0.2f, y:%0.2f, dX:%0.2f, dY:%0.2f, mod:LRSCA: %i%i%i%i%i",
        points[0].x, points[0].y, points[0].dX, points[0].dY, points[0].ms.L, points[0].ms.R, points[0].ms.S, points[0].ms.C, points[0].ms.A);

  if (mResizingInProcess && points.size() == 1)
    OnDragResize(points[0].x, points[0].y);
  else if (ControlIsCaptured() && !IsInPlatformTextEntry())
  {
    IControl *textEntry = nullptr;
      
    if (GetControlInTextEntry())
      textEntry = mTextEntryControl.get();
      
    for (auto& point : points)
    {
      float x = point.x;
      float y = point.y;
      float dX = point.dX;
      float dY = point.dY;
      IMouseMod mod = point.ms;
      
      auto itr = mCapturedMap.find(mod.touchID);
      
      if (itr != mCapturedMap.end())
      {
        IControl* pCapturedControl = itr->second;

        if (textEntry && pCapturedControl != textEntry)
            pCapturedControl = nullptr;
          
        if (pCapturedControl && (dX != 0 || dY != 0))
        {
          pCapturedControl->OnMouseDrag(x, y, dX, dY, mod);
        }
      }
    }
  }
#ifdef IGRAPHICS_IMGUI
  else if(mImGuiRenderer && points.size() == 1)
    mImGuiRenderer->OnMouseMove(points[0].x, points[0].y, points[0].ms);
#endif
}

bool IGraphics::OnMouseDblClick(float x, float y, const IMouseMod& mod)
{
  Trace("IGraphics::OnMouseDblClick", __LINE__, "x:%0.2f, y:%0.2f, mod:LRSCA: %i%i%i%i%i",
        x, y, mod.L, mod.R, mod.S, mod.C, mod.A);
  
#ifdef IGRAPHICS_IMGUI
  if(mImGuiRenderer)
  {
    mImGuiRenderer->OnMouseDown(x, y, mod);
    return true;
  }
#endif

  IControl* pControl = GetMouseControl(x, y, true);
    
  if (pControl)
  {
    if (pControl->GetMouseDblAsSingleClick())
    {
      IMouseInfo info;
      info.x = x;
      info.y = y;
      info.ms = mod;
      std::vector<IMouseInfo> list {info};
      OnMouseDown(list);
    }
    else
    {
      pControl->OnMouseDblClick(x, y, mod);
      ReleaseMouseCapture();
    }
  }
    
  return pControl;
}

void IGraphics::OnMouseWheel(float x, float y, const IMouseMod& mod, float d)
{
#ifdef IGRAPHICS_IMGUI
    if(mImGuiRenderer)
    {
      mImGuiRenderer->OnMouseWheel(x, y, mod, d);
      return;
    }
#endif
  
  IControl* pControl = GetMouseControl(x, y, false);
  if (pControl)
    pControl->OnMouseWheel(x, y, mod, d);
}

bool IGraphics::OnKeyDown(float x, float y, const IKeyPress& key)
{
  Trace("IGraphics::OnKeyDown", __LINE__, "x:%0.2f, y:%0.2f, key:%s",
        x, y, key.utf8);

  bool handled = false;

#ifdef IGRAPHICS_IMGUI
  if(mImGuiRenderer)
  {
    handled = mImGuiRenderer->OnKeyDown(x, y, key);
    
    if(handled)
      return true;
  }
#endif
  
  IControl* pControl = GetMouseControl(x, y, false);
  
  if (pControl && pControl != GetControl(0))
    handled = pControl->OnKeyDown(x, y, key);

  if(!handled)
    handled = mKeyHandlerFunc ? mKeyHandlerFunc(key, false) : false;
  
  return handled;
}

bool IGraphics::OnKeyUp(float x, float y, const IKeyPress& key)
{
  Trace("IGraphics::OnKeyUp", __LINE__, "x:%0.2f, y:%0.2f, key:%s",
        x, y, key.utf8);
  
  bool handled = false;
  
#ifdef IGRAPHICS_IMGUI
  if(mImGuiRenderer)
  {
    handled = mImGuiRenderer->OnKeyUp(x, y, key);
    
    if(handled)
      return true;
  }
#endif
  
  IControl* pControl = GetMouseControl(x, y, false);
  
  if (pControl && pControl != GetControl(0))
    handled = pControl->OnKeyUp(x, y, key);
  
  if(!handled)
    handled = mKeyHandlerFunc ? mKeyHandlerFunc(key, true) : false;
  
  return handled;
}

void IGraphics::OnDrop(const char* str, float x, float y)
{
  IControl* pControl = GetMouseControl(x, y, false);
  if (pControl) pControl->OnDrop(str);
}

void IGraphics::ReleaseMouseCapture()
{
  mCapturedMap.clear();
  if (mCursorHidden)
    HideMouseCursor(false);
}

int IGraphics::GetMouseControlIdx(float x, float y, bool mouseOver)
{
  if (!mouseOver || mEnableMouseOver)
  {
    // Search from front to back
    for (auto c = NControls() - 1; c >= (mouseOver ? 1 : 0); --c)
    {
      IControl* pControl = GetControl(c);

#ifndef NDEBUG
      if(!mLiveEdit)
      {
#endif
        if (!pControl->IsHidden() && !pControl->GetIgnoreMouse())
        {
          if ((!pControl->IsDisabled() || (mouseOver ? pControl->GetMouseOverWhenDisabled() : pControl->GetMouseEventsWhenDisabled())))
          {
            if (pControl->IsHit(x, y))
            {
              return c;
            }
          }
        }
#ifndef NDEBUG
      }
      else if (pControl->GetRECT().Contains(x, y))
      {
        return c;
      }
#endif
    }
  }
  
  return -1;
}

IControl* IGraphics::GetMouseControl(float x, float y, bool capture, bool mouseOver, ITouchID touchID)
{
  IControl* pControl = nullptr;

  auto itr = mCapturedMap.find(touchID);
  
  if(ControlIsCaptured() && itr != mCapturedMap.end())
  {
    pControl = itr->second;
    
    if(pControl)
      return pControl;
  }
  
  int controlIdx = -1;
  
  if (!pControl && mPopupControl && mPopupControl->GetExpanded())
    pControl = mPopupControl.get();
  
  if (!pControl && mTextEntryControl && mTextEntryControl->EditInProgress())
    pControl = mTextEntryControl.get();
  
#if !defined(NDEBUG)
  if (!pControl && mLiveEdit)
    pControl = mLiveEdit.get();
#endif
  
  if (!pControl && mCornerResizer && mCornerResizer->GetRECT().Contains(x, y))
    pControl = mCornerResizer.get();
  
  if (!pControl && mPerfDisplay && mPerfDisplay->GetRECT().Contains(x, y))
    pControl = mPerfDisplay.get();
  
  if (!pControl)
  {
    controlIdx = GetMouseControlIdx(x, y, mouseOver);
    pControl = (controlIdx >= 0) ? GetControl(controlIdx) : nullptr;
  }
  
  if (capture && pControl)
  {
    if(MultiTouchEnabled())
    {
      bool alreadyCaptured = ControlIsCaptured(pControl);

      if (alreadyCaptured && !pControl->GetWantsMultiTouch())
        return nullptr;
    }
    
    mCapturedMap.insert(std::make_pair(touchID, pControl));
    
//    DBGMSG("ADD - NCONTROLS captured = %lu\n", mCapturedMap.size());
  }
  
  if (mouseOver)
    mMouseOverIdx = controlIdx;
  
  return pControl;
}

int IGraphics::GetParamIdxForPTAutomation(float x, float y)
{
  IControl* pControl = GetMouseControl(x, y, false);
  int idx = mLastClickedParam = pControl ? pControl->GetParamIdx() : -1;
  return idx;
}

int IGraphics::GetLastClickedParamForPTAutomation()
{
  const int idx = mLastClickedParam;
  mLastClickedParam = kNoParameter;
  return idx;
}

void IGraphics::SetPTParameterHighlight(int paramIdx, bool isHighlighted, int color)
{
  ForMatchingControls(&IControl::SetPTParameterHighlight, paramIdx, isHighlighted, color);
}

void IGraphics::PopupHostContextMenuForParam(IControl* pControl, int paramIdx, float x, float y)
{
  IPopupMenu& contextMenu = mPromptPopupMenu;
  contextMenu.Clear();

  if(pControl)
  {
    pControl->CreateContextMenu(contextMenu);

#if defined VST3_API || defined VST3C_API
    VST3_API_BASE* pVST3 = dynamic_cast<VST3_API_BASE*>(GetDelegate());

    if (!pVST3->GetComponentHandler() || !pVST3->GetView())
      return;

    Steinberg::FUnknownPtr<Steinberg::Vst::IComponentHandler3>handler(pVST3->GetComponentHandler() );

    if (handler == 0)
      return;

    Steinberg::Vst::ParamID p = paramIdx;

    Steinberg::Vst::IContextMenu* pVST3ContextMenu = handler->createContextMenu(pVST3->GetView(), &p);

    if (pVST3ContextMenu)
    {
      Steinberg::Vst::IContextMenu::Item item = {0};

      for (int i = 0; i < contextMenu.NItems(); i++)
      {
        Steinberg::UString128 (contextMenu.GetItemText(i)).copyTo (item.name, 128);
        item.tag = i;

        if (!contextMenu.GetItem(i)->GetEnabled())
          item.flags = Steinberg::Vst::IContextMenu::Item::kIsDisabled;
        else
          item.flags = 0;

        pVST3ContextMenu->addItem(item, pControl);
      }
#ifdef OS_WIN
      x *= GetTotalScale();
      y *= GetTotalScale();
#else
      x *= GetDrawScale();
      y *= GetDrawScale();
#endif
      pVST3ContextMenu->popup((Steinberg::UCoord) x, (Steinberg::UCoord) y);
      pVST3ContextMenu->release();
    }

#else
    if(!contextMenu.NItems())
      return;

    DoCreatePopupMenu(*pControl, contextMenu, IRECT(x, y, x, y), kNoValIdx, true);
#endif
  }
}

void IGraphics::PopupHostContextMenuForParam(int controlIdx, int paramIdx, float x, float y)
{
  PopupHostContextMenuForParam(GetControl(controlIdx), paramIdx, x, y);
}

void IGraphics::OnGUIIdle()
{
  TRACE
  ForAllControls(&IControl::OnGUIIdle);
}

void IGraphics::OnDragResize(float x, float y)
{
  if(mGUISizeMode == EUIResizerMode::Scale)
  {
    float scaleX = (x * GetDrawScale()) / mMouseDownX;
    float scaleY = (y * GetDrawScale()) / mMouseDownY;

    Resize(Width(), Height(), std::min(scaleX, scaleY));
  }
  else
  {
    Resize(static_cast<int>(x), static_cast<int>(y), GetDrawScale());
  }
}

IBitmap IGraphics::GetScaledBitmap(IBitmap& src)
{
  //TODO: bug with # frames!
//  return LoadBitmap(src.GetResourceName().Get(), src.N(), src.GetFramesAreHorizontal(), (GetRoundedScreenScale() == 1 && GetDrawScale() > 1.) ? 2 : 0 /* ??? */);
  return LoadBitmap(src.GetResourceName().Get(), src.N(), src.GetFramesAreHorizontal(), GetRoundedScreenScale());
}

void IGraphics::EnableTooltips(bool enable)
{
  mEnableTooltips = enable;
  if (enable) mEnableMouseOver = true;
}

void IGraphics::EnableLiveEdit(bool enable)
{
#ifndef NDEBUG
  if (enable)
  {
    if (!mLiveEdit)
    {
      mLiveEdit = std::make_unique<IGraphicsLiveEdit>(mEnableMouseOver);
      mLiveEdit->SetDelegate(*GetDelegate());
    }
  }
  else
  {
    mLiveEdit = nullptr;
  }
  
  ClearMouseOver();
  ReleaseMouseCapture();
  SetMouseCursor(ECursor::ARROW);
  SetAllControlsDirty();
#endif
}

// Skia has its own implementation for SVGs. On all other platforms we use NanoSVG, because it works.
#ifdef IGRAPHICS_SKIA
ISVG IGraphics::LoadSVG(const char* fileName, const char* units, float dpi)
{
  StaticStorage<SVGHolder>::Accessor storage(sSVGCache);
  SVGHolder* pHolder = storage.Find(fileName);
  
  if(!pHolder)
  {
    WDL_TypedBuf<uint8_t> svgData = LoadResource(fileName, "svg");
    if (svgData.GetSize() == 0)
    {
      return ISVG(nullptr);
    }
    else
    {
      return LoadSVG(fileName, svgData.Get(), svgData.GetSize(), units, dpi);
    }
  }
  
  return ISVG(pHolder->mSVGDom);
}

ISVG IGraphics::LoadSVG(const char* name, const void* pData, int dataSize, const char* units, float dpi)
{
  StaticStorage<SVGHolder>::Accessor storage(sSVGCache);
  SVGHolder* pHolder = storage.Find(name);

  if (!pHolder)
  {
    sk_sp<SkSVGDOM> svgDOM;
    SkDOM xmlDom;

    SkMemoryStream svgStream(pData, dataSize);
    svgDOM = SkSVGDOM::MakeFromStream(svgStream);
    
    if (!svgDOM)
      return ISVG(nullptr); // return invalid SVG

    // If an SVG doesn't have a container size, SKIA doesn't seem to have access to any meaningful size info.
    // So use NanoSVG to get the size.
    if (svgDOM->containerSize().width() == 0)
    {
      NSVGimage* pImage = nullptr;

      WDL_String svgStr;
      svgStr.Set((const char*)pData, dataSize);
      pImage = nsvgParse(svgStr.Get(), units, dpi);
      
      assert(pImage);

      svgDOM->setContainerSize(SkSize::Make(pImage->width, pImage->height));

      nsvgDelete(pImage);
    }

    pHolder = new SVGHolder(svgDOM);
    storage.Add(pHolder, name);
  }

  return ISVG(pHolder->mSVGDom);
}

#else
ISVG IGraphics::LoadSVG(const char* fileName, const char* units, float dpi)
{
  StaticStorage<SVGHolder>::Accessor storage(sSVGCache);
  SVGHolder* pHolder = storage.Find(fileName);

  if(!pHolder)
  {
    WDL_TypedBuf<uint8_t> svgData = LoadResource(fileName, "svg");
    if (svgData.GetSize() == 0)
    {
      return ISVG(nullptr);
    }
    else
    {
      return LoadSVG(fileName, svgData.Get(), svgData.GetSize(), units, dpi);
    }
  }

  return ISVG(pHolder->mImage);
}

ISVG IGraphics::LoadSVG(const char* name, const void* pData, int dataSize, const char* units, float dpi)
{
  StaticStorage<SVGHolder>::Accessor storage(sSVGCache);
  SVGHolder* pHolder = storage.Find(name);

  if (!pHolder)
  {
    NSVGimage* pImage = nullptr;

    // Because we're taking a const void* pData, but NanoSVG takes a void*, 
    WDL_String svgStr;
    svgStr.Set((const char*)pData, dataSize);
    pImage = nsvgParse(svgStr.Get(), units, dpi);

    if (!pImage)
      return ISVG(nullptr);
    
    pHolder = new SVGHolder(pImage);

    storage.Add(pHolder, name);
  }

  return ISVG(pHolder->mImage);
}
#endif

WDL_TypedBuf<uint8_t> IGraphics::LoadResource(const char* fileNameOrResID, const char* fileType)
{
  WDL_TypedBuf<uint8_t> result;

  WDL_String path;
  EResourceLocation resourceFound = LocateResource(fileNameOrResID, fileType, path, GetBundleID(), GetWinModuleHandle(), GetSharedResourcesSubPath());

  if (resourceFound == EResourceLocation::kNotFound)
    return result;

#ifdef OS_WIN    
  if (resourceFound == EResourceLocation::kWinBinary)
  {
    int size = 0;
    const void* pResData = LoadWinResource(path.Get(), fileType, size, GetWinModuleHandle());
    result.Resize(size);
    result.Set((const uint8_t*)pResData, size);
  }
#endif
  if (resourceFound == EResourceLocation::kAbsolutePath)
  {
    FILE* fd = fopen(path.Get(), "rb");
    if (!fd)
      return result;
    
    // First we determine the file size
    if (fseek(fd, 0, SEEK_END))
    {
      fclose(fd);
      return result;
    }
    long size = ftell(fd);

    // Now reset to the start of the file so we can actually read it.
    if (fseek(fd, 0, SEEK_SET))
    {
      fclose(fd);
      return result;
    }

    result.Resize((int)size);
    size_t bytesRead = fread(result.Get(), 1, (size_t)size, fd);
    if (bytesRead != (size_t)size)
    {
      fclose(fd);
      result.Resize(0, true);
      return result;
    }
    fclose(fd);
  }

  return result;
}

IBitmap IGraphics::LoadBitmap(const char* name, int nStates, bool framesAreHorizontal, int targetScale)
{
  if (targetScale == 0)
    targetScale = GetRoundedScreenScale();

  StaticStorage<APIBitmap>::Accessor storage(sBitmapCache);
  APIBitmap* pAPIBitmap = storage.Find(name, targetScale);

  // If the bitmap is not already cached at the targetScale
  if (!pAPIBitmap)
  {
    WDL_String fullPath;
    std::unique_ptr<APIBitmap> loadedBitmap;
    int sourceScale = 0;
    
    const char* ext = name + strlen(name) - 1;
    while (ext >= name && *ext != '.') --ext;
    ++ext;
    
    bool bitmapTypeSupported = BitmapExtSupported(ext);
    
    if (!bitmapTypeSupported)
      return IBitmap(); // return invalid IBitmap

    EResourceLocation resourceLocation = SearchImageResource(name, ext, fullPath, targetScale, sourceScale);

    if (resourceLocation == EResourceLocation::kNotFound)
    {
      // If no resource exists then search the cache for a suitable match
      pAPIBitmap = SearchBitmapInCache(name, targetScale, sourceScale);
    }
    else
    {
      // Try in the cache for a mismatched bitmap
      if (sourceScale != targetScale)
        pAPIBitmap = storage.Find(name, sourceScale);

      // Load the resource if no match found
      if (!pAPIBitmap)
      {
        loadedBitmap = std::unique_ptr<APIBitmap>(LoadAPIBitmap(fullPath.Get(), sourceScale, resourceLocation, ext));
        pAPIBitmap= loadedBitmap.get();
      }
    }

    // Protection from searching for non-existent bitmaps (e.g. typos in config.h or .rc)
    assert(pAPIBitmap && "Bitmap not found");

    // Scale or retain if needed (N.B. - scaling retains in the cache)
    if (pAPIBitmap->GetScale() != targetScale)
    {
      return ScaleBitmap(IBitmap(pAPIBitmap, nStates, framesAreHorizontal, name), name, targetScale);
    }
    else if (loadedBitmap)
    {
      RetainBitmap(IBitmap(loadedBitmap.release(), nStates, framesAreHorizontal, name), name);
    }
  }

  return IBitmap(pAPIBitmap, nStates, framesAreHorizontal, name);
}

IBitmap IGraphics::LoadBitmap(const char *name, const void *pData, int dataSize, int nStates, bool framesAreHorizontal, int targetScale)
{
  if (targetScale == 0)
    targetScale = GetRoundedScreenScale();

  StaticStorage<APIBitmap>::Accessor storage(sBitmapCache);
  APIBitmap* pAPIBitmap = storage.Find(name, targetScale);

  // If the bitmap is not already cached at the targetScale
  if (!pAPIBitmap)
  {
    WDL_String fullPath;
    std::unique_ptr<APIBitmap> loadedBitmap;
    int sourceScale = 0;
    
    const char* ext = name + strlen(name) - 1;
    while (ext >= name && *ext != '.') --ext;
    ++ext;
    
    bool bitmapTypeSupported = BitmapExtSupported(ext);
    
    if (!bitmapTypeSupported)
      return IBitmap(); // return invalid IBitmap

    // Seach the cache for an existing copy, maybe with a different scale
    pAPIBitmap = SearchBitmapInCache(name, targetScale, sourceScale);
    // It's definitely not loaded, so load it with scale = 1.
    if (!pAPIBitmap)
    {
      loadedBitmap = std::unique_ptr<APIBitmap>(LoadAPIBitmap(name, pData, dataSize, 1));
      pAPIBitmap= loadedBitmap.get();
    }

    // Protection from searching for non-existent bitmaps (e.g. typos in config.h or .rc)
    // Also protects from invalid bitmap data.
    assert(pAPIBitmap && "Bitmap not found");

    // Scale or retain if needed (N.B. - scaling retains in the cache)
    if (pAPIBitmap->GetScale() != targetScale)
    {
      return ScaleBitmap(IBitmap(pAPIBitmap, nStates, framesAreHorizontal, name), name, targetScale);
    }
    else if (loadedBitmap)
    {
      RetainBitmap(IBitmap(loadedBitmap.release(), nStates, framesAreHorizontal, name), name);
    }
  }

  return IBitmap(pAPIBitmap, nStates, framesAreHorizontal, name);
}

void IGraphics::ReleaseBitmap(const IBitmap &bitmap)
{
  StaticStorage<APIBitmap>::Accessor storage(sBitmapCache);
  storage.Remove(bitmap.GetAPIBitmap());
}

void IGraphics::RetainBitmap(const IBitmap& bitmap, const char* cacheName)
{
  StaticStorage<APIBitmap>::Accessor storage(sBitmapCache);
  storage.Add(bitmap.GetAPIBitmap(), cacheName, bitmap.GetScale());
}

IBitmap IGraphics::ScaleBitmap(const IBitmap& inBitmap, const char* name, int scale)
{
  int screenScale = GetRoundedScreenScale();
  float drawScale = GetDrawScale();

  mScreenScale = scale;
  mDrawScale = inBitmap.GetDrawScale();

  IRECT bounds = IRECT(0, 0, inBitmap.W() / inBitmap.GetDrawScale(), inBitmap.H() / inBitmap.GetDrawScale());
  StartLayer(nullptr, bounds, true);
  DrawBitmap(inBitmap, bounds, 0, 0, nullptr);
  ILayerPtr layer = EndLayer();
  IBitmap outBitmap = IBitmap(layer->mBitmap.release(), inBitmap.N(), inBitmap.GetFramesAreHorizontal(), name);
  RetainBitmap(outBitmap, name);

  mScreenScale = screenScale;
  mDrawScale = drawScale;
    
  return outBitmap;
}

inline void IGraphics::SearchNextScale(int& sourceScale, int targetScale)
{
  // Search downwards from MAX_IMG_SCALE, skipping targetScale before trying again
  if (sourceScale == targetScale && (targetScale != MAX_IMG_SCALE))
    sourceScale = MAX_IMG_SCALE;
  else if (sourceScale == targetScale + 1)
    sourceScale = targetScale - 1;
  else
    sourceScale--;
}

EResourceLocation IGraphics::SearchImageResource(const char* name, const char* type, WDL_String& result, int targetScale, int& sourceScale)
{
  // Search target scale, then descending
  for (sourceScale = targetScale ; sourceScale > 0; SearchNextScale(sourceScale, targetScale))
  {
    WDL_String fullName(name);
    
    if (sourceScale != 1)
    {
      WDL_String baseName(name); baseName.remove_fileext();
      WDL_String ext(fullName.get_fileext());
      fullName.SetFormatted((int) (strlen(name) + strlen("@2x")), "%s@%dx%s", baseName.Get(), sourceScale, ext.Get());
    }

    EResourceLocation found = LocateResource(fullName.Get(), type, result, GetBundleID(), GetWinModuleHandle(), GetSharedResourcesSubPath());

    if (found > EResourceLocation::kNotFound)
      return found;
  }

  return EResourceLocation::kNotFound;
}

APIBitmap* IGraphics::SearchBitmapInCache(const char* name, int targetScale, int& sourceScale)
{
  StaticStorage<APIBitmap>::Accessor storage(sBitmapCache);
    
  // Search target scale, then descending
  for (sourceScale = targetScale; sourceScale > 0; SearchNextScale(sourceScale, targetScale))
  {
    APIBitmap* pBitmap = storage.Find(name, sourceScale);

    if (pBitmap)
      return pBitmap;
  }

  return nullptr;
}

void IGraphics::StyleAllVectorControls(const IVStyle& style)
{
  for (auto c = 0; c < NControls(); c++)
  {
    IVectorBase* pVB = dynamic_cast<IVectorBase*>(GetControl(c));
    if (pVB)
      pVB->SetStyle(style);
  }
}

void IGraphics::CreateTextEntry(IControl& control, const IText& text, const IRECT& bounds, const char* str, int valIdx)
{
  mInTextEntry = &control;
  mTextEntryValIdx = valIdx;
    
  int paramIdx = valIdx > kNoValIdx  ? control.GetParamIdx(valIdx) : kNoParameter;

  if (mTextEntryControl)
    mTextEntryControl->CreateTextEntry(paramIdx, text, bounds, control.GetTextEntryLength(), str);
  else
    CreatePlatformTextEntry(paramIdx, text, bounds, control.GetTextEntryLength(), str);
  
  mInTextEntry->SetDirty(false);
}

void IGraphics::DoCreatePopupMenu(IControl& control, IPopupMenu& menu, const IRECT& bounds, int valIdx, bool isContext)
{
  ReleaseMouseCapture();
    
  mInPopupMenu = &control;
  mPopupMenuValIdx = valIdx;
  mIsContextMenu = isContext;
  
  if(mPopupControl) // if we are not using platform pop-up menus
  {
    mPopupControl->CreatePopupMenu(menu, bounds);
  }
  else
  {
    bool isAsync = false;
    IPopupMenu* pReturnMenu = CreatePlatformPopupMenu(menu, bounds, isAsync);
    
    if(!isAsync)
      SetControlValueAfterPopupMenu(pReturnMenu);
  }
}

void IGraphics::CreatePopupMenu(IControl& control, IPopupMenu& menu, const IRECT& bounds, int valIdx)
{
  DoCreatePopupMenu(control, menu, bounds, valIdx, false);
}

void IGraphics::EndDragResize()
{
  mResizingInProcess = false;
  
  if (GetResizerMode() == EUIResizerMode::Scale)
  {
    // If scaling up we may want to load in high DPI bitmaps if scale > 1.
    ForAllControls(&IControl::OnRescale);
    SetAllControlsDirty();
  }
}

void IGraphics::StartLayer(IControl* pControl, const IRECT& r, bool cacheable)
{
  auto pixelBackingScale = GetBackingPixelScale();
  IRECT alignedBounds = r.GetPixelAligned(pixelBackingScale);
  const int w = static_cast<int>(std::ceil(pixelBackingScale * std::ceil(alignedBounds.W())));
  const int h = static_cast<int>(std::ceil(pixelBackingScale * std::ceil(alignedBounds.H())));

  PushLayer(new ILayer(CreateAPIBitmap(w, h, GetRoundedScreenScale(), GetDrawScale(), cacheable), alignedBounds, pControl, pControl ? pControl->GetRECT() : IRECT()));
}

void IGraphics::ResumeLayer(ILayerPtr& layer)
{
  ILayerPtr ownedLayer;
    
  ownedLayer.swap(layer);
  ILayer* pOwnerlessLayer = ownedLayer.release();
    
  if (pOwnerlessLayer)
  {
    PushLayer(pOwnerlessLayer);
  }
}

ILayerPtr IGraphics::EndLayer()
{
  return ILayerPtr(PopLayer());
}

void IGraphics::PushLayer(ILayer* pLayer)
{
  mLayers.push(pLayer);
  UpdateLayer();
  PathTransformReset();
  PathClipRegion(pLayer->Bounds());
  PathClear();
}

ILayer* IGraphics::PopLayer()
{
  ILayer* pLayer = nullptr;
  
  if (!mLayers.empty())
  {
    pLayer = mLayers.top();
    mLayers.pop();
  }
  
  UpdateLayer();
  PathTransformReset();
  PathClipRegion();
  PathClear();
  
  return pLayer;
}

bool IGraphics::CheckLayer(const ILayerPtr& layer)
{
  const APIBitmap* pBitmap = layer ? layer->GetAPIBitmap() : nullptr;
    
  if (pBitmap && layer->mControl && layer->mControlRECT != layer->mControl->GetRECT())
  {
    layer->mControlRECT = layer->mControl->GetRECT();
    layer->Invalidate();
  }

  return pBitmap && !layer->mInvalid && pBitmap->GetDrawScale() == GetDrawScale() && pBitmap->GetScale() == GetRoundedScreenScale();
}

void IGraphics::DrawLayer(const ILayerPtr& layer, const IBlend* pBlend)
{
  PathTransformSave();
  PathTransformReset();
  DrawBitmap(layer->GetBitmap(), layer->Bounds(), 0, 0, pBlend);
  PathTransformRestore();
}

void IGraphics::DrawFittedLayer(const ILayerPtr& layer, const IRECT& bounds, const IBlend* pBlend)
{
  IBitmap bitmap = layer->GetBitmap();
  IRECT layerBounds = layer->Bounds();
  PathTransformSave();
  PathTransformTranslate(bounds.L, bounds.T);
  IRECT newBounds(0., 0., layerBounds.W(), layerBounds.H());
  PathTransformScale(bounds.W() / layerBounds.W(), bounds.H() / layerBounds.H());
  DrawBitmap(bitmap, newBounds, 0, 0, pBlend);
  PathTransformRestore();
}

void IGraphics::DrawRotatedLayer(const ILayerPtr& layer, double angle)
{
  PathTransformSave();
  PathTransformReset();
  IBitmap bitmap = layer->GetBitmap();
  IRECT bounds = layer->Bounds();
  DrawRotatedBitmap(bitmap, bounds.MW(), bounds.MH(), angle);
  PathTransformRestore();
}

void IGraphics::ApplyLayerDropShadow(ILayerPtr& layer, const IShadow& shadow)
{
  auto GaussianBlurSwap = [](uint8_t* out, uint8_t* in, uint8_t* kernel, int width, int height,
                             int outStride, int inStride, int kernelSize, uint32_t norm)
  {
    int repeats = 0;
    int fullKernelSize = kernelSize * 2 + 1;
    uint32_t last = 0;

    auto RepeatCheck = [&](int idx)
    {
      repeats = last == in[idx * 4] ? std::min(repeats + 1, fullKernelSize) : 1;
      last = in[idx * 4];
        
      return repeats == fullKernelSize;
    };
      
    for (int i = 0; i < height; i++, in += inStride)
    {
      for (int j = 0; j < kernelSize - 1; j++)
      {
        uint32_t accum = in[j * 4] * kernel[0];
        for (int k = 1; k < j + 1; k++)
          accum += kernel[k] * in[(j - k) * 4];
        for (int k = 1; k < kernelSize; k++)
          accum += kernel[k] * in[(j + k) * 4];
        out[j * outStride + (i * 4)] = static_cast<uint8_t>(std::min(static_cast<uint32_t>(255), accum / norm));
      }
      for (int j = 0; j < kernelSize * 2 - 2; j++)
        RepeatCheck(j);
      for (int j = kernelSize - 1; j < (width - kernelSize) + 1; j++)
      {
        if (RepeatCheck(j + kernelSize - 1))
        {
            out[j * outStride + (i * 4)] = static_cast<uint8_t>(last);
            continue;
        }
          
        uint32_t accum = in[j * 4] * kernel[0];
        for (int k = 1; k < kernelSize; k++)
          accum += kernel[k] * (in[(j - k) * 4] + in[(j + k) * 4]);
        out[j * outStride + (i * 4)] = static_cast<uint8_t>(std::min(static_cast<uint32_t>(255), accum / norm));
      }
      for (int j = (width - kernelSize) + 1; j < width; j++)
      {
        uint32_t accum = in[j * 4] * kernel[0];
        for (int k = 1; k < kernelSize; k++)
          accum += kernel[k] * in[(j - k) * 4];
        for (int k = 1; k < width - j; k++)
          accum += kernel[k] * in[(j + k) * 4];
        out[j * outStride + (i * 4)] = static_cast<uint8_t>(std::min(static_cast<uint32_t>(255), accum / norm));
      }
    }
  };
  
  RawBitmapData temp1;
  RawBitmapData temp2;
  RawBitmapData kernel;
    
  // Get bitmap in 32-bit form
  GetLayerBitmapData(layer, temp1);
    
  if (!temp1.GetSize())
      return;
  temp2.Resize(temp1.GetSize());
    
  // Form kernel (reference blurSize from zero (which will be no blur))
  bool flipped = FlippedBitmap();
  float scale = layer->GetAPIBitmap()->GetScale() * layer->GetAPIBitmap()->GetDrawScale();
  float blurSize = std::max(1.f, (shadow.mBlurSize * scale) + 1.f);
  float blurConst = 4.5f / (blurSize * blurSize);
  int iSize = static_cast<int>(ceil(blurSize));
  int width = layer->GetAPIBitmap()->GetWidth();
  int height = layer->GetAPIBitmap()->GetHeight();
  int stride1 = temp1.GetSize() / width;
  int stride2 = flipped ? -temp1.GetSize() / height : temp1.GetSize() / height;
  int stride3 = flipped ? -stride2 : stride2;

  kernel.Resize(iSize);
        
  for (int i = 0; i < iSize; i++)
    kernel.Get()[i] = static_cast<uint8_t>(std::round(255.f * std::expf(-(i * i) * blurConst)));
  
  // Kernel normalisation
  int normFactor = kernel.Get()[0];
    
  for (int i = 1; i < iSize; i++)
    normFactor += kernel.Get()[i] + kernel.Get()[i];
  
  // Do blur
  uint8_t* asRows = temp1.Get() + AlphaChannel();
  uint8_t* inRows = flipped ? asRows + stride3 * (height - 1) : asRows;
  uint8_t* asCols = temp2.Get() + AlphaChannel();
  
  GaussianBlurSwap(asCols, inRows, kernel.Get(), width, height, stride1, stride2, iSize, normFactor);
  GaussianBlurSwap(asRows, asCols, kernel.Get(), height, width, stride3, stride1, iSize, normFactor);
  
  // Apply alphas to the pattern and recombine/replace the image
  ApplyShadowMask(layer, temp1, shadow);
}

bool IGraphics::LoadFont(const char* fontID, const char* fileNameOrResID)
{
  PlatformFontPtr font = LoadPlatformFont(fontID, fileNameOrResID);
  
  if (font)
  {
    if (LoadAPIFont(fontID, font))
    {
      CachePlatformFont(fontID, font);
      return true;
    }
  }
  
  DBGMSG("Could not locate font %s\n", fileNameOrResID);
  return false;
}

bool IGraphics::LoadFont(const char* fontID, void* pData, int dataSize)
{
  PlatformFontPtr font = LoadPlatformFont(fontID, pData, dataSize);

  if (font)
  {
    if (LoadAPIFont(fontID, font))
    {
      CachePlatformFont(fontID, font);
      return true;
    }
  }

  DBGMSG("Could not load font %s\n", fontID);
  return false;
}

bool IGraphics::LoadFont(const char* fontID, const char* fontName, ETextStyle style)
{
  PlatformFontPtr font = LoadPlatformFont(fontID, fontName, style);
  
  if (font)
  {
    if (LoadAPIFont(fontID, font))
    {
      CachePlatformFont(fontID, font);
      return true;
    }
  }
  
  DBGMSG("Could not locate font %s\n", fontID);
  return false;
}

void IGraphics::DoMeasureTextRotation(const IText& text, const IRECT& bounds, IRECT& rect) const
{
  double tx = 0.0, ty = 0.0;
  
  CalculateTextRotation(text, bounds, rect, tx, ty);
  rect.Translate(static_cast<float>(tx), static_cast<float>(ty));
}

void IGraphics::CalculateTextRotation(const IText& text, const IRECT& bounds, IRECT& rect, double& tx, double& ty) const
{
  if (!text.mAngle)
    return;
  
  IMatrix m = IMatrix().Rotate(text.mAngle);
  
  double x0 = rect.L;
  double y0 = rect.T;
  double x1 = rect.R;
  double y1 = rect.T;
  double x2 = rect.R;
  double y2 = rect.B;
  double x3 = rect.L;
  double y3 = rect.B;
  
  m.TransformPoint(x0, y0);
  m.TransformPoint(x1, y1);
  m.TransformPoint(x2, y2);
  m.TransformPoint(x3, y3);
  
  IRECT r1(static_cast<float>(std::min(x0, x3)), static_cast<float>(std::min(y0, y3)), static_cast<float>(std::max(x0, x3)), static_cast<float>(std::max(y0, y3)));
  IRECT r2(static_cast<float>(std::min(x1, x2)), static_cast<float>(std::min(y1, y2)), static_cast<float>(std::max(x1, x2)), static_cast<float>(std::max(y1, y2)));
  rect = r1.Union(r2);
  
  switch (text.mAlign)
  {
    case EAlign::Near:     tx = bounds.L - rect.L;         break;
    case EAlign::Center:   tx = bounds.MW() - rect.MW();   break;
    case EAlign::Far:      tx = bounds.R - rect.R;         break;
  }
  
  switch (text.mVAlign)
  {
    case EVAlign::Top:      ty = bounds.T - rect.T;        break;
    case EVAlign::Middle:   ty = bounds.MH() - rect.MH();  break;
    case EVAlign::Bottom:   ty = bounds.B - rect.B;        break;
  }
}

void IGraphics::SetQwertyMidiKeyHandlerFunc(std::function<void(const IMidiMsg& msg)> func)
{
  SetKeyHandlerFunc([&, func](const IKeyPress& key, bool isUp) {
    IMidiMsg msg;
    
    int note = 0;
    static int base = 48;
    static bool keysDown[128] = {};
    
    auto onOctSwitch = [&]() {
      base = Clip(base, 24, 96);
      
      for(auto i=0;i<128;i++) {
        if(keysDown[i]) {
          msg.MakeNoteOffMsg(i, 0);
          GetDelegate()->SendMidiMsgFromUI(msg);
          if(func)
            func(msg);
        }
      }
    };
    
    switch (key.VK) {
      case kVK_A: note = 0; break;
      case kVK_W: note = 1; break;
      case kVK_S: note = 2; break;
      case kVK_E: note = 3; break;
      case kVK_D: note = 4; break;
      case kVK_F: note = 5; break;
      case kVK_T: note = 6; break;
      case kVK_G: note = 7; break;
      case kVK_Y: note = 8; break;
      case kVK_H: note = 9; break;
      case kVK_U: note = 10; break;
      case kVK_J: note = 11; break;
      case kVK_K: note = 12; break;
      case kVK_O: note = 13; break;
      case kVK_L: note = 14; break;
      case kVK_Z: if(!isUp) { base -= 12; onOctSwitch(); } return true;
      case kVK_X: if(!isUp) { base += 12; onOctSwitch(); } return true;
      default: return true; // don't beep, but don't do anything
    }
    
    int pitch = base + note;
    
    if(!isUp) {
      if(keysDown[pitch] == false) {
        msg.MakeNoteOnMsg(pitch, 127, 0);
        keysDown[pitch] = true;
        GetDelegate()->SendMidiMsgFromUI(msg);
        if(func)
          func(msg);
      }
    }
    else {
      if(keysDown[pitch] == true) {
        msg.MakeNoteOffMsg(pitch, 0);
        keysDown[pitch] = false;
        GetDelegate()->SendMidiMsgFromUI(msg);
        if(func)
          func(msg);
      }
    }
    
    return true;
  });
}

bool IGraphics::RespondsToGesture(float x, float y)
{
  IControl* pControl = GetMouseControl(x, y, false, false);

  if(pControl && pControl->GetWantsGestures())
    return true;
  
  if(mGestureRegions.Size() == 0)
    return false;
  else
  {
    int regionIdx = mGestureRegions.Find(x, y);
    
    if(regionIdx > -1)
      return true;
  }
  
  return false;
}

void IGraphics::OnGestureRecognized(const IGestureInfo& info)
{
  IControl* pControl = GetMouseControl(info.x, info.y, false, false);

  if(pControl && pControl->GetWantsGestures())
    pControl->OnGesture(info);
  else
  {
    int regionIdx = mGestureRegions.Find(info.x, info.y);
    
    if(regionIdx > -1)
      mGestureRegionFuncs.find(regionIdx)->second(nullptr, info);
  }
}

void IGraphics::AttachGestureRecognizer(EGestureType type)
{
  if (std::find(std::begin(mRegisteredGestures), std::end(mRegisteredGestures), type) != std::end(mRegisteredGestures))
  {
    mRegisteredGestures.push_back(type);
  }
}

void IGraphics::AttachGestureRecognizerToRegion(const IRECT& bounds, EGestureType type, IGestureFunc func)
{
  mGestureRegions.Add(bounds);
  AttachGestureRecognizer(type);
  mGestureRegionFuncs.insert(std::make_pair(mGestureRegions.Size()-1, func));
}

void IGraphics::ClearGestureRegions()
{
  mGestureRegions.Clear();
  mGestureRegionFuncs.clear();
}

#ifdef IGRAPHICS_IMGUI
void IGraphics::AttachImGui(std::function<void(IGraphics*)> drawFunc, std::function<void()> setupFunc)
{
  mImGuiRenderer = std::make_unique<ImGuiRenderer>(this, drawFunc, setupFunc);
  
  CreatePlatformImGui();
}
#endif

  void IGraphics::DrawRotatedBitmap(const IBitmap& bitmap, float destCtrX, float destCtrY, double angle, const IBlend* pBlend)
  {
    float width = bitmap.W() / bitmap.GetDrawScale();
    float height = bitmap.H() / bitmap.GetDrawScale();
    
    PathTransformSave();
    PathTransformTranslate(destCtrX, destCtrY);
    PathTransformRotate((float) angle);
    DrawBitmap(bitmap, IRECT(-width * 0.5f, - height * 0.5f, width * 0.5f, height * 0.5f), 0, 0, pBlend);
    PathTransformRestore();
  }
  
  void IGraphics::DrawPoint(const IColor& color, float x, float y, const IBlend* pBlend)
  {
    FillRect(color, IRECT(x, y, x+1.f, y+1.f), pBlend);
  }
  
  void IGraphics::DrawLine(const IColor& color, float x1, float y1, float x2, float y2, const IBlend* pBlend, float thickness)
  {
    PathClear();
    PathMoveTo(x1, y1);
    PathLineTo(x2, y2);
    PathStroke(color, thickness, IStrokeOptions(), pBlend);
  }
  
  void IGraphics::DrawGrid(const IColor& color, const IRECT& bounds, float gridSizeH, float gridSizeV, const IBlend* pBlend, float thickness)
  {
    PathClear();

    // Vertical Lines grid
    if (gridSizeH > 1.f)
    {
      for (float x = bounds.L + gridSizeH; x < bounds.R; x += gridSizeH)
      {
        PathMoveTo(x, bounds.T);
        PathLineTo(x, bounds.B);
      }
    }
    // Horizontal Lines grid
    if (gridSizeV > 1.f)
    {
      for (float y = bounds.T + gridSizeV; y < bounds.B; y += gridSizeV)
      {
        PathMoveTo(bounds.L, y);
        PathLineTo(bounds.R, y);
      }
    }
    
    PathStroke(color, thickness, IStrokeOptions(), pBlend);
  }
  
  void IGraphics::DrawData(const IColor& color, const IRECT& bounds, float* normYPoints, int nPoints, float* normXPoints, const IBlend* pBlend, float thickness)
  {
    PathClear();
    
    float xPos = bounds.L;

    PathMoveTo(xPos, bounds.B - (bounds.H() * normYPoints[0]));

    for (auto i = 1; i < nPoints; i++)
    {
      if(normXPoints)
        xPos = bounds.L + (bounds.W() * normXPoints[i]);
      else
        xPos = bounds.L + ((bounds.W() / (float) (nPoints - 1) * i));
      
      PathLineTo(xPos, bounds.B - (bounds.H() * normYPoints[i]));
    }
    
    PathStroke(color, thickness, IStrokeOptions(), pBlend);
  }
  
  void IGraphics::DrawDottedLine(const IColor& color, float x1, float y1, float x2, float y2, const IBlend* pBlend, float thickness, float dashLen)
  {
    PathClear();
    
    IStrokeOptions options;
    options.mDash.SetDash(&dashLen, 0.0, 1);
    PathMoveTo(x1, y1);
    PathLineTo(x2, y2);
    PathStroke(color, thickness, options, pBlend);
  }
  
  void IGraphics::DrawTriangle(const IColor& color, float x1, float y1, float x2, float y2, float x3, float y3, const IBlend* pBlend, float thickness)
  {
    PathClear();
    PathTriangle(x1, y1, x2, y2, x3, y3);
    PathStroke(color, thickness, IStrokeOptions(), pBlend);
  }
  
  void IGraphics::DrawRect(const IColor& color, const IRECT& bounds, const IBlend* pBlend, float thickness)
  {
    PathClear();
    PathRect(bounds);
    PathStroke(color, thickness, IStrokeOptions(), pBlend);
  }
  
  void IGraphics::DrawRoundRect(const IColor& color, const IRECT& bounds, float cornerRadius, const IBlend* pBlend, float thickness)
  {
    PathClear();
    PathRoundRect(bounds, cornerRadius);
    PathStroke(color, thickness, IStrokeOptions(), pBlend);
  }
  
  void IGraphics::DrawRoundRect(const IColor& color, const IRECT& bounds, float cRTL, float cRTR, float cRBR, float cRBL, const IBlend* pBlend, float thickness)
  {
    PathClear();
    PathRoundRect(bounds, cRTL, cRTR, cRBR, cRBL);
    PathStroke(color, thickness, IStrokeOptions(), pBlend);
  }
  
  void IGraphics::DrawConvexPolygon(const IColor& color, float* x, float* y, int nPoints, const IBlend* pBlend, float thickness)
  {
    PathClear();
    PathConvexPolygon(x, y, nPoints);
    PathStroke(color, thickness, IStrokeOptions(), pBlend);
  }
  
  void IGraphics::DrawArc(const IColor& color, float cx, float cy, float r, float a1, float a2, const IBlend* pBlend, float thickness)
  {
    PathClear();
    PathArc(cx, cy, r, a1, a2);
    PathStroke(color, thickness, IStrokeOptions(), pBlend);
  }
  
  void IGraphics::DrawCircle(const IColor& color, float cx, float cy, float r, const IBlend* pBlend, float thickness)
  {
    PathClear();
    PathCircle(cx, cy, r);
    PathStroke(color, thickness, IStrokeOptions(), pBlend);
  }
  
  void IGraphics::DrawDottedRect(const IColor& color, const IRECT& bounds, const IBlend* pBlend, float thickness, float dashLen)
  {
    PathClear();
    IStrokeOptions options;
    options.mDash.SetDash(&dashLen, 0., 1);
    PathRect(bounds);
    PathStroke(color, thickness, options, pBlend);
  }
  
  void IGraphics::DrawEllipse(const IColor& color, const IRECT& bounds, const IBlend* pBlend, float thickness)
  {
    PathClear();
    PathEllipse(bounds);
    PathStroke(color, thickness, IStrokeOptions(), pBlend);
  }
  
  void IGraphics::DrawEllipse(const IColor& color, float x, float y, float r1, float r2, float angle, const IBlend* pBlend, float thickness)
  {
    PathClear();
    PathEllipse(x, y, r1, r2, angle);
    PathStroke(color, thickness, IStrokeOptions(), pBlend);
  }

  void IGraphics::FillTriangle(const IColor& color, float x1, float y1, float x2, float y2, float x3, float y3, const IBlend* pBlend)
  {
    PathClear();
    PathTriangle(x1, y1, x2, y2, x3, y3);
    PathFill(color, IFillOptions(), pBlend);
  }
  
  void IGraphics::FillRect(const IColor& color, const IRECT& bounds, const IBlend* pBlend)
  {
    PathClear();
    PathRect(bounds);
    PathFill(color, IFillOptions(), pBlend);
  }
  
  void IGraphics::FillRoundRect(const IColor& color, const IRECT& bounds, float cornerRadius, const IBlend* pBlend)
  {
    PathClear();
    PathRoundRect(bounds, cornerRadius);
    PathFill(color, IFillOptions(), pBlend);
  }
  
  void IGraphics::FillRoundRect(const IColor& color, const IRECT& bounds, float cRTL, float cRTR, float cRBR, float cRBL, const IBlend* pBlend)
  {
    PathClear();
    PathRoundRect(bounds, cRTL, cRTR, cRBR, cRBL);
    PathFill(color, IFillOptions(), pBlend);
  }
  
  void IGraphics::FillConvexPolygon(const IColor& color, float* x, float* y, int nPoints, const IBlend* pBlend)
  {
    PathClear();
    PathConvexPolygon(x, y, nPoints);
    PathFill(color, IFillOptions(), pBlend);
  }
  
  void IGraphics::FillArc(const IColor& color, float cx, float cy, float r, float a1, float a2, const IBlend* pBlend)
  {
    PathClear();
    PathMoveTo(cx, cy);
    PathArc(cx, cy, r, a1, a2);
    PathClose();
    PathFill(color, IFillOptions(), pBlend);
  }
  
  void IGraphics::FillCircle(const IColor& color, float cx, float cy, float r, const IBlend* pBlend)
  {
    PathClear();
    PathCircle(cx, cy, r);
    PathFill(color, IFillOptions(), pBlend);
  }
  
  void IGraphics::FillEllipse(const IColor& color, const IRECT& bounds, const IBlend* pBlend)
  {
    PathClear();
    PathEllipse(bounds);
    PathFill(color, IFillOptions(), pBlend);
  }
  
  void IGraphics::FillEllipse(const IColor& color, float x, float y, float r1, float r2, float angle, const IBlend* pBlend)
  {
    PathClear();
    PathEllipse(x, y, r1, r2, angle);
    PathFill(color, IFillOptions(), pBlend);
  }
  
  void IGraphics::PathTriangle(float x1, float y1, float x2, float y2, float x3, float y3)
  {
    PathMoveTo(x1, y1);
    PathLineTo(x2, y2);
    PathLineTo(x3, y3);
    PathClose();
  }
  
  void IGraphics::PathRect(const IRECT& bounds)
  {
    PathMoveTo(bounds.L, bounds.T);
    PathLineTo(bounds.R, bounds.T);
    PathLineTo(bounds.R, bounds.B);
    PathLineTo(bounds.L, bounds.B);
    PathClose();
  }
  
  void IGraphics::PathRoundRect(const IRECT& bounds, float ctl, float ctr, float cbl, float cbr)
  {
    if (ctl <= 0.f && ctr <= 0.f && cbl <= 0.f && cbr <= 0.f)
    {
      PathRect(bounds);
    }
    else
    {
      const float y = bounds.B - bounds.H();
      PathMoveTo(bounds.L, y + ctl);
      PathArc(bounds.L + ctl, y + ctl, ctl, 270.f, 360.f);
      PathArc(bounds.L + bounds.W() - ctr, y + ctr, ctr, 0.f, 90.f);
      PathArc(bounds.L + bounds.W() - cbr, y + bounds.H() - cbr, cbr, 90.f, 180.f);
      PathArc(bounds.L + cbl, y + bounds.H() - cbl, cbl, 180.f, 270.f);
      PathClose();
    }
  }
  
  void IGraphics::PathRoundRect(const IRECT& bounds, float cr)
  {
    PathRoundRect(bounds, cr, cr, cr, cr);
  }
  
  void IGraphics::PathEllipse(float x, float y, float r1, float r2, float angle)
  {
    PathTransformSave();
    
    if (r1 <= 0.0 || r2 <= 0.0)
      return;
    
    PathTransformTranslate(x, y);
    PathTransformRotate(angle);
    PathTransformScale(r1, r2);
    
    PathCircle(0.0, 0.0, 1.0);
    
    PathTransformRestore();
  }
  
  void IGraphics::PathEllipse(const IRECT& bounds)
  {
    PathEllipse(bounds.MW(), bounds.MH(), bounds.W() / 2.f, bounds.H() / 2.f);
  }
  
  void IGraphics::PathCircle(float cx, float cy, float r)
  {
    PathMoveTo(cx, cy - r);
    PathArc(cx, cy, r, 0.f, 360.f);
    PathClose();
  }
  
  void IGraphics::PathConvexPolygon(float* x, float* y, int nPoints)
  {
    PathMoveTo(x[0], y[0]);
    for(int i = 1; i < nPoints; i++)
      PathLineTo(x[i], y[i]);
    PathClose();
  }
    
  void IGraphics::PathTransformSave()
  {
    mTransformStates.push(mTransform);
  }
  
  void IGraphics::PathTransformRestore()
  {
    if (!mTransformStates.empty())
    {
      mTransform = mTransformStates.top();
      mTransformStates.pop();
      PathTransformSetMatrix(mTransform);
    }
  }
  
  void IGraphics::PathTransformReset(bool clearStates)
  {
    if (clearStates)
    {
      std::stack<IMatrix> newStack;
      mTransformStates.swap(newStack);
    }
    
    mTransform = IMatrix();
    PathTransformSetMatrix(mTransform);
  }
  
  void IGraphics::PathTransformTranslate(float x, float y)
  {
    mTransform.Translate(x, y);
    PathTransformSetMatrix(mTransform);
  }
  
  void IGraphics::PathTransformScale(float scaleX, float scaleY)
  {
    mTransform.Scale(scaleX, scaleY);
    PathTransformSetMatrix(mTransform);
  }
  
  void IGraphics::PathTransformScale(float scale)
  {
    PathTransformScale(scale, scale);
  }
  
  void IGraphics::PathTransformRotate(float angle)
  {
    mTransform.Rotate(angle);
    PathTransformSetMatrix(mTransform);
  }
    
  void IGraphics::PathTransformSkew(float xAngle, float yAngle)
  {
    mTransform.Skew(xAngle, yAngle);
    PathTransformSetMatrix(mTransform);
  }

  void IGraphics::PathTransformMatrix(const IMatrix& matrix)
  {
    mTransform.Transform(matrix);
    PathTransformSetMatrix(mTransform);
  }

  void IGraphics::PathClipRegion(const IRECT r)
  {
    IRECT drawArea = mLayers.empty() ? mClipRECT : mLayers.top()->Bounds();
    IRECT clip = r.Empty() ? drawArea : r.Intersect(drawArea);
    PathTransformSetMatrix(IMatrix());
    SetClipRegion(clip);
    PathTransformSetMatrix(mTransform);
  }
  
  void IGraphics::DrawFittedBitmap(const IBitmap& bitmap, const IRECT& bounds, const IBlend* pBlend)
  {
    PathTransformSave();
    PathTransformTranslate(bounds.L, bounds.T);
    IRECT newBounds(0., 0., static_cast<float>(bitmap.W()), static_cast<float>(bitmap.H()));
    PathTransformScale(bounds.W() / static_cast<float>(bitmap.W()), bounds.H() / static_cast<float>(bitmap.H()));
    DrawBitmap(bitmap, newBounds, 0, 0, pBlend);
    PathTransformRestore();
  }
  
  void IGraphics::DrawSVG(const ISVG& svg, const IRECT& dest, const IBlend* pBlend)
  {
    float xScale = dest.W() / svg.W();
    float yScale = dest.H() / svg.H();
    float scale = xScale < yScale ? xScale : yScale;
    
    PathTransformSave();
    PathTransformTranslate(dest.L, dest.T);
    PathTransformScale(scale);
    DoDrawSVG(svg, pBlend);
    PathTransformRestore();
  }
  
  void IGraphics::DrawRotatedSVG(const ISVG& svg, float destCtrX, float destCtrY, float width, float height, double angle, const IBlend* pBlend)
  {
    PathTransformSave();
    PathTransformTranslate(destCtrX, destCtrY);
    PathTransformRotate((float) angle);
    DrawSVG(svg, IRECT(-width * 0.5f, - height * 0.5f, width * 0.5f, height * 0.5f), pBlend);
    PathTransformRestore();
  }

  IPattern IGraphics::GetSVGPattern(const NSVGpaint& paint, float opacity)
  {
    int alpha = std::min(255, std::max(int(roundf(opacity * 255.f)), 0));
    
    switch (paint.type)
    {
      case NSVG_PAINT_COLOR:
        return IColor(alpha, (paint.color >> 0) & 0xFF, (paint.color >> 8) & 0xFF, (paint.color >> 16) & 0xFF);
        
      case NSVG_PAINT_LINEAR_GRADIENT:
      case NSVG_PAINT_RADIAL_GRADIENT:
      {
        NSVGgradient* pGrad = paint.gradient;
        
        IPattern pattern(paint.type == NSVG_PAINT_LINEAR_GRADIENT ? EPatternType::Linear : EPatternType::Radial);
        
        // Set Extend Rule
        switch (pGrad->spread)
        {
          case NSVG_SPREAD_PAD:       pattern.mExtend = EPatternExtend::Pad;       break;
          case NSVG_SPREAD_REFLECT:   pattern.mExtend = EPatternExtend::Reflect;   break;
          case NSVG_SPREAD_REPEAT:    pattern.mExtend = EPatternExtend::Repeat;    break;
        }
        
        // Copy Stops        
        for (int i = 0; i < pGrad->nstops; i++)
        {
          unsigned int color = pGrad->stops[i].color;
          pattern.AddStop(IColor(255, (color >> 0) & 0xFF, (color >> 8) & 0xFF, (color >> 16) & 0xFF), pGrad->stops[i].offset);
        }
        
        // Copy transform        
        pattern.SetTransform(pGrad->xform[0], pGrad->xform[1], pGrad->xform[2], pGrad->xform[3], pGrad->xform[4], pGrad->xform[5]);
        
        return pattern;
      }
      default:
        return IColor(alpha, 0, 0, 0);
    }
  }
  
  void IGraphics::DoDrawSVG(const ISVG& svg, const IBlend* pBlend)
  {
#ifdef IGRAPHICS_SKIA
    SkCanvas* canvas = static_cast<SkCanvas*>(GetDrawContext());
    svg.mSVGDom->render(canvas); //TODO: blend
#else
    NSVGimage* pImage = svg.mImage;
    
    assert(pImage != nullptr);
    
    for (NSVGshape* pShape = pImage->shapes; pShape; pShape = pShape->next)
    {
      if (!(pShape->flags & NSVG_FLAGS_VISIBLE))
        continue;
      
      // Build a new path for each shape
      PathClear();
      
      // iterate subpaths in this shape
      for (NSVGpath* pPath = pShape->paths; pPath; pPath = pPath->next)
      {
        PathMoveTo(pPath->pts[0], pPath->pts[1]);
        
        for (int i = 1; i < pPath->npts; i += 3)
        {
          float *p = &pPath->pts[i*2];
          PathCubicBezierTo(p[0], p[1], p[2], p[3], p[4], p[5]);
        }
        
        if (pPath->closed)
          PathClose();
        
        // Compute whether this path is a hole or a solid and set the winding direction accordingly.
        int crossings = 0;
        IVec2 p0{pPath->pts[0], pPath->pts[1]};
        IVec2 p1{pPath->bounds[0] - 1.0f, pPath->bounds[1] - 1.0f};
        // Iterate all other paths
        for (NSVGpath *pPath2 = pShape->paths; pPath2; pPath2 = pPath2->next)
        {
          if (pPath2 == pPath)
            continue;
          // Iterate all lines on the path
          if (pPath2->npts < 4)
            continue;
          for (int i = 1; i < pPath2->npts + 3; i += 3)
          {
            float *p = &pPath2->pts[2*i];
            // The previous point
            IVec2 p2 {p[-2], p[-1]};
            // The current point
            IVec2 p3 = (i < pPath2->npts) ? IVec2{p[4], p[5]} : IVec2{pPath2->pts[0], pPath2->pts[1]};
            float crossing = GetLineCrossing(p0, p1, p2, p3);
            float crossing2 = GetLineCrossing(p2, p3, p0, p1);
            if (0.0 <= crossing && crossing < 1.0 && 0.0 <= crossing2)
            {
              crossings++;
            }
          }
        }
        PathSetWinding(crossings % 2 != 0);
      }
      
      // Fill combined path using windings set in subpaths
      if (pShape->fill.type != NSVG_PAINT_NONE)
      {
        IFillOptions options;
        options.mFillRule = EFillRule::Preserve;
        
        options.mPreserve = pShape->stroke.type != NSVG_PAINT_NONE;
        PathFill(GetSVGPattern(pShape->fill, pShape->opacity), options, pBlend);
      }
      
      // Stroke
      if (pShape->stroke.type != NSVG_PAINT_NONE)
      {
        IStrokeOptions options;
        
        options.mMiterLimit = pShape->miterLimit;
        
        switch (pShape->strokeLineCap)
        {
          case NSVG_CAP_BUTT:   options.mCapOption = ELineCap::Butt;    break;
          case NSVG_CAP_ROUND:  options.mCapOption = ELineCap::Round;   break;
          case NSVG_CAP_SQUARE: options.mCapOption = ELineCap::Square;  break;
        }
        
        switch (pShape->strokeLineJoin)
        {
          case NSVG_JOIN_MITER:   options.mJoinOption = ELineJoin::Miter;   break;
          case NSVG_JOIN_ROUND:   options.mJoinOption = ELineJoin::Round;   break;
          case NSVG_JOIN_BEVEL:   options.mJoinOption = ELineJoin::Bevel;   break;
        }
        
        options.mDash.SetDash(pShape->strokeDashArray, pShape->strokeDashOffset, pShape->strokeDashCount);
        
        PathStroke(GetSVGPattern(pShape->stroke, pShape->opacity), pShape->strokeWidth, options, pBlend);
      }
    }
  #endif
  }
