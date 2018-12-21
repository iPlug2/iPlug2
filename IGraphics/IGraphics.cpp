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
typedef IPlugVST3 VST3_API_BASE;
#elif defined VST3C_API
#include "pluginterfaces/base/ustring.h"
#include "IPlugVST3_Controller.h"
#include "IPlugVST3_view.h"
typedef IPlugVST3Controller VST3_API_BASE;
#endif

#include "IPlugParameter.h"
#include "IPlugPluginBase.h"

#include "IControl.h"
#include "IControls.h"
#include "IGraphicsLiveEdit.h"
#include "IPerfDisplayControl.h"
#include "IPopupMenuControl.h"

struct SVGHolder
{
  NSVGimage* mImage = nullptr;

  SVGHolder(NSVGimage* pImage)
  : mImage(pImage)
  {
  }

  ~SVGHolder()
  {
    if(mImage)
      nsvgDelete(mImage);

    mImage = nullptr;
  }
};

static StaticStorage<APIBitmap> s_bitmapCache;
static StaticStorage<SVGHolder> s_SVGCache;

IGraphics::IGraphics(IGEditorDelegate& dlg, int w, int h, int fps, float scale)
: mDelegate(dlg)
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
}

IGraphics::~IGraphics()
{
  if (mKeyCatcher)
    DELETE_NULL(mKeyCatcher);

  if (mPopupControl)
    DELETE_NULL(mPopupControl);
  
  if (mCornerResizer)
    DELETE_NULL(mCornerResizer);

#if !defined(NDEBUG)
  if (mLiveEdit)
    DELETE_NULL(mLiveEdit);
#endif

  mControls.Empty(true);
}

void IGraphics::SetScreenScale(int scale)
{
  mScreenScale = scale;

  int i, n = mControls.GetSize();
  IControl** ppControl = mControls.GetList();
  for (i = 0; i < n; ++i, ++ppControl)
  {
    (*ppControl)->OnRescale();
  }
  
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
  
  GetDelegate()->ResizeGraphicsFromUI((int) (w * scale), (int) (h * scale), scale);
  PlatformResize();

  int i, n = mControls.GetSize();
  IControl** ppControl = mControls.GetList();
  for (i = 0; i < n; ++i, ++ppControl)
  {
    (*ppControl)->OnResize();
  }

  SetAllControlsDirty();
  DrawResize();
  
  if(mLayoutOnResize)
    GetDelegate()->LayoutUI(this);
}

void IGraphics::SetControlValueFromStringAfterPrompt(IControl& control, const char* str)
{
  const IParam* pParam = control.GetParam();

  if (pParam)
  {
    const double v = pParam->StringToValue(str);
    control.SetValueFromUserInput(pParam->ToNormalized(v));
  }
}

void IGraphics::AttachBackground(const char* name)
{
  IBitmap bg = LoadBitmap(name, 1, false);
  IControl* pBG = new IBitmapControl(mDelegate, 0, 0, bg, kNoParameter, kBlendClobber);
  pBG->SetGraphics(this);
  mControls.Insert(0, pBG);
}

void IGraphics::AttachPanelBackground(const IColor& color)
{
  IControl* pBG = new IPanelControl(mDelegate, GetBounds(), color);
  pBG->SetGraphics(this);
  mControls.Insert(0, pBG);
}

int IGraphics::AttachControl(IControl* pControl, int controlTag, const char* group)
{
  pControl->SetGraphics(this);
  pControl->SetTag(controlTag);
  pControl->SetGroup(group);
  mControls.Add(pControl);
  return mControls.GetSize() - 1;
}

void IGraphics::AttachKeyCatcher(IControl* pControl)
{
  mKeyCatcher = pControl;
  mKeyCatcher->SetGraphics(this);
}

void IGraphics::AttachCornerResizer(EUIResizerMode sizeMode, bool layoutOnResize)
{
  AttachCornerResizer(new ICornerResizerBase(mDelegate, GetBounds(), 20), sizeMode, layoutOnResize);
}

void IGraphics::AttachCornerResizer(ICornerResizerBase* pControl, EUIResizerMode sizeMode, bool layoutOnResize)
{
  mCornerResizer = pControl;
  mGUISizeMode = sizeMode;
  mLayoutOnResize = layoutOnResize;
  mCornerResizer->SetGraphics(this);
}

void IGraphics::AttachPopupMenuControl(const IText& text, const IRECT& bounds)
{
  mPopupControl = new IPopupMenuControl(mDelegate, kNoParameter, text, IRECT(), bounds);
  mPopupControl->SetGraphics(this);
}

