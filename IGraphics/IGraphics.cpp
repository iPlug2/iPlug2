/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include "IGraphics.h"

#define NANOSVG_IMPLEMENTATION
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
, mMinWidth(w / 2)
, mMaxWidth(w * 2)
, mMinHeight(h / 2)
, mMaxHeight(h * 2)
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
  
  RemoveAllControls();
    
  StaticStorage<APIBitmap>::Accessor bitmapStorage(sBitmapCache);
  bitmapStorage.Release();
  StaticStorage<SVGHolder>::Accessor svgStorage(sSVGCache);
  svgStorage.Release();
}

void IGraphics::SetScreenScale(int scale)
{
  mScreenScale = scale;
  PlatformResize(GetDelegate()->EditorResize());
  ForAllControls(&IControl::OnRescale);
  SetAllControlsDirty();
  DrawResize();
}

void IGraphics::Resize(int w, int h, float scale)
{
  w = Clip(w, mMinWidth, mMaxWidth);
  h = Clip(h, mMinHeight, mMaxHeight);
  scale = Clip(scale, mMinScale, mMaxScale);
  
  if (w == Width() && h == Height() && scale == GetDrawScale()) return;
  
  DBGMSG("resize %i, resize %i, scale %f\n", w, h, scale);
  ReleaseMouseCapture();

  mDrawScale = scale;
  mWidth = w;
  mHeight = h;
  
  if (mCornerResizer)
    mCornerResizer->OnRescale();

  PlatformResize(GetDelegate()->EditorResize());
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
  SetAllControlsDirty();
}

void IGraphics::RemoveControls(int fromIdx)
{
  int idx = NControls()-1;
  while (idx >= fromIdx)
  {
    IControl* pControl = GetControl(idx);
    
    if (pControl == mMouseCapture)
      mMouseCapture = nullptr;

    if (pControl == mMouseOver)
      ClearMouseOver();

    if (pControl == mInTextEntry)
      mInTextEntry = nullptr;

    if (pControl == mInPopupMenu)
      mInPopupMenu = nullptr;
    
    mControls.Delete(idx--, true);
  }
  
  SetAllControlsDirty();
}

void IGraphics::RemoveAllControls()
{
  mMouseCapture = nullptr;
  ClearMouseOver();

  mPopupControl = nullptr;
  mTextEntryControl = nullptr;
  mCornerResizer = nullptr;
  mPerfDisplay = nullptr;
    
#if !defined(NDEBUG)
  mLiveEdit = nullptr;
#endif
  
  mControls.Empty(true);
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
    
  mInPopupMenu = nullptr;
}

void IGraphics::AttachBackground(const char* name)
{
  IBitmap bg = LoadBitmap(name, 1, false);
  IControl* pBG = new IBitmapControl(0, 0, bg, kNoParameter, EBlend::Clobber);
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
  pControl->SetDelegate(*GetDelegate());
  pControl->SetTag(ctrlTag);
  pControl->SetGroup(group);
  mControls.Add(pControl);
  pControl->OnAttached();
  return pControl;
}

