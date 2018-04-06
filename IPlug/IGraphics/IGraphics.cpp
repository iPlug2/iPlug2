
#include "IGraphics.h"

#define NANOSVG_IMPLEMENTATION
#include <cstdio>
#include "nanosvg.h"

#ifdef VST3_API
#include "IPlugVST3.h"
#include "pluginterfaces/base/ustring.h"
#endif

#include "IPlugParameter.h"

#ifndef NDEBUG
#include "IGraphicsLiveEdit.h"
#endif

#include "IControl.h"

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

IGraphics::IGraphics(IDelegate& dlg, int w, int h, int fps)
: mDelegate(dlg)
, mWidth(w)
, mHeight(h)
{
  mFPS = (fps > 0 ? fps : DEFAULT_FPS);
}

IGraphics::~IGraphics()
{
  if (mKeyCatcher)
    DELETE_NULL(mKeyCatcher);

#if !defined(NDEBUG) && defined(APP_API)
  if (mLiveEdit)
    DELETE_NULL(mLiveEdit);
#endif

#ifdef IGRAPHICS_FREETYPE
  if (mFTFace != nullptr)
  {
    FT_Done_Face(mFTFace);
    FT_Done_FreeType(mFTLibrary);
  }
#endif

  mControls.Empty(true);
}

void IGraphics::Resize(int w, int h, float scale)
{
  ReleaseMouseCapture();
  
  float oldScale = mScale;
  mScale = scale;
  mWidth = w;
  mHeight = h;
  
  if (oldScale != scale)
    OnDisplayScale();
  
  GetDelegate().ResizeGraphicsFromUI();
}

void IGraphics::OnDisplayScale()
{
  int i, n = mControls.GetSize();
  IControl** ppControl = mControls.GetList();
  for (i = 0; i < n; ++i, ++ppControl)
  {
    IControl* pControl = *ppControl;
    pControl->OnRescale();
    pControl->OnResize();
  }

  SetAllControlsDirty();
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
  IControl* pBG = new IBitmapControl(mDelegate, 0, 0, -1, bg, kBlendClobber);
  pBG->SetGraphics(this);
  mControls.Insert(0, pBG);
}

void IGraphics::AttachPanelBackground(const IColor& color)
{
  IControl* pBG = new IPanelControl(mDelegate, GetBounds(), color);
  pBG->SetGraphics(this);
  mControls.Insert(0, pBG);
}

int IGraphics::AttachControl(IControl* pControl)
{
  pControl->SetGraphics(this);
  mControls.Add(pControl);
  return mControls.GetSize() - 1;
}