void IGraphics::AttachPerformanceDisplay()
{
  mPerfDisplay = new IPerfDisplayControl(mDelegate, GetBounds().GetPadded(-10).GetFromTLHC(200, 50));
  mPerfDisplay->SetGraphics(this);
}

IControl* IGraphics::GetControlWithTag(int controlTag)
{
  int i, n = mControls.GetSize();
  IControl** ppControl = mControls.GetList();
  for (i = 0; i < n; ++i, ++ppControl)
  {
    IControl* pControl = *ppControl;
    if (pControl->GetTag() == controlTag)
    {
      return pControl;
    }
  }
  
  return nullptr;
}

void IGraphics::HideControl(int paramIdx, bool hide)
{
  int i, n = mControls.GetSize();
  IControl** ppControl = mControls.GetList();
  for (i = 0; i < n; ++i, ++ppControl)
  {
    IControl* pControl = *ppControl;
    if (pControl->ParamIdx() == paramIdx)
    {
      pControl->Hide(hide);
    }
    // Could be more than one, don't break until we check them all.
  }
}

void IGraphics::GrayOutControl(int paramIdx, bool gray)
{
  int i, n = mControls.GetSize();
  IControl** ppControl = mControls.GetList();
  for (i = 0; i < n; ++i, ++ppControl)
  {
    IControl* pControl = *ppControl;
    if (pControl->ParamIdx() == paramIdx)
    {
      pControl->GrayOut(gray);
    }
    // Could be more than one, don't break until we check them all.
  }
}

void IGraphics::ClampControl(int paramIdx, double lo, double hi, bool normalized)
{
  if (!normalized)
  {
    const IParam* pParam = mDelegate.GetParam(paramIdx);

    if (pParam)
    {
      lo = pParam->ToNormalized(lo);
      hi = pParam->ToNormalized(hi);
    }
  }

  int i, n = mControls.GetSize();
  IControl** ppControl = mControls.GetList();
  for (i = 0; i < n; ++i, ++ppControl)
  {
    IControl* pControl = *ppControl;
    if (pControl->ParamIdx() == paramIdx)
    {
      pControl->Clamp(lo, hi);
    }
    // Could be more than one, don't break until we check them all.
  }
}

void IGraphics::ForControlWithParam(int paramIdx, std::function<void(IControl& control)> func)
{
  for (auto c = 0; c < NControls(); c++)
  {
    IControl* pControl = GetControl(c);

    if (pControl->ParamIdx() == paramIdx)
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

void IGraphics::SetAllControlsDirty()
{
  int i, n = mControls.GetSize();
  IControl** ppControl = mControls.GetList();
  for (i = 0; i < n; ++i, ++ppControl)
  {
    (*ppControl)->SetDirty(false);
  }
}

void IGraphics::SetAllControlsClean()
{
  int i, n = mControls.GetSize();
  IControl** ppControl = mControls.GetList();
  for (i = 0; i < n; ++i, ++ppControl)
  {
   (*ppControl)->SetClean();
  }
  
  if(mCornerResizer)
    mCornerResizer->SetClean();
}

void IGraphics::AssignParamNameToolTips()
{
  int i, n = mControls.GetSize();
  IControl** ppControl = mControls.GetList();
  for (i = 0; i < n; ++i, ++ppControl)
  {
    IControl* pControl = *ppControl;
    if (pControl->ParamIdx() > -1)
    {
      pControl->SetTooltip(pControl->GetParam()->GetNameForHost());
    }
  }
}

void IGraphics::UpdatePeers(IControl* pCaller) // TODO: this could be really slow
{
  int i, n = mControls.GetSize();
  IControl** ppControl = mControls.GetList();
  for (i = 0; i < n; ++i, ++ppControl) {
    IControl* pControl = *ppControl;
    if (pControl->ParamIdx() == pCaller->ParamIdx() && pControl != pCaller)
    {
      //this is not actually called from the delegate. But we don't want to push the updates to the peers back to the delegate, so we use this method
      pControl->SetValueFromDelegate(pCaller->GetValue());
      // Could be more than one, don't break until we check them all.
    }
  }
}

void IGraphics::PromptUserInput(IControl& control, const IRECT& bounds)
{
  const IParam* pParam = control.GetParam();

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
      }

      CreatePopupMenu(mPromptPopupMenu, bounds, &control);
    }
    // TODO: what if there are Int/Double Params with a display text e.g. -96db = "mute"
    else // type == IParam::kTypeInt || type == IParam::kTypeDouble
    {
      pParam->GetDisplayForHost(currentText, false);
      CreateTextEntry(control, control.GetText(), bounds, currentText.Get());
    }
  }
}

