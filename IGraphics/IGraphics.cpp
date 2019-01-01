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
#include "IFPSDisplayControl.h"
#include "ICornerResizerControl.h"
#include "IPopupMenuControl.h"
#include "ITextEntryControl.h"

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
  RemoveAllControls();
}

void IGraphics::SetScreenScale(int scale)
{
  mScreenScale = scale;
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

  GetDelegate()->EditorPropertiesModified();
  PlatformResize();
  ForAllControls(&IControl::OnResize);
  SetAllControlsDirty();
  DrawResize();
  
  if(mLayoutOnResize)
    GetDelegate()->LayoutUI(this);
}

void IGraphics::RemoveControls(int fromIdx)
{
  int idx = NControls()-1;
  while (idx >= fromIdx)
  {
    IControl* pControl = GetControl(idx);
    if (pControl == mMouseCapture)
    {
      mMouseCapture = nullptr;
    }
    if (pControl == mMouseOver)
    {
      mMouseOver = nullptr;
      mMouseOverIdx = -1;
    }
    
    mControls.Delete(idx--, true);
  }
  
  SetAllControlsDirty();
}

void IGraphics::RemoveAllControls()
{
  mMouseCapture = mMouseOver = nullptr;
  mMouseOverIdx = -1;

  if (mPopupControl)
    DELETE_NULL(mPopupControl);
  
  if (mTextEntryControl)
    DELETE_NULL(mTextEntryControl);
  
  if (mCornerResizer)
    DELETE_NULL(mCornerResizer);
  
#if !defined(NDEBUG)
  if (mLiveEdit)
    DELETE_NULL(mLiveEdit);
#endif
  
  mControls.Empty(true);
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

void IGraphics::AttachCornerResizer(EUIResizerMode sizeMode, bool layoutOnResize)
{
  AttachCornerResizer(new ICornerResizerControl(mDelegate, GetBounds(), 20), sizeMode, layoutOnResize);
}

void IGraphics::AttachCornerResizer(ICornerResizerControl* pControl, EUIResizerMode sizeMode, bool layoutOnResize)
{
  assert(mCornerResizer == nullptr); // only want one corner resizer

  if (mCornerResizer == nullptr)
  {
    mCornerResizer = pControl;
    mGUISizeMode = sizeMode;
    mLayoutOnResize = layoutOnResize;
    mCornerResizer->SetGraphics(this);
  }
  else
  {
    delete pControl;
  }
}

void IGraphics::AttachPopupMenuControl(const IText& text, const IRECT& bounds)
{
  if (mPopupControl == nullptr)
  {
    mPopupControl = new IPopupMenuControl(mDelegate, kNoParameter, text, IRECT(), bounds);
    mPopupControl->SetGraphics(this);
  }
}

void IGraphics::AttachTextEntryControl()
{
  if(mTextEntryControl == nullptr)
  {
    mTextEntryControl = new ITextEntryControl(mDelegate);
    mTextEntryControl->SetGraphics(this);
  }
}

void IGraphics::ShowFPSDisplay(bool enable)
{
  if(enable)
  {
    if (mPerfDisplay == nullptr)
    {
      mPerfDisplay = new IFPSDisplayControl(mDelegate, GetBounds().GetPadded(-10).GetFromTLHC(200, 50));
      mPerfDisplay->SetGraphics(this);
    }
  }
  else
  {
    if(mPerfDisplay)
      DELETE_NULL(mPerfDisplay);
  }

  SetAllControlsDirty();
}

IControl* IGraphics::GetControlWithTag(int controlTag)
{
  for (auto c = 0; c < NControls(); c++)
  {
    IControl* pControl = GetControl(c);
    if (pControl->GetTag() == controlTag)
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

void IGraphics::GrayOutControl(int paramIdx, bool gray)
{
  ForMatchingControls(&IControl::GrayOut, paramIdx, gray);
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

  ForMatchingControls(&IControl::Clamp, paramIdx, lo, hi);
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
  ForAllControls(&IControl::SetDirty, false);
}

void IGraphics::SetAllControlsClean()
{
  ForAllControls(&IControl::SetClean);
}

void IGraphics::AssignParamNameToolTips()
{
  auto func = [](IControl& control)
  {
    if (control.ParamIdx() > -1)
      control.SetTooltip(control.GetParam()->GetNameForHost());
  };
  
  ForStandardControlsFunc(func);
}

void IGraphics::UpdatePeers(IControl* pCaller) // TODO: this could be really slow
{
  auto func = [pCaller](IControl& control)
  {
    // Not actually called from the delegate, but we don't want to push the updates back to the delegate
    if (control.ParamIdx() == pCaller->ParamIdx() && (&control != pCaller))
      control.SetValueFromDelegate(pCaller->GetValue());
  };
    
  ForStandardControlsFunc(func);
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
    
  auto func = [&dirty, &rects](IControl& control)
  {
    if (control.IsDirty())
    {
      rects.Add(control.GetRECT());
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
    IRECT controlBounds = pControl->GetRECT().GetPixelAligned(scale);
    IRECT clipBounds = bounds.Intersect(controlBounds);

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
void IGraphics::Draw(const IRECT& bounds, float scale)
{
  ForAllControlsFunc([this, bounds, scale](IControl& control) { DrawControl(&control, bounds, scale); });

#ifndef NDEBUG
  // Helper for debugging
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
  
  mMouseDownX = x;
  mMouseDownY = y;

  if (pControl)
  {
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
      PopupHostContextMenuForParam(pControl, paramIdx, x, y);
      return;
    }
    #endif

    if (paramIdx >= 0)
    {
      mDelegate.BeginInformHostOfParamChangeFromUI(paramIdx);
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
    int paramIdx = mMouseCapture->ParamIdx();
    mMouseCapture->OnMouseUp(x, y, mod);
    if (paramIdx >= 0)
    {
      mDelegate.EndInformHostOfParamChangeFromUI(paramIdx);
    }
    ReleaseMouseCapture();
  }
    
  if (mResizingInProcess)
  {
    mResizingInProcess = false;
    if (GetResizerMode() == EUIResizerMode::kUIResizerScale)
    {
      // If scaling up we may want to load in high DPI bitmaps if scale > 1.
      ForAllControls(&IControl::OnRescale);
      SetAllControlsDirty();
    }
  }
}

bool IGraphics::OnMouseOver(float x, float y, const IMouseMod& mod)
{
  Trace("IGraphics::OnMouseOver", __LINE__, "x:%0.2f, y:%0.2f, mod:LRSCA: %i%i%i%i%i",
        x, y, mod.L, mod.R, mod.S, mod.C, mod.A);

  // N.B. GetMouseControl handles which controls can recieve mouseovers
    
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

//TODO: THIS DOESN'T GET CALLED ON MAC
void IGraphics::OnMouseOut()
{
  Trace("IGraphics::OnMouseOut", __LINE__, "");

  ForAllControls(&IControl::OnMouseOut);
  mMouseOver = nullptr;
  mMouseOverIdx = -1;
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
}

bool IGraphics::OnMouseDblClick(float x, float y, const IMouseMod& mod)
{
  Trace("IGraphics::OnMouseDblClick", __LINE__, "x:%0.2f, y:%0.2f, mod:LRSCA: %i%i%i%i%i",
        x, y, mod.L, mod.R, mod.S, mod.C, mod.A);

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
  IControl* pControl = GetMouseControl(x, y, false);
  if (pControl) pControl->OnMouseWheel(x, y, mod, d);
}

bool IGraphics::OnKeyDown(float x, float y, const IKeyPress& key)
{
  Trace("IGraphics::OnKeyDown", __LINE__, "x:%0.2f, y:%0.2f, key:%i",
        x, y, key.Ascii);

  bool handled = false;

  IControl* pControl = GetMouseControl(x, y, false);
  
  if (pControl && pControl != GetControl(0))
    handled = pControl->OnKeyDown(x, y, key);

  if(!handled)
    handled = mKeyHandlerFunc ? mKeyHandlerFunc(key) : false;
  
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
      if(mLiveEdit != nullptr)
      {
#endif
        if (!pControl->IsHidden() && !pControl->GetIgnoreMouse())
        {
          if ((!pControl->IsGrayed() || (mouseOver ? pControl->GetMOWhenGrayed() : pControl->GetMEWhenGrayed())))
          {
            if (pControl->IsHit(x, y))
            {
              return c;
            }
          }
        }
#if _DEBUG
      }
      else if (pControl->IsHit(x, y))
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
    control = mPopupControl;
  
  if (!control && mTextEntryControl && mTextEntryControl->EditInProgress())
    control = mTextEntryControl;
  
#if !defined(NDEBUG)
  if (mLiveEdit)
    control = mLiveEdit;
#endif
  
  if (!control && mCornerResizer && mCornerResizer->GetRECT().Contains(x, y))
    control = mCornerResizer;
  
  if (!control && mPerfDisplay && mPerfDisplay->GetRECT().Contains(x, y))
    control = mPerfDisplay;
  
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
  int idx = mLastClickedParam = pControl ? pControl->ParamIdx() : -1;
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

void IGraphics::PopupHostContextMenuForParam(int controlIdx, int paramIdx, float x, float y)
{
  PopupHostContextMenuForParam(GetControl(controlIdx), paramIdx, x, y);
}

void IGraphics::OnGUIIdle()
{
  TRACE;

  ForAllControls(&IControl::OnGUIIdle);
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
  //TODO: bug with # frames!
//  return LoadBitmap(src.GetResourceName().Get(), src.N(), src.GetFramesAreHorizontal(), (GetScreenScale() == 1 && GetDrawScale() > 1.) ? 2 : 0 /* ??? */);
  return LoadBitmap(src.GetResourceName().Get(), src.N(), src.GetFramesAreHorizontal(), GetScreenScale());
}

void IGraphics::EnableTooltips(bool enable)
{
  mEnableTooltips = enable;
  if (enable) mHandleMouseOver = true;
}

void IGraphics::EnableLiveEdit(bool enable/*, const char* file, int gridsize*/)
{
#if defined(_DEBUG)
  if(enable)
  {
    if (mLiveEdit == nullptr)
    {
      mLiveEdit = new IGraphicsLiveEdit(mDelegate, mHandleMouseOver/*, file, gridsize*/);
      mLiveEdit->SetGraphics(this);
    }
  }
  else
  {
    if(mLiveEdit)
      DELETE_NULL(mLiveEdit);
  }
  
  mMouseOver = nullptr;
  mMouseOverIdx = -1;

  SetAllControlsDirty();
#endif
}

ISVG IGraphics::LoadSVG(const char* fileName, const char* units, float dpi)
{
  SVGHolder* pHolder = s_SVGCache.Find(fileName);

  if(!pHolder)
  {
    WDL_String path;
    EResourceLocation resourceFound = OSFindResource(fileName, "svg", path);

    if (resourceFound == EResourceLocation::kNotFound)
      return ISVG(nullptr); // return invalid SVG

    NSVGimage* pImage = nullptr;

#ifdef OS_WIN    
    if (resourceFound == EResourceLocation::kWinBinary)
    {
      int size = 0;
      const void* pResData = LoadWinResource(path.Get(), "svg", size);

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
    
    const char* ext = name + strlen(name) - 1;
    while (ext >= name && *ext != '.') --ext;
    ++ext;
    
    bool bitmapTypeSupported = BitmapExtSupported(ext);
    
    if(!bitmapTypeSupported)
      return IBitmap(); // return invalid IBitmap

    EResourceLocation resourceLocation = SearchImageResource(name, ext, fullPath, targetScale, sourceScale);

    if (resourceLocation == EResourceLocation::kNotFound)
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
        pAPIBitmap = LoadAPIBitmap(fullPath.Get(), sourceScale, resourceLocation, ext);
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

EResourceLocation IGraphics::SearchImageResource(const char* name, const char* type, WDL_String& result, int targetScale, int& sourceScale)
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

    EResourceLocation found = OSFindResource(fullName.Get(), type, result);

    if (found > EResourceLocation::kNotFound)
      return found;
  }

  return EResourceLocation::kNotFound;
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
    if (pVB)
      pVB->Style(drawFrame, drawShadow, emboss, roundness, frameThickness, shadowOffset, spec);
  }
}

void IGraphics::CreateTextEntry(IControl& control, const IText& text, const IRECT& bounds, const char* str)
{
  if (mTextEntryControl)
  {
    mTextEntryControl->CreateTextEntry(bounds, text, str);
    return;
  }
  else
    CreatePlatformTextEntry(control, text, bounds, str);
}

IPopupMenu* IGraphics::CreatePopupMenu(IPopupMenu& menu, const IRECT& bounds, IControl* pCaller)
{
  ReleaseMouseCapture();

  if(mPopupControl) // if we are not using platform pop-up menus
    return mPopupControl->CreatePopupMenu(menu, bounds, pCaller);
  else
    return CreatePlatformPopupMenu(menu, bounds, pCaller);
}

void IGraphics::StartLayer(const IRECT& r)
{
  IRECT alignedBounds = r.GetPixelAligned(GetBackingPixelScale());
  const int w = static_cast<int>(std::round(alignedBounds.W()));
  const int h = static_cast<int>(std::round(alignedBounds.H()));

  PushLayer(new ILayer(CreateAPIBitmap(w, h), alignedBounds), true);
}

void IGraphics::ResumeLayer(ILayerPtr& layer)
{
  ILayerPtr ownedLayer;
    
  ownedLayer.swap(layer);
  ILayer* ownerlessLayer = ownedLayer.release();
    
  if (ownerlessLayer)
  {
    PushLayer(ownerlessLayer, true);
  }
}

ILayerPtr IGraphics::EndLayer()
{
  return ILayerPtr(PopLayer(true));
}

void IGraphics::PushLayer(ILayer *layer, bool clearTransforms)
{
  mLayers.push(layer);
  UpdateLayer();
  PathTransformReset(clearTransforms);
  PathClipRegion(layer->Bounds());
  PathClear();
}

ILayer* IGraphics::PopLayer(bool clearTransforms)
{
  ILayer* pLayer = nullptr;
  
  if (!mLayers.empty())
  {
    pLayer = mLayers.top();
    mLayers.pop();
  }
  
  UpdateLayer();
  PathTransformReset(clearTransforms);
  PathClipRegion();
  PathClear();
  
  return pLayer;
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

void IGraphics::DrawRotatedLayer(const ILayerPtr& layer, double angle)
{
  PathTransformSave();
  PathTransformReset();
  IBitmap bitmap = layer->GetBitmap();
  IRECT bounds = layer->Bounds();
  DrawRotatedBitmap(bitmap, bounds.MW(), bounds.MH(), angle);
  PathTransformRestore();
}

void GaussianBlurSwap(unsigned char *out, unsigned char *in, unsigned char *kernel, int width, int height, int outStride, int inStride, int kernelSize, unsigned long norm)
{
  for (int i = 0; i < height; i++, in += inStride)
  {
    for (int j = 0; j < kernelSize - 1; j++)
    {
      unsigned long accum = in[j * 4] * kernel[0];
      for (int k = 1; k < j + 1; k++)
        accum += kernel[k] * in[(j - k) * 4];
      for (int k = 1; k < kernelSize; k++)
        accum += kernel[k] * in[(j + k) * 4];
      out[j * outStride + (i * 4)] = std::min(static_cast<unsigned long>(255), accum / norm);
    }
    for (int j = kernelSize - 1; j < (width - kernelSize) + 1; j++)
    {
      unsigned long accum = in[j * 4] * kernel[0];
      for (int k = 1; k < kernelSize; k++)
        accum += kernel[k] * (in[(j - k) * 4] + in[(j + k) * 4]);
      out[j * outStride + (i * 4)] = std::min(static_cast<unsigned long>(255), accum / norm);
    }
    for (int j = (width - kernelSize) + 1; j < width; j++)
    {
      unsigned long accum = in[j * 4] * kernel[0];
      for (int k = 1; k < kernelSize; k++)
        accum += kernel[k] * in[(j - k) * 4];
      for (int k = 1; k < width - j; k++)
        accum += kernel[k] * in[(j + k) * 4];
      out[j * outStride + (i * 4)] = std::min(static_cast<unsigned long>(255), accum / norm);
    }
  }
}

void IGraphics::ApplyLayerDropShadow(ILayerPtr& layer, const IShadow& shadow)
{
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
  double scale = layer->GetAPIBitmap()->GetScale() * layer->GetAPIBitmap()->GetDrawScale();
  double blurSize = std::max(1.0, (shadow.mBlurSize * scale) + 1.0);
  double blurConst = 4.5 / (blurSize * blurSize);
  int iSize = ceil(blurSize);
  int width = layer->GetAPIBitmap()->GetWidth();
  int height = layer->GetAPIBitmap()->GetHeight();
  int stride1 = temp1.GetSize() / width;
  int stride2 = flipped ? -temp1.GetSize() / height : temp1.GetSize() / height;
  int stride3 = flipped ? -stride2 : stride2;

  kernel.Resize(iSize);
        
  for (int i = 0; i < iSize; i++)
    kernel.Get()[i] = std::round(255.f * std::expf(-(i * i) * blurConst));
  
  // Kernel normalisation
  
  int normFactor = kernel.Get()[0];
    
  for (int i = 1; i < iSize; i++)
    normFactor += kernel.Get()[i] + kernel.Get()[i];
  
  // Do blur
  
  unsigned char* asRows = temp1.Get() + AlphaChannel();
  unsigned char* inRows = flipped ? asRows + stride3 * (height - 1) : asRows;
  unsigned char* asCols = temp2.Get() + AlphaChannel();
  
  GaussianBlurSwap(asCols, inRows, kernel.Get(), width, height, stride1, stride2, iSize, normFactor);
  GaussianBlurSwap(asRows, asCols, kernel.Get(), height, width, stride3, stride1, iSize, normFactor);
  
  // Apply alphas to the pattern and recombine/replace the image
    
  ApplyShadowMask(layer, temp1, shadow);
}