void IGraphics::AttachKeyCatcher(IControl& control)
{
  mKeyCatcher = &control;
  mKeyCatcher->SetGraphics(this);
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
    const IParam* pParam = mDelegate.GetParamObjectFromUI(paramIdx);

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

void IGraphics::SetAllControlsDirty()
{
  int i, n = mControls.GetSize();
  IControl** ppControl = mControls.GetList();
  for (i = 0; i < n; ++i, ++ppControl)
  {
    IControl* pControl = *ppControl;
    pControl->SetDirty(false);
  }
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

void IGraphics::UpdatePeers(IControl* pCaller)
{
  int i, n = mControls.GetSize();
  IControl** ppControl = mControls.GetList();
  for (i = 0; i < n; ++i, ++ppControl) {
    IControl* pControl = *ppControl;
    if (pControl->ParamIdx() == pCaller->ParamIdx() && pControl != pCaller)
    {
      //this is not actually called from the delegate. But we don't want to push the updates to the peers back to the delegate, so we use this method rather than
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
      IPopupMenu menu;

      // Fill the menu
      for (int i = 0; i < nDisplayTexts; ++i)
      {
        const char* str = pParam->GetDisplayText(i);
        // TODO: what if two parameters have the same text?
        if (!strcmp(str, currentText.Get())) // strings are equal
          menu.AddItem( new IPopupMenu::Item(str, IPopupMenu::Item::kChecked), -1 );
        else // not equal
          menu.AddItem( new IPopupMenu::Item(str), -1 );
      }

      if(CreatePopupMenu(menu, bounds))
        control.SetValueFromUserInput(pParam->ToNormalized( (double) menu.GetChosenItemIdx() ));
    }
    // TODO: what if there are Int/Double Params with a display text e.g. -96db = "mute"
    else // type == IParam::kTypeInt || type == IParam::kTypeDouble
    {
      pParam->GetDisplayForHost(currentText, false);
      CreateTextEntry(control, control.GetText(), bounds, currentText.Get());
    }
  }
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

void IGraphics::DrawGrid(const IColor& color, const IRECT& bounds, int gridSizeH, int gridSizeV, const IBlend* pBlend, float thickness)
{
  // Vertical Lines grid
  if (gridSizeH > 1)
  {
    for (int x = 0; x < bounds.W(); x += gridSizeH)
    {
      DrawVerticalLine(color, bounds, (float)x/(float) bounds.W(), pBlend, thickness);
    }
  }
    // Horizontal Lines grid
  if (gridSizeV > 1)
  {
    for (int y = 0; y < bounds.H(); y += gridSizeV)
    {
      DrawHorizontalLine(color, bounds, (float)y/(float) bounds.H(), pBlend, thickness);
    }
  }
}

bool IGraphics::IsDirty(IRECT& bounds)
{
#ifndef NDEBUG
  if (mShowControlBounds)
  {
    bounds = mDrawRECT;
    return true;
  }
#endif

  bool dirty = false;
  int i, n = mControls.GetSize();
  IControl** ppControl = mControls.GetList();
  for (i = 0; i < n; ++i, ++ppControl)
  {
    IControl* pControl = *ppControl;
    if (pControl->IsDirty())
    {
      bounds = bounds.Union(pControl->GetRECT());
      dirty = true;
    }
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

// The OS is announcing what needs to be redrawn,
// which may be a larger area than what is strictly dirty.
void IGraphics::Draw(const IRECT& bounds)
{
  int n = mControls.GetSize();

  if (!n)
    return;

  if (mStrict)
  {
    mDrawRECT = bounds;
    IControl** ppControl = mControls.GetList();
    for (auto i = 0; i < n; ++i, ++ppControl)
    {
      IControl* pControl = *ppControl;
      if (!(pControl->IsHidden()) && bounds.Intersects(pControl->GetRECT()))
      {
        ClipRegion(mDrawRECT.Intersect(pControl->GetRECT()));
        pControl->Draw(*this);

#ifdef AAX_API
        pControl->DrawPTHighlight(*this);
#endif

        ResetClipRegion();
      }
      pControl->SetClean();
    }
  }
  else
  {
    IControl* pBG = mControls.Get(0);
    if (pBG->IsDirty()) // Special case when everything needs to be drawn.
    {
      mDrawRECT = pBG->GetRECT();
      for (int j = 0; j < n; ++j)
      {
        IControl* pControl2 = mControls.Get(j);
        if (!j || !(pControl2->IsHidden()))
        {
          ClipRegion(mDrawRECT.Intersect(pControl2->GetRECT()));
          pControl2->Draw(*this);

#ifdef AAX_API
          pControl2->DrawPTHighlight(*this);
#endif

          pControl2->SetClean();
          ResetClipRegion();
        }
      }
    }
    else
    {
      for (auto i = 1; i < n; ++i)   // loop through all controls starting from one (not bg)
      {
        IControl* pControl = mControls.Get(i); // assign control i to pControl
        if (pControl->IsDirty())   // if pControl is dirty
        {
          mDrawRECT = pControl->GetRECT(); // put the bounds in the mDrawRect member variable
          for (auto j = 0; j < n; ++j)   // loop through all controls
          {
            IControl* pControl2 = mControls.Get(j); // assign control j to pControl2

            // if control1 == control2 OR control2 is not hidden AND control2's bounds intersects mDrawRect
            if (!pControl2->IsHidden() && (i == j || pControl2->GetRECT().Intersects(mDrawRECT)))
            {
              ClipRegion(mDrawRECT.Intersect(pControl2->GetRECT()));
              pControl2->Draw(*this);

#ifdef AAX_API
              pControl2->DrawPTHighlight(*this);
#endif
              ResetClipRegion();
            }
          }
          pControl->SetClean();
        }
      }
    }
  }

#ifndef NDEBUG
  // some helpers for debugging
  if(mShowAreaDrawn)
  {
    static IColor c;
    c.Randomise(50);
    FillRect(c, bounds);
  }

  if(mShowControlBounds)
  {
    for (int j = 1; j < mControls.GetSize(); j++)
    {
      IControl* pControl = mControls.Get(j);
      DrawRect(CONTROL_BOUNDS_COLOR, pControl->GetRECT());
    }
  }

#if defined(APP_API)
  if(mLiveEdit)
    mLiveEdit->Draw(*this);
#endif
//  WDL_String str;
//  str.SetFormatted(32, "x: %i, y: %i", mMouseX, mMouseY);
  IText txt(CONTROL_BOUNDS_COLOR, 20);
  txt.mAlign = IText::kAlignNear;
  IRECT r;
//  DrawText(txt, str.Get(), r);
//  MeasureText(txt, GetDrawingAPIStr(), r);
//  FillRect(COLOR_BLACK, r);
//  DrawText(txt, GetDrawingAPIStr(), r);

#endif

  RenderDrawBitmap();
}

void IGraphics::SetStrictDrawing(bool strict)
{
  mStrict = strict;
  SetAllControlsDirty();
}

void IGraphics::MoveMouseCursor(float x, float y)
{
  // Call this with the window-relative coords after doing platform specifc cursor move
    
  if (mMouseCapture >= 0)
  {
    //mMouseX = x;
    //mMouseY = y;
  }
}

void IGraphics::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  Trace("IGraphics::OnMouseDown", __LINE__, "x:%0.2f, y:%0.2f, mod:LRSCA: %i%i%i%i%i",
        x, y, mod.L, mod.R, mod.S, mod.C, mod.A);

  mMouseDownX = x;
  mMouseDownY = y;
  
#if !defined(NDEBUG) && defined(APP_API)
  if(mLiveEdit)
  {
    mLiveEdit->OnMouseDown(x, y, mod);
    return;
  }
#endif

  ReleaseMouseCapture();
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
  Trace("IGraphics::OnMouseUp", __LINE__, "x:%0.2f, y:%0.2f, mod:LRSCA: %i%i%i%i%i",
        x, y, mod.L, mod.R, mod.S, mod.C, mod.A);

#if !defined(NDEBUG) && defined(APP_API)
  if(mLiveEdit)
  {
    mLiveEdit->OnMouseUp(x, y, mod);
    return;
  }
#endif
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

#if !defined(NDEBUG) && defined(APP_API)
  if(mLiveEdit)
  {
    mLiveEdit->OnMouseOver(x, y, mod);
    return true;
  }
#endif

  if (mHandleMouseOver)
  {
    int c = GetMouseControlIdx(x, y, true);
    if (c >= 0)
    {
      mControls.Get(c)->OnMouseOver(x, y, mod);
      if (mMouseOver >= 0 && mMouseOver != c)
      {
        mControls.Get(mMouseOver)->OnMouseOut();
      }
      mMouseOver = c;
    }
  }
  return mHandleMouseOver;
}

//TODO: if control Rect is the same as IGraphicsBounds, this doesn't fire
void IGraphics::OnMouseOut()
{
  Trace("IGraphics::OnMouseDblClick", __LINE__, "");

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
  Trace("IGraphics::OnMouseDrag:", __LINE__, "x:%0.2f, y:%0.2f, dX:%0.2f, dY:%0.2f, mod:LRSCA: %i%i%i%i%i",
        x, y, dX, dY, mod.L, mod.R, mod.S, mod.C, mod.A);

#if !defined(NDEBUG) && defined(APP_API)
  if(mLiveEdit)
  {
    mLiveEdit->OnMouseDrag(x, y, 0, 0, mod);
    return;
  }
#endif

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
    if (pControl->MouseDblAsSingleClick())
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
  IPopupMenu contextMenu;
  IControl* pControl = GetControl(controlIdx);

  if(pControl)
  {
    pControl->CreateContextMenu(contextMenu);

    if(!contextMenu.NItems())
      return;

#ifdef VST3_API
    IPlugVST3* pVST3 = dynamic_cast<IPlugVST3*>(&mDelegate);

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
    CreatePopupMenu(contextMenu, x, y);
    pControl->OnContextSelection(contextMenu.GetChosenItemIdx());
#endif
  }
  return;
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

IBitmap IGraphics::GetScaledBitmap(IBitmap& src)
{
  return LoadBitmap(src.GetResourceName().Get(), src.N(), src.GetFramesAreHorizontal());
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
    mHandleMouseOver = enable;
}

void IGraphics::EnableLiveEdit(bool enable, const char* file, int gridsize)
{
#if !defined(NDEBUG) && defined(APP_API)
  if(enable)
  {
    mLiveEdit = new IGraphicsLiveEdit(GetDelegate(), file, gridsize);
    mLiveEdit->SetGraphics(this);
  }
  else
  {
    if(mLiveEdit)
      DELETE_NULL(mLiveEdit);
  }
#endif
}

ISVG IGraphics::LoadSVG(const char* name)
{
  WDL_String path;
  bool found = OSFindResource(name, "svg", path);
  assert(found == true);

  SVGHolder* pHolder = s_SVGCache.Find(path.Get());

  if(!pHolder)
  {
    NSVGimage* pImage = nsvgParseFromFile(path.Get(), "px", 72);
    assert(pImage != nullptr);
    //TODO: get win32 resource as string - nsvgParseFromFile won't work
    pHolder = new SVGHolder(pImage);
    s_SVGCache.Add(pHolder, path.Get());
  }

  return ISVG(pHolder->mImage);
}

void IGraphics::LoadFont(const char* name)
{
#ifdef IGRAPHICS_FREETYPE
  if (mFTFace)
  {
    FT_Done_Face(mFTFace);
    FT_Done_FreeType(mFTLibrary);
  }

  FT_Init_FreeType(&mFTLibrary);
  FT_New_Face(mFTLibrary, name, 0, &mFTFace);
#endif
}

IBitmap IGraphics::LoadBitmap(const char* name, int nStates, bool framesAreHorizontal)
{
  const int targetScale = round(GetDisplayScale());

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
  // Cache and return as an IBitmap

  APIBitmap* pAPIBitmap = ScaleAPIBitmap(inBitmap.GetAPIBitmap(), scale);
  IBitmap bitmap = IBitmap(pAPIBitmap, inBitmap.N(), inBitmap.GetFramesAreHorizontal(), name);
  RetainBitmap(bitmap, name);

  return bitmap;
}

inline void IGraphics::SearchNextScale(int& sourceScale, int targetScale)
{
  // Search downwards from 8, skipping targetScale before trying again
  if (sourceScale == targetScale && (targetScale != 8))
    sourceScale = 8;
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
    char fullName[4096];

    if (sourceScale != 1)
    {
      // Form altered name
      char tempName[4096];
      tempName[4095] = 0;

      strncpy(tempName, name, 4095);
      char* filename = strtok(tempName, ".");
      char* ext = strtok(nullptr, ".");
      snprintf(fullName, 4095, "%s@%dx.%s", filename, sourceScale, ext);
    }
    else
      strncpy(fullName, name, 4095);

    if (OSFindResource(fullName, type, result))
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

#ifdef IPLUG_CPP14
auto IGraphics::LoadResource(const char* fileName, int nStates = 1, bool framesAreHorizontal = false)
{
  WDL_String fn(fileName);
  
  const char* ext = fn.get_fileext();
  
  if(strcmp(ext, "png"))
  {
    return LoadBitmap(fileName, nStates, framesAreHorizontal);
  }
  else if(strcmp(ext, "svg"))
  {
    return LoadSVG(fileName);
  }
  else if(strcmp(ext, "ttf"))
  {
    return LoadFont(fileName);
  }
}
#endif

void IGraphics::StyleAllVectorControls(bool drawFrame, bool drawShadow, bool emboss, float roundness, float frameThickness, float shadowOffset, const IVColorSpec& spec)
{
  for (auto c = 0; c < NControls(); c++)
  {
    IVectorBase* pVB = dynamic_cast<IVectorBase*>(GetControl(c));
    if(pVB)
      pVB->Style(drawFrame, drawShadow, emboss, roundness, frameThickness, shadowOffset, spec);
  }
}