bool IGraphics::DrawText(const IText& text, const char* str, const IRECT& bounds, const IBlend* pBlend)
{
  return DoDrawMeasureText(text, str, const_cast<IRECT&>(bounds), pBlend, false);
}

bool IGraphics::MeasureText(const IText& text, const char* str, IRECT& bounds)
{
  return DoDrawMeasureText(text, str, bounds, nullptr, true);
}

bool IGraphics::DrawText(const IText& text, const char* str, float x, float y, const IBlend* pBlend)
{
  IRECT bounds = { x, y, x, y };
  return DrawText(text, str, bounds, pBlend);
}

void IGraphics::DrawBitmap(IBitmap& bitmap, const IRECT& bounds, int bmpState, const IBlend* pBlend)
{
  int srcX = 0;
  int srcY = 0;

  bmpState = Clip(bmpState, 1, bitmap.N());

  if (bitmap.N() > 1 && bmpState > 1)
  {
    if (bitmap.GetFramesAreHorizontal())
    {
      srcX = int(0.5f + bitmap.W() * (float) (bmpState - 1) / (float) bitmap.N());
    }
    else
    {
      srcY = int(0.5f + bitmap.H() * (float) (bmpState - 1) / (float) bitmap.N());
    }
  }

  return DrawBitmap(bitmap, bounds, srcX, srcY, pBlend);
}