void IGraphics::AttachCornerResizer(EUIResizerMode sizeMode, bool layoutOnResize)
{
  AttachCornerResizer(new ICornerResizerControl(GetBounds(), 20), sizeMode, layoutOnResize);
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

void IGraphics::AttachPopupMenuControl(const IText& text, const IRECT& bounds)
{
  if (!mPopupControl)
  {
    mPopupControl = std::make_unique<IPopupMenuControl>(kNoParameter, text, IRECT(), bounds);
    mPopupControl->SetDelegate(*GetDelegate());
  }
}

void IGraphics::AttachTextEntryControl()
{
  if (!mTextEntryControl)
  {
    mTextEntryControl = std::make_unique<ITextEntryControl>();
    mTextEntryControl->SetDelegate(*GetDelegate());
  }
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

IControl* IGraphics::GetControlWithTag(int ctrlTag)
{
  for (auto c = 0; c < NControls(); c++)
  {
    IControl* pControl = GetControl(c);
    if (pControl->GetTag() == ctrlTag)
    {
      return pControl;
    }
  }
  
  return nullptr;
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
  
#if !defined(NDEBUG)
  if (mLiveEdit)
    func(*mLiveEdit);
#endif
  
  if (mCornerResizer)
    func(*mCornerResizer);
  
  if (mTextEntryControl)
    func(*mTextEntryControl);
  
  if (mPopupControl)
    func(*mPopupControl);
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
      control.SetTooltip(control.GetParam()->GetNameForHost());
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
      pParam->GetDisplayForHost(currentText);
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
        
        mPromptPopupMenu.SetRootTitle(pParam->GetNameForHost());
      }

      CreatePopupMenu(control, mPromptPopupMenu, bounds, valIdx);
    }
    // TODO: what if there are Int/Double Params with a display text e.g. -96db = "mute"
    else // type == IParam::kTypeInt || type == IParam::kTypeDouble
    {
      pParam->GetDisplayForHost(currentText, false);
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

void IGraphics::MeasureText(const IText& text, const char* str, IRECT& bounds) const
{
  if (!str || str[0] == '\0')
    return;
    
  DoMeasureText(text, str, bounds);
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

void IGraphics::DrawBitmapedText(const IBitmap& bitmap, IRECT& bounds, IText& text, IBlend* pBlend, const char* str, bool vCenter, bool multiline, int charWidth, int charHeight, int charOffset)
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

void IGraphics::DrawGrid(const IColor& color, const IRECT& bounds, float gridSizeH, float gridSizeV, const IBlend* pBlend, float thickness)
{
  // Vertical Lines grid
  if (gridSizeH > 1.f)
  {
    for (float x = gridSizeH; x < bounds.W(); x += gridSizeH)
    {
      DrawVerticalLine(color, bounds, x/bounds.W(), pBlend, thickness);
    }
  }
    // Horizontal Lines grid
  if (gridSizeV > 1.f)
  {
    for (float y = gridSizeV; y < bounds.H(); y += gridSizeV)
    {
      DrawHorizontalLine(color, bounds, y/bounds.H(), pBlend, thickness);
    }
  }
}

void IGraphics::DrawData(const IColor& color, const IRECT& bounds, float* normYPoints, int nPoints, float* normXPoints, const IBlend* pBlend, float thickness)
{
  //TODO:
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

  //TODO: for GL backends, having an ImGui on top currently requires repainting everything on each frame
#if defined IGRAPHICS_IMGUI && (defined IGRAPHICS_GL2 || defined IGRAPHICS_GL3)
  if (mImGuiRenderer && mImGuiRenderer->GetDrawFunc())
  {
    rects.Add(GetBounds());
    return true;
  }
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

void IGraphics::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  Trace("IGraphics::OnMouseDown", __LINE__, "x:%0.2f, y:%0.2f, mod:LRSCA: %i%i%i%i%i",
        x, y, mod.L, mod.R, mod.S, mod.C, mod.A);

  IControl* pControl = GetMouseControl(x, y, true);

#ifdef IGRAPHICS_IMGUI
  if(mImGuiRenderer)
  {
    bool cornerResizer = false;
    if(mCornerResizer.get() != nullptr)
      cornerResizer = pControl == mCornerResizer.get();

    if(!cornerResizer && mImGuiRenderer.get()->OnMouseDown(x, y, mod))
    {
      ReleaseMouseCapture();
      return;
    }
  }
#endif
  
  mMouseDownX = x;
  mMouseDownY = y;

  if (pControl)
  {
    int nVals = pControl->NVals();
    int valIdx = pControl->GetValIdxForPos(x, y);
    int paramIdx = pControl->GetParamIdx((valIdx > kNoValIdx) ? valIdx : 0);

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
      PopupHostContextMenuForParam(pControl, paramIdx, x, y);
      return;
    }
    #endif

    for (int v = 0; v < nVals; v++)
    {
      if (pControl->GetParamIdx(v) > kNoParameter)
        GetDelegate()->BeginInformHostOfParamChangeFromUI(pControl->GetParamIdx(v));
    }
    
    pControl->OnMouseDown(x, y, mod);
  }
}

void IGraphics::OnMouseUp(float x, float y, const IMouseMod& mod)
{
  Trace("IGraphics::OnMouseUp", __LINE__, "x:%0.2f, y:%0.2f, mod:LRSCA: %i%i%i%i%i",
        x, y, mod.L, mod.R, mod.S, mod.C, mod.A);
  
  if (mMouseCapture)
  {
    IControl* pCapturedControl = mMouseCapture; // OnMouseUp could clear mMouseCapture, so stash here
    
    pCapturedControl->OnMouseUp(x, y, mod);
    
    int nVals = pCapturedControl->NVals();

    for (int v = 0; v < nVals; v++)
    {
      if (pCapturedControl->GetParamIdx(v) > kNoParameter)
        GetDelegate()->EndInformHostOfParamChangeFromUI(pCapturedControl->GetParamIdx(v));
    }
    
    ReleaseMouseCapture();
  }

  if (mResizingInProcess)
  {
    mResizingInProcess = false;
    if (GetResizerMode() == EUIResizerMode::Scale)
    {
      // If scaling up we may want to load in high DPI bitmaps if scale > 1.
      ForAllControls(&IControl::OnRescale);
      SetAllControlsDirty();
    }
  }
  
#ifdef IGRAPHICS_IMGUI
  if(mImGuiRenderer)
  {
    if(mImGuiRenderer.get()->OnMouseUp(x, y, mod))
    {
      ReleaseMouseCapture();
      return;
    }
  }
#endif
}

bool IGraphics::OnMouseOver(float x, float y, const IMouseMod& mod)
{
  Trace("IGraphics::OnMouseOver", __LINE__, "x:%0.2f, y:%0.2f, mod:LRSCA: %i%i%i%i%i",
        x, y, mod.L, mod.R, mod.S, mod.C, mod.A);
  
#ifdef IGRAPHICS_IMGUI
  if(mImGuiRenderer)
    mImGuiRenderer.get()->OnMouseMove(x, y, mod);
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

void IGraphics::OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod)
{
  Trace("IGraphics::OnMouseDrag:", __LINE__, "x:%0.2f, y:%0.2f, dX:%0.2f, dY:%0.2f, mod:LRSCA: %i%i%i%i%i",
        x, y, dX, dY, mod.L, mod.R, mod.S, mod.C, mod.A);

  if (mResizingInProcess)
  {
    OnResizeGesture(x, y);
  }
  else if (mMouseCapture && (dX != 0 || dY != 0))
  {
    mMouseCapture->OnMouseDrag(x, y, dX, dY, mod);
  }
#ifdef IGRAPHICS_IMGUI
  else if(mImGuiRenderer)
    mImGuiRenderer.get()->OnMouseMove(x, y, mod);
#endif
}

bool IGraphics::OnMouseDblClick(float x, float y, const IMouseMod& mod)
{
  Trace("IGraphics::OnMouseDblClick", __LINE__, "x:%0.2f, y:%0.2f, mod:LRSCA: %i%i%i%i%i",
        x, y, mod.L, mod.R, mod.S, mod.C, mod.A);
  
#ifdef IGRAPHICS_IMGUI
  if(mImGuiRenderer)
  {
    mImGuiRenderer.get()->OnMouseDown(x, y, mod);
    return true;
  }
#endif

  IControl* pControl = GetMouseControl(x, y, true);
    
  if (pControl)
  {
    if (pControl->GetMouseDblAsSingleClick())
    {
      OnMouseDown(x, y, mod);
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
      mImGuiRenderer.get()->OnMouseWheel(x, y, mod, d);
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
    handled = mImGuiRenderer.get()->OnKeyDown(x, y, key);
    
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
    handled = mImGuiRenderer.get()->OnKeyUp(x, y, key);
    
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
  mMouseCapture = nullptr;
  HideMouseCursor(false);
}

int IGraphics::GetMouseControlIdx(float x, float y, bool mouseOver)
{
  if (!mouseOver || mHandleMouseOver)
  {
    // Search from front to back
    for (auto c = NControls() - 1; c >= (mouseOver ? 1 : 0); --c)
    {
      IControl* pControl = GetControl(c);

#if _DEBUG
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
#if _DEBUG
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

IControl* IGraphics::GetMouseControl(float x, float y, bool capture, bool mouseOver)
{
  if (mMouseCapture)
    return mMouseCapture;
  
  IControl* control = nullptr;
  int controlIdx = -1;
  
  if (!control && mPopupControl && mPopupControl->GetExpanded())
    control = mPopupControl.get();
  
  if (!control && mTextEntryControl && mTextEntryControl->EditInProgress())
    control = mTextEntryControl.get();
  
#if !defined(NDEBUG)
  if (!control && mLiveEdit)
    control = mLiveEdit.get();
#endif
  
  if (!control && mCornerResizer && mCornerResizer->GetRECT().Contains(x, y))
    control = mCornerResizer.get();
  
  if (!control && mPerfDisplay && mPerfDisplay->GetRECT().Contains(x, y))
    control = mPerfDisplay.get();
  
  if (!control)
  {
    controlIdx = GetMouseControlIdx(x, y, mouseOver);
    control = (controlIdx >= 0) ? GetControl(controlIdx) : nullptr;
  }
  
  if (capture)
    mMouseCapture = control;

  if (mouseOver)
    mMouseOverIdx = controlIdx;
  
  return control;
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

      x *= GetDrawScale();
      y *= GetDrawScale();
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

void IGraphics::OnResizeGesture(float x, float y)
{
  if(mGUISizeMode == EUIResizerMode::Scale)
  {
    float scaleX = (x * GetDrawScale()) / mMouseDownX;
    float scaleY = (y * GetDrawScale()) / mMouseDownY;

    Resize(Width(), Height(), std::min(scaleX, scaleY));
  }
  else
  {
    Resize((int) x, (int) y, GetDrawScale());
  }
}

IBitmap IGraphics::GetScaledBitmap(IBitmap& src)
{
  //TODO: bug with # frames!
//  return LoadBitmap(src.GetResourceName().Get(), src.N(), src.GetFramesAreHorizontal(), (GetScreenScale() == 1 && GetDrawScale() > 1.) ? 2 : 0 /* ??? */);
  return LoadBitmap(src.GetResourceName().Get(), src.N(), src.GetFramesAreHorizontal(), GetScreenScale());
}

void IGraphics::EnableTooltips(bool enable)
{
  mEnableTooltips = enable;
  if (enable) mHandleMouseOver = true;
}

void IGraphics::EnableLiveEdit(bool enable, const char* file, int gridsize)
{
#if defined(_DEBUG)
  if (enable)
  {
    if (!mLiveEdit)
    {
      mLiveEdit = std::make_unique<IGraphicsLiveEdit>(mHandleMouseOver/*, file, gridsize*/);
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

#ifdef IGRAPHICS_SKIA
ISVG IGraphics::LoadSVG(const char* fileName, const char* units, float dpi)
{
  StaticStorage<SVGHolder>::Accessor storage(sSVGCache);
  SVGHolder* pHolder = storage.Find(fileName);
  
  if(!pHolder)
  {
    WDL_String path;
    EResourceLocation resourceFound = LocateResource(fileName, "svg", path, GetBundleID(), GetWinModuleHandle(), GetSharedResourcesSubPath());
    
    if (resourceFound == EResourceLocation::kNotFound)
      return ISVG(nullptr); // return invalid SVG
    
    sk_sp<SkSVGDOM> svgDOM;
    bool success = false;
    SkDOM xmlDom;

#ifdef OS_WIN
    if (resourceFound == EResourceLocation::kWinBinary)
    {
      int size = 0;
      const void* pResData = LoadWinResource(path.Get(), "svg", size, GetWinModuleHandle());

      if (pResData)
      {
        SkMemoryStream svgStream(pResData, size);
        success = xmlDom.build(svgStream) != nullptr;
      }
    }
#endif

    if (resourceFound == EResourceLocation::kAbsolutePath)
    {
      SkFILEStream svgStream(path.Get());

      if(svgStream.isValid())
        success = xmlDom.build(svgStream) != nullptr;
    }

    if (success)
      svgDOM = SkSVGDOM::MakeFromDOM(xmlDom);

    success = svgDOM != nullptr;

    if (!success)
      return ISVG(nullptr); // return invalid SVG

    if (svgDOM->containerSize().width() == 0)
      svgDOM->setContainerSize(SkSize::Make(1000, 1000)); //TODO: what should be done when no container size?

    pHolder = new SVGHolder(svgDOM);
    
    storage.Add(pHolder, path.Get());
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
    WDL_String path;
    EResourceLocation resourceFound = LocateResource(fileName, "svg", path, GetBundleID(), GetWinModuleHandle(), GetSharedResourcesSubPath());

    if (resourceFound == EResourceLocation::kNotFound)
      return ISVG(nullptr); // return invalid SVG

    NSVGimage* pImage = nullptr;

#ifdef OS_WIN    
    if (resourceFound == EResourceLocation::kWinBinary)
    {
      int size = 0;
      const void* pResData = LoadWinResource(path.Get(), "svg", size, GetWinModuleHandle());

      if (pResData)
      {
        WDL_String svgStr{ static_cast<const char*>(pResData) };

        pImage = nsvgParse(svgStr.Get(), units, dpi);
      }
      else
        return ISVG(nullptr); // return invalid SVG
    }
#endif

    if (resourceFound == EResourceLocation::kAbsolutePath)
    {
      pImage = nsvgParseFromFile(path.Get(), units, dpi);

      if(!pImage)
        return ISVG(nullptr); // return invalid SVG
    }

    pHolder = new SVGHolder(pImage);
    
    storage.Add(pHolder, path.Get());
  }

  return ISVG(pHolder->mImage);
}
#endif

IBitmap IGraphics::LoadBitmap(const char* name, int nStates, bool framesAreHorizontal, int targetScale)
{
  if (targetScale == 0)
    targetScale = GetScreenScale();

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

void IGraphics::ReleaseBitmap(const IBitmap& bitmap)
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
  int screenScale = GetScreenScale();
  float drawScale = GetDrawScale();

  mScreenScale = scale;
  mDrawScale = inBitmap.GetDrawScale();

  IRECT bounds = IRECT(0, 0, inBitmap.W() / inBitmap.GetDrawScale(), inBitmap.H() / inBitmap.GetDrawScale());
  StartLayer(nullptr, bounds);
  DrawBitmap(inBitmap, bounds, 0, 0, nullptr);
  ILayerPtr layer = EndLayer();
  IBitmap bitmap = IBitmap(layer->mBitmap.release(), inBitmap.N(), inBitmap.GetFramesAreHorizontal(), name);
  RetainBitmap(bitmap, name);

  mScreenScale = screenScale;
  mDrawScale = drawScale;
    
  return bitmap;
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
    IPopupMenu* pReturnMenu = CreatePlatformPopupMenu(menu, bounds);
    SetControlValueAfterPopupMenu(pReturnMenu);
  }
}

void IGraphics::CreatePopupMenu(IControl& control, IPopupMenu& menu, const IRECT& bounds, int valIdx)
{
  DoCreatePopupMenu(control, menu, bounds, valIdx, false);
}

void IGraphics::StartLayer(IControl* pControl, const IRECT& r)
{
  IRECT alignedBounds = r.GetPixelAligned(GetBackingPixelScale());
  const int w = static_cast<int>(std::ceil(GetBackingPixelScale() * std::ceil(alignedBounds.W())));
  const int h = static_cast<int>(std::ceil(GetBackingPixelScale() * std::ceil(alignedBounds.H())));

  PushLayer(new ILayer(CreateAPIBitmap(w, h, GetScreenScale(), GetDrawScale()), alignedBounds, pControl, pControl ? pControl->GetRECT() : IRECT()));
}

void IGraphics::ResumeLayer(ILayerPtr& layer)
{
  ILayerPtr ownedLayer;
    
  ownedLayer.swap(layer);
  ILayer* ownerlessLayer = ownedLayer.release();
    
  if (ownerlessLayer)
  {
    PushLayer(ownerlessLayer);
  }
}

ILayerPtr IGraphics::EndLayer()
{
  return ILayerPtr(PopLayer());
}

void IGraphics::PushLayer(ILayer *layer)
{
  mLayers.push(layer);
  UpdateLayer();
  PathTransformReset();
  PathClipRegion(layer->Bounds());
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

  return pBitmap && !layer->mInvalid && pBitmap->GetDrawScale() == GetDrawScale() && pBitmap->GetScale() == GetScreenScale();
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
      for (int j = kernelSize - 1; j < (width - kernelSize) + 1; j++)
      {
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
      case kVK_Z: base -= 12; onOctSwitch(); return true;
      case kVK_X: base += 12; onOctSwitch(); return true;
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
        msg.MakeNoteOffMsg(pitch, 127, 0);
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
  
#if !defined IGRAPHICS_GL2 && !defined IGRAPHICS_GL3 // TODO: IGRAPHICS_GL!
  CreatePlatformImGui();
#endif
}
#endif