void IGraphics::DrawBitmapedText(IBitmap& bitmap, IRECT& bounds, IText& text, IBlend* pBlend, const char* str, bool vCenter, bool multiline, int charWidth, int charHeight, int charOffset)
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

    if (text.mAlign == IText::kAlignCenter)
      basicXOffset = bounds.L + ((bounds.W() - (stringLength * charWidth)) / 2.f);
    else if (text.mAlign == IText::kAlignNear)
      basicXOffset = bounds.L;
    else if (text.mAlign == IText::kAlignFar)
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

    for(int line=0; line<nLines; line++)
    {
      float yOffset = basicYOffset + line * charHeight;

      for(int linepos=0; linepos<nCharsThatFitIntoLine; linepos++)
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
    for (float x = 0; x < bounds.W(); x += gridSizeH)
    {
      DrawVerticalLine(color, bounds, x/bounds.W(), pBlend, thickness);
    }
  }
    // Horizontal Lines grid
  if (gridSizeV > 1.f)
  {
    for (float y = 0; y < bounds.H(); y += gridSizeV)
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
  bool dirty = false;
  int i, n = mControls.GetSize();
  IControl** ppControl = mControls.GetList();
  for (i = 0; i < n; ++i, ++ppControl)
  {
    IControl* pControl = *ppControl;
    if (pControl->IsDirty())
    {
      rects.Add(pControl->GetRECT());
      dirty = true;
    }
  }

  if (mPopupControl && mPopupControl->IsDirty())
  {
    rects.Add(mPopupControl->GetRECT());
    dirty = true;
  }

  if (mPerfDisplay)
  {
    rects.Add(mPerfDisplay->GetRECT());
    dirty = true;
  }
  
  if(mCornerResizer && mCornerResizer->IsDirty())
  {
    rects.Add(mCornerResizer->GetRECT());
    dirty = true;
  }
  
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
void IGraphics::DrawControl(IControl* pControl, const IRECT& bounds, bool alwaysShow)
{
  if (pControl && (!pControl->IsHidden() || alwaysShow))
  {
    IRECT clipBounds = bounds.Intersect(pControl->GetRECT());

    if (clipBounds.W() <= 0.0 || clipBounds.H() <= 0)
      return;
    
    PrepareRegion(clipBounds);
    pControl->Draw(*this);
    
#ifdef AAX_API
    pControl->DrawPTHighlight(*this);
#endif
    
#ifndef NDEBUG
    // helper for debugging
    if (mShowControlBounds)
    {
      DrawRect(CONTROL_BOUNDS_COLOR, pControl->GetRECT());
    }
#endif
  }
}

// Draw a region of the graphics (redrawing all contained items)
void IGraphics::Draw(const IRECT& bounds)
{
  int n = mControls.GetSize();
  
  if (!n && !mPopupControl && !mCornerResizer)
    return;
  
  IControl** ppControl = mControls.GetList();
  
  for (auto i = 0; i < n; ++i, ++ppControl)
  {
    DrawControl(*ppControl, bounds, !i);
  }
  
  DrawControl(mPopupControl, bounds, false);
  DrawControl(mCornerResizer, bounds, false);
  DrawControl(mPerfDisplay, bounds, false);

#ifndef NDEBUG
  DrawControl(mLiveEdit, bounds, false);
  
  // helper for debugging
  if (mShowAreaDrawn)
  {
    PrepareRegion(bounds);
    static IColor c;
    c.Randomise(50);
    FillRect(c, bounds);
  }
#endif
}

// Called indicating a number of rectangles in the UI that need to redraw
void IGraphics::Draw(IRECTList& rects)
{
  if (!rects.Size())
    return;
  
  BeginFrame();
    
  if (mStrict)
  {
    IRECT r = rects.Bounds();
    r.PixelAlign();
    Draw(r);
  }
  else
  {
    rects.PixelAlign();
    rects.Optimize();
    
    for (auto i = 0; i < rects.Size(); i++)
      Draw(rects.Get(i));
  }
  
  EndFrame();
}

void IGraphics::SetStrictDrawing(bool strict)
{
  mStrict = strict;
  SetAllControlsDirty();
}

//void IGraphics::MoveMouseCursor(float x, float y)
//{
//  // Call this with the window-relative coords after doing platform specifc cursor move
//
//  if (mMouseCapture >= 0)
//  {
//    //mMouseX = x;
//    //mMouseY = y;
//  }
//}

void IGraphics::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  Trace("IGraphics::OnMouseDown", __LINE__, "x:%0.2f, y:%0.2f, mod:LRSCA: %i%i%i%i%i",
        x, y, mod.L, mod.R, mod.S, mod.C, mod.A);

  mMouseDownX = x;
  mMouseDownY = y;

#if DEBUG
  if(mLiveEdit)
  {
    mLiveEdit->OnMouseDown(x, y, mod);
    return;
  }
#endif

  ReleaseMouseCapture();

  if(mPopupControl && mPopupControl->GetExpanded())
  {
    mPopupControl->OnMouseDown(x, y, mod);
    return;
  }
  
  if(mCornerResizer)
  {
    if(mCornerResizer->GetRECT().Contains(x, y))
    {
      mCornerResizer->OnMouseDown(x, y, mod);
      return;
    }
  }
  
  if(mPerfDisplay)
  {
    if(mPerfDisplay->GetRECT().Contains(x, y))
    {
      mPerfDisplay->OnMouseDown(x, y, mod);
      return;
    }
  }

  int c = GetMouseControlIdx(x, y);
  if (c >= 0)
  {
    mMouseCapture = c;

    IControl* pControl = mControls.Get(c);
    int paramIdx = pControl->ParamIdx();

    #ifdef AAX_API
    if (mAAXViewContainer && paramIdx >= 0)
    {
      uint32_t mods = GetAAXModifiersFromIMouseMod(mod);
      #ifdef OS_WIN
      // required to get start/windows and alt keys
      uint32_t aaxViewMods = 0;
      mAAXViewContainer->GetModifiers(&aaxViewMods);
      mods |= aaxViewMods;
      #endif
      WDL_String paramID;
      paramID.SetFormatted(32, "%i", paramIdx+1);

      if (mAAXViewContainer->HandleParameterMouseDown(paramID.Get(), mods) == AAX_SUCCESS)
      {
        return; // event handled by PT
      }
    }
    #endif

    #ifndef IGRAPHICS_NO_CONTEXT_MENU
    if (mod.R && paramIdx >= 0)
    {
      ReleaseMouseCapture();
      PopupHostContextMenuForParam(c, paramIdx, x, y);
      return;
    }
    #endif

    if (paramIdx >= 0)
      mDelegate.BeginInformHostOfParamChangeFromUI(paramIdx);

    pControl->OnMouseDown(x, y, mod);
  }
}

void IGraphics::OnMouseUp(float x, float y, const IMouseMod& mod)
{
  if(mResizingInProcess)
  {
    mResizingInProcess = false;
    mCornerResizer->OnMouseUp(x, y, mod);
    
    // if scaling up we may want to load in high DPI bitmaps if scale > 1.
    if(GetResizerMode() == EUIResizerMode::kUIResizerScale)
    {
      int i, n = mControls.GetSize();
      IControl** ppControl = mControls.GetList();
      for (i = 0; i < n; ++i, ++ppControl)
      {
        (*ppControl)->OnRescale();
      }
      
      SetAllControlsDirty();
    }
    
    return;
  }
  
  Trace("IGraphics::OnMouseUp", __LINE__, "x:%0.2f, y:%0.2f, mod:LRSCA: %i%i%i%i%i",
        x, y, mod.L, mod.R, mod.S, mod.C, mod.A);

#if !defined(NDEBUG)
  if(mLiveEdit)
  {
    mLiveEdit->OnMouseUp(x, y, mod);
    return;
  }
#endif


  if(mPopupControl && mPopupControl->GetExpanded())
  {
    ReleaseMouseCapture();

    if(mPopupControl->GetRECT().Contains(x, y))
    {
      mPopupControl->OnMouseUp(x, y, mod);
      return;
    }
  }

  int c = GetMouseControlIdx(x, y);
  ReleaseMouseCapture();

  if (c >= 0)
  {
    IControl* pControl = mControls.Get(c);
    pControl->OnMouseUp(x, y, mod);
    pControl = mControls.Get(c); // needed if the mouse message caused a resize/rebuild
    int paramIdx = pControl->ParamIdx();
    if (paramIdx >= 0)
    {
      mDelegate.EndInformHostOfParamChangeFromUI(paramIdx);
    }
  }
}

bool IGraphics::OnMouseOver(float x, float y, const IMouseMod& mod)
{
  Trace("IGraphics::OnMouseOver", __LINE__, "x:%0.2f, y:%0.2f, mod:LRSCA: %i%i%i%i%i",
        x, y, mod.L, mod.R, mod.S, mod.C, mod.A);

#if !defined(NDEBUG)
  if(mLiveEdit)
  {
    mLiveEdit->OnMouseOver(x, y, mod);
    return true;
  }
#endif

  if(mPopupControl && mPopupControl->GetExpanded())
  {
    if(mPopupControl->GetRECT().Contains(x, y))
    {
      mPopupControl->OnMouseOver(x, y, mod);
    }
    else
      mPopupControl->OnMouseOut();
    
    return true;
  }
  else if(mCornerResizer)
  {
    static bool inCornerResizer = false;
    
    if(mCornerResizer->GetRECT().Contains(x, y))
    {
      inCornerResizer = true;
      mCornerResizer->OnMouseOver(x, y, mod);
      return true;
    }
    else
    {
      if(inCornerResizer)
      {
        mCornerResizer->OnMouseOut();
        inCornerResizer = false;
        return true;
      }
    }
  }

  if (mHandleMouseOver)
  {
    int c = GetMouseControlIdx(x, y, true);
    if (mMouseOver > 0 && mMouseOver != c) // the background should not receive MouseOver calls
    {
      mControls.Get(mMouseOver)->OnMouseOut();
    }
    mMouseOver = c;
    if (c > 0) // the background should not receive MouseOver calls
    {
      mControls.Get(c)->OnMouseOver(x, y, mod);
      return true;
    }
  }

  return false;
}

//TODO: THIS DOESN'T GET CALLED ON MAC
void IGraphics::OnMouseOut()
{
  Trace("IGraphics::OnMouseOut", __LINE__, "");

  if(mPopupControl && mPopupControl->GetExpanded())
  {
    mPopupControl->OnMouseOut();
  }

  if(mCornerResizer)
  {
    mCornerResizer->OnMouseOut();
  }

  int i, n = mControls.GetSize();
  IControl** ppControl = mControls.GetList();
  for (i = 0; i < n; ++i, ++ppControl)
  {
    IControl* pControl = *ppControl;
    pControl->OnMouseOut();
  }
  mMouseOver = -1;
}

void IGraphics::OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod)
{
  if(mResizingInProcess)
  {
    OnResizeGesture(x, y);
    return;
  }
  
  Trace("IGraphics::OnMouseDrag:", __LINE__, "x:%0.2f, y:%0.2f, dX:%0.2f, dY:%0.2f, mod:LRSCA: %i%i%i%i%i",
        x, y, dX, dY, mod.L, mod.R, mod.S, mod.C, mod.A);

#if !defined(NDEBUG)
  if(mLiveEdit)
  {
    mLiveEdit->OnMouseDrag(x, y, 0, 0, mod);
    return;
  }
#endif

  if(mPopupControl && mPopupControl->GetExpanded())
  {
    mPopupControl->OnMouseDrag(x, y, dX, dY, mod);
    return;
  }

  int c = mMouseCapture;
  if (c >= 0 && (dX != 0 || dY != 0))
    mControls.Get(c)->OnMouseDrag(x, y, dX, dY, mod);
}

bool IGraphics::OnMouseDblClick(float x, float y, const IMouseMod& mod)
{
  Trace("IGraphics::OnMouseDblClick", __LINE__, "x:%0.2f, y:%0.2f, mod:LRSCA: %i%i%i%i%i",
        x, y, mod.L, mod.R, mod.S, mod.C, mod.A);

  ReleaseMouseCapture();
  bool newCapture = false;
  int c = GetMouseControlIdx(x, y);
  if (c >= 0)
  {
    IControl* pControl = mControls.Get(c);
    if (pControl->GetMouseDblAsSingleClick())
    {
      mMouseCapture = c;
      pControl->OnMouseDown(x, y, mod);
      newCapture = true;
    }
    else
    {
      pControl->OnMouseDblClick(x, y, mod);
    }
  }
  return newCapture;
}

void IGraphics::OnMouseWheel(float x, float y, const IMouseMod& mod, float d)
{
  if(mPopupControl && mPopupControl->GetExpanded())
  {
    mPopupControl->OnMouseWheel(x, y, mod, d);
    return;
  }

  int c = GetMouseControlIdx(x, y);
  if (c >= 0)
  {
    mControls.Get(c)->OnMouseWheel(x, y, mod, d);
  }
}

void IGraphics::ReleaseMouseCapture()
{
  mMouseCapture = -1;
  HideMouseCursor(false);
}

bool IGraphics::OnKeyDown(float x, float y, int key)
{
  Trace("IGraphics::OnKeyDown", __LINE__, "x:%0.2f, y:%0.2f, key:%i",
        x, y, key);

  int c = GetMouseControlIdx(x, y);
  if (c > 0)
    return mControls.Get(c)->OnKeyDown(x, y, key);
  else if (mKeyCatcher)
    return mKeyCatcher->OnKeyDown(x, y, key);
  else
    return false;
}

int IGraphics::GetMouseControlIdx(float x, float y, bool mo)
{
  if (mMouseCapture >= 0)
    return mMouseCapture;

  bool allow; // this is so that mouseovers can still be called when a control is grayed out

  // The BG is a control and will catch everything, so assume the programmer
  // attached the controls from back to front, and return the frontmost match.
  int i = mControls.GetSize() - 1;
  IControl** ppControl = mControls.GetList() + i;
  for (/* */; i >= 0; --i, --ppControl)
  {
    IControl* pControl = *ppControl;

    if (mo)
    {
      if (pControl->GetMOWhenGrayed())
        allow = true;
      else
        allow = !pControl->IsGrayed();
    }
    else
    {
      if (pControl->GetMEWhenGrayed())
        allow = true;
      else
        allow = !pControl->IsGrayed();
    }

    if (!pControl->IsHidden() && !pControl->GetIgnoreMouse() && allow && pControl->IsHit(x, y))
    {
      return i;
    }
  }
  return -1;
}

int IGraphics::GetParamIdxForPTAutomation(float x, float y)
{
  int ctrl = GetMouseControlIdx(x, y, false);
  int idx = -1;

  if(ctrl)
    idx = mControls.Get(ctrl)->ParamIdx();

  mLastClickedParam = idx;
  return idx;
}

int IGraphics::GetLastClickedParamForPTAutomation()
{
  const int idx = mLastClickedParam;
  mLastClickedParam = kNoParameter;
  return idx;
}

void IGraphics::SetPTParameterHighlight(int param, bool isHighlighted, int color)
{
  int i, n = mControls.GetSize();
  IControl** ppControl = mControls.GetList();
  for (i = 0; i < n; ++i, ++ppControl)
  {
    IControl* pControl = *ppControl;

    if (pControl->ParamIdx() == param)
    {
      pControl->SetPTParameterHighlight(isHighlighted, color);
    }
  }
}

void IGraphics::PopupHostContextMenuForParam(int controlIdx, int paramIdx, float x, float y)
{
  IPopupMenu& contextMenu = mPromptPopupMenu;
  contextMenu.Clear();

  IControl* pControl = GetControl(controlIdx);

  if(pControl)
  {
    pControl->CreateContextMenu(contextMenu);

    if(!contextMenu.NItems())
      return;

#if defined VST3_API || defined VST3C_API
    VST3_API_BASE* pVST3 = dynamic_cast<VST3_API_BASE*>(&mDelegate);

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

        pVST3ContextMenu->addItem(item, GetControl(controlIdx));
      }

      pVST3ContextMenu->popup((Steinberg::UCoord) x, (Steinberg::UCoord) y);
      pVST3ContextMenu->release();
    }

#else
    if(mPopupControl) // if we are not using platform popup menus, IPopupMenuControl will not block
    {
      CreatePopupMenu(contextMenu, x, y, pControl);
      mPopupControl->SetMenuIsContextMenu(true);
    }
    else
    {
      CreatePopupMenu(contextMenu, x, y);
      pControl->OnContextSelection(contextMenu.GetChosenItemIdx());
    }
#endif
  }
}

void IGraphics::OnGUIIdle()
{
  TRACE;

  int i, n = mControls.GetSize();
  IControl** ppControl = mControls.GetList();
  for (i = 0; i < n; ++i, ++ppControl)
  {
    IControl* pControl = *ppControl;
    pControl->OnGUIIdle();
  }
}

void IGraphics::OnResizeGesture(float x, float y)
{
  if(mGUISizeMode == EUIResizerMode::kUIResizerScale)
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
  return LoadBitmap(src.GetResourceName().Get(), src.N(), src.GetFramesAreHorizontal(), (GetScreenScale() == 1 && GetDrawScale() > 1.) ? 2 : 0);
}

void IGraphics::OnDrop(const char* str, float x, float y)
{
  int i = GetMouseControlIdx(x, y);
  IControl* pControl = GetControl(i);
  if (pControl != nullptr)
    pControl->OnDrop(str);
}

void IGraphics::EnableTooltips(bool enable)
{
  mEnableTooltips = enable;
  if (enable)
    mHandleMouseOver = true;
}

void IGraphics::EnableLiveEdit(bool enable, const char* file, int gridsize)
{
#if defined(DEBUG)
  if(enable)
  {
    mLiveEdit = new IGraphicsLiveEdit(mDelegate, file, gridsize);
    mLiveEdit->SetGraphics(this);
  }
  else
  {
    if(mLiveEdit)
      DELETE_NULL(mLiveEdit);
  }
#endif
}

#ifdef OS_WIN
NSVGimage* LoadSVGFromWinResource(HINSTANCE hInst, const char* resid)
{
  HRSRC hResource = FindResource(hInst, resid, "SVG");
  if (!hResource) return NULL;

  DWORD imageSize = SizeofResource(hInst, hResource);
  if (imageSize < 8) return NULL;

  HGLOBAL res = LoadResource(hInst, hResource);
  const void* pResourceData = LockResource(res);
  if (!pResourceData) return NULL;

  return nsvgParse((char*)pResourceData, "px", 72);
}
#endif

ISVG IGraphics::LoadSVG(const char* name)
{
  WDL_String path;
  bool resourceFound = OSFindResource(name, "svg", path);
  assert(resourceFound == true);

  SVGHolder* pHolder = s_SVGCache.Find(path.Get());

  if(!pHolder)
  {
#ifdef OS_WIN
    NSVGimage* pImage = LoadSVGFromWinResource((HINSTANCE) GetPlatformInstance(), path.Get());
#else
    NSVGimage* pImage = nsvgParseFromFile(path.Get(), "px", 72);
#endif
    assert(pImage != nullptr);

    pHolder = new SVGHolder(pImage);
    s_SVGCache.Add(pHolder, path.Get());
  }

  return ISVG(pHolder->mImage);
}

IBitmap IGraphics::LoadBitmap(const char* name, int nStates, bool framesAreHorizontal, int targetScale)
{
  if (targetScale == 0)
    targetScale = GetScreenScale();

  APIBitmap* pAPIBitmap = s_bitmapCache.Find(name, targetScale);

  // If the bitmap is not already cached at the targetScale
  if (!pAPIBitmap)
  {
    WDL_String fullPath;
    int sourceScale = 0;
    bool fromDisk = false;

    if (!SearchImageResource(name, "png", fullPath, targetScale, sourceScale))
    {
      // If no resource exists then search the cache for a suitable match
      pAPIBitmap = SearchBitmapInCache(name, targetScale, sourceScale);
    }
    else
    {
      // Try again in cache for mismatched bitmaps, but load from disk if needed
      if (sourceScale != targetScale)
        pAPIBitmap = s_bitmapCache.Find(name, sourceScale);

      if (!pAPIBitmap)
      {
        pAPIBitmap = LoadAPIBitmap(fullPath, sourceScale);
        fromDisk = true;
      }
    }

    // Protection from searching for non-existant bitmaps (e.g. typos in config.h or .rc)
    assert(pAPIBitmap);

    const IBitmap bitmap(pAPIBitmap, nStates, framesAreHorizontal, name);

    // Scale if needed
    if (pAPIBitmap->GetScale() != targetScale)
    {
      // Scaling adds to the cache but if we've loaded from disk then we need to dispose of the temporary APIBitmap
      IBitmap scaledBitmap = ScaleBitmap(bitmap, name, targetScale);
      if (fromDisk)
        delete pAPIBitmap;
      return scaledBitmap;
    }

    // Retain if we've newly loaded from disk
    if (fromDisk)
      RetainBitmap(bitmap, name);
  }

  return IBitmap(pAPIBitmap, nStates, framesAreHorizontal, name);
}

void IGraphics::ReleaseBitmap(const IBitmap& bitmap)
{
  s_bitmapCache.Remove(bitmap.GetAPIBitmap());
}

void IGraphics::RetainBitmap(const IBitmap& bitmap, const char* cacheName)
{
  s_bitmapCache.Add(bitmap.GetAPIBitmap(), cacheName, bitmap.GetScale());
}

IBitmap IGraphics::ScaleBitmap(const IBitmap& inBitmap, const char* name, int scale)
{
  APIBitmap* pAPIBitmap = ScaleAPIBitmap(inBitmap.GetAPIBitmap(), scale);
  IBitmap bitmap = IBitmap(pAPIBitmap, inBitmap.N(), inBitmap.GetFramesAreHorizontal(), name);
  RetainBitmap(bitmap, name);

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

bool IGraphics::SearchImageResource(const char* name, const char* type, WDL_String& result, int targetScale, int& sourceScale)
{
  // Search target scale, then descending
  for (sourceScale = targetScale ; sourceScale > 0; SearchNextScale(sourceScale, targetScale))
  {
    WDL_String fullName(name);
    
    if (sourceScale != 1)
    {
      WDL_String baseName(fullName.get_filepart()); baseName.remove_fileext();
      WDL_String ext(fullName.get_fileext());
      fullName.SetFormatted((int) (strlen(name) + strlen("@2x")), "%s@%dx%s", baseName.Get(), sourceScale, ext.Get());
    }
      
    if (OSFindResource(fullName.Get(), type, result))
      return true;
  }

  return false;
}

APIBitmap* IGraphics::SearchBitmapInCache(const char* name, int targetScale, int& sourceScale)
{
  // Search target scale, then descending
  for (sourceScale = targetScale; sourceScale > 0; SearchNextScale(sourceScale, targetScale))
  {
    APIBitmap* pBitmap = s_bitmapCache.Find(name, sourceScale);

    if (pBitmap)
      return pBitmap;
  }

  return nullptr;
}

void IGraphics::StyleAllVectorControls(bool drawFrame, bool drawShadow, bool emboss, float roundness, float frameThickness, float shadowOffset, const IVColorSpec& spec)
{
  for (auto c = 0; c < NControls(); c++)
  {
    IVectorBase* pVB = dynamic_cast<IVectorBase*>(GetControl(c));
    if(pVB)
      pVB->Style(drawFrame, drawShadow, emboss, roundness, frameThickness, shadowOffset, spec);
  }
}

void IGraphics::StartLayer(const IRECT& r)
{
  mLayers.push(new ILayer(CreateAPIBitmap(r.W(), r.H()), r));
  UpdateLayer();
  PathTransformReset(true);
  PathClipRegion(r);
  PathClear();
}

ILayerPtr IGraphics::EndLayer()
{
  ILayer* pLayer = nullptr;
  
  if (!mLayers.empty())
  {
    pLayer = mLayers.top();
    mLayers.pop();
  }
  
  UpdateLayer();
  PathTransformReset(true);
  PathClipRegion();
  PathClear();
  
  return ILayerPtr(pLayer);
}

bool IGraphics::CheckLayer(const ILayerPtr& layer)
{
  const APIBitmap* pBitmap = layer ? layer->GetAPIBitmap() : nullptr;
  return pBitmap && !layer->mInvalid && pBitmap->GetDrawScale() == GetDrawScale() && pBitmap->GetScale() == GetScreenScale();
}

void IGraphics::DrawLayer(const ILayerPtr& layer)
{
  PathTransformSave();
  PathTransformReset();
  IBitmap bitmap = layer->GetBitmap();
  IRECT bounds = layer->Bounds();
  DrawBitmap(bitmap, bounds, 0, 0);
  PathTransformRestore();
}
