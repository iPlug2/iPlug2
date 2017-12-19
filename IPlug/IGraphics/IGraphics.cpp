#include "IGraphics.h"

IGraphics::IGraphics(IPlugBaseGraphics& plug, int w, int h, int fps)
: mPlug(plug)
, mWidth(w)
, mHeight(h)
, mIdleTicks(0)
, mMouseCapture(-1)
, mMouseOver(-1)
, mMouseX(0)
, mMouseY(0)
, mHandleMouseOver(false)
, mStrict(true)
, mLastClickedParam(-1)
, mKeyCatcher(nullptr)
, mCursorHidden(false)
, mHiddenMousePointX(-1)
, mHiddenMousePointY(-1)
, mEnableTooltips(false)
, mShowControlBounds(false)
, mShowAreaDrawn(false)
, mDisplayScale(1.)
, mScale(1.)
{
  mFPS = (fps > 0 ? fps : DEFAULT_FPS);
}

IGraphics::~IGraphics()
{
  if (mKeyCatcher)
    DELETE_NULL(mKeyCatcher);
  
  mControls.Empty(true);
}

void IGraphics::Resize(int w, int h)
{
  ReleaseMouseCapture();
//  mControls.Empty(true); // TODO fix
//  PrepDraw();
//  mPlug.ResizeGraphics(w, h);
  
  for (int i = 0; i < mPlug.NParams(); ++i)
    SetParameterFromPlug(i, mPlug.GetParam(i)->GetNormalized(), true);
}

void IGraphics::SetFromStringAfterPrompt(IControl* pControl, IParam* pParam, const char* txt)
{
  if (pParam)
  {
    double v;

    IParam::EParamType type = pParam->Type();

    if ( type == IParam::kTypeEnum || type == IParam::kTypeBool)
    {
      int vi = 0;
      pParam->MapDisplayText(txt, &vi);
      v = (double)vi;
    }
    else
    {
      v = atof(txt);
      if (pParam->GetDisplayIsNegated()) v = -v;
    }

    pControl->SetValueFromUserInput(pParam->GetNormalized(v));
  }
}

void IGraphics::AttachBackground(const char* name)
{
  IBitmap bg = LoadIBitmap(name);
  mControls.Insert(0, new IBitmapControl(mPlug, 0, 0, -1, bg, IChannelBlend::kBlendClobber));
}

void IGraphics::AttachPanelBackground(const IColor& color)
{
  IControl* pBG = new IPanelControl(mPlug, IRECT(0, 0, mWidth, mHeight), color);
  mControls.Insert(0, pBG);
}

int IGraphics::AttachControl(IControl* pControl)
{
  mControls.Add(pControl);
  return mControls.GetSize() - 1;
}

void IGraphics::AttachKeyCatcher(IControl& control)
{
  mKeyCatcher = &control;
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
    IParam* pParam = mPlug.GetParam(paramIdx);
    lo = pParam->GetNormalized(lo);
    hi = pParam->GetNormalized(hi);
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

void IGraphics::SetParameterFromPlug(int paramIdx, double value, bool normalized)
{
  if (!normalized)
  {
    IParam* pParam = mPlug.GetParam(paramIdx);
    value = pParam->GetNormalized(value);
  }
  int i, n = mControls.GetSize();
  IControl** ppControl = mControls.GetList();
  for (i = 0; i < n; ++i, ++ppControl)
  {
    IControl* pControl = *ppControl;
    if (pControl->ParamIdx() == paramIdx)
    {
      pControl->SetValueFromPlug(value);
      // Could be more than one, don't break until we check them all.
    }
    
    // now look for any auxilliary parameters
    int auxParamIdx = pControl->AuxParamIdx(paramIdx);
    
    if (auxParamIdx > -1) // there are aux params
    {
      pControl->SetAuxParamValueFromPlug(auxParamIdx, value);
    }
  }
}

void IGraphics::SetControlFromPlug(int controlIdx, double normalizedValue)
{
  if (controlIdx >= 0 && controlIdx < mControls.GetSize())
  {
    mControls.Get(controlIdx)->SetValueFromPlug(normalizedValue);
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

void IGraphics::SetParameterFromGUI(int paramIdx, double normalizedValue)
{
  int i, n = mControls.GetSize();
  IControl** ppControl = mControls.GetList();
  for (i = 0; i < n; ++i, ++ppControl) {
    IControl* pControl = *ppControl;
    if (pControl->ParamIdx() == paramIdx) {
      pControl->SetValueFromUserInput(normalizedValue);
      // Could be more than one, don't break until we check them all.
    }
  }
}

void IGraphics::PromptUserInput(IControl* pControl, IParam* pParam, IRECT& textRect)
{
  if (!pControl || !pParam) return;

  IParam::EParamType type = pParam->Type();
  int n = pParam->GetNDisplayTexts();
  char currentText[MAX_PARAM_LEN];

  if ( type == IParam::kTypeEnum || (type == IParam::kTypeBool && n))
  {
    pParam->GetDisplayForHost(currentText);
    IPopupMenu menu;

    // Fill the menu
    for (int i = 0; i < n; ++i)
    {
      const char* str = pParam->GetDisplayText(i);
      // TODO: what if two parameters have the same text?
      if (!strcmp(str, currentText)) // strings are equal
        menu.AddItem( new IPopupMenuItem(str, IPopupMenuItem::kChecked), -1 );
      else // not equal
        menu.AddItem( new IPopupMenuItem(str), -1 );
    }

    if(CreateIPopupMenu(menu, textRect))
    {
      pControl->SetValueFromUserInput(pParam->GetNormalized( (double) menu.GetChosenItemIdx() ));
    }
  }
  // TODO: what if there are Int/Double Params with a display text e.g. -96db = "mute"
  else // type == IParam::kTypeInt || type == IParam::kTypeDouble
  {
    pParam->GetDisplayForHostNoDisplayText(currentText);
    CreateTextEntry(pControl, pControl->GetText(), textRect, currentText, pParam);
  }

}

void IGraphics::DrawBitmap(IBitmap& bitmap, const IRECT& rect, int bmpState, const IChannelBlend* pBlend)
{
  int srcX = 0;
  int srcY = 0;

  if (bitmap.N > 1 && bmpState > 1)
  {
    if (bitmap.mFramesAreHorizontal)
    {
      srcX = int(0.5 + (double) bitmap.W * (double) (bmpState - 1) / (double) bitmap.N);
    }
    else
    {
      srcY = int(0.5 + (double) bitmap.H * (double) (bmpState - 1) / (double) bitmap.N);
    }
  }
  return DrawBitmap(bitmap, rect, srcX, srcY, pBlend);
}

void IGraphics::DrawRect(const IColor& color, const IRECT& rect)
{
  DrawHorizontalLine(color, rect.T, rect.L, rect.R);
  DrawHorizontalLine(color, rect.B, rect.L, rect.R);
  DrawVerticalLine(color, rect.L, rect.T, rect.B);
  DrawVerticalLine(color, rect.R, rect.T, rect.B);
}

void IGraphics::DrawVerticalLine(const IColor& color, const IRECT& rect, float x)
{
  x = BOUNDED(x, 0.0f, 1.0f);
  int xi = rect.L + int(x * (float) (rect.R - rect.L));
  return DrawVerticalLine(color, xi, rect.T, rect.B);
}

void IGraphics::DrawHorizontalLine(const IColor& color, const IRECT& rect, float y)
{
  y = BOUNDED(y, 0.0f, 1.0f);
  int yi = rect.B - int(y * (float) (rect.B - rect.T));
  return DrawHorizontalLine(color, yi, rect.L, rect.R);
}

void IGraphics::DrawVerticalLine(const IColor& color, int xi, int yLo, int yHi)
{
  DrawLine(color, xi, yLo, xi, yHi);
}

void IGraphics::DrawHorizontalLine(const IColor& color, int yi, int xLo, int xHi)
{
  DrawLine(color, xLo, yi, xHi, yi);
}

void IGraphics::DrawRadialLine(const IColor& color, float cx, float cy, float angle, float rMin, float rMax, bool aa)
{
  float sinV = sinf(angle);
  float cosV = cosf(angle);
  float xLo = (cx + rMin * sinV);
  float xHi = (cx + rMax * sinV);
  float yLo = (cy - rMin * cosV);
  float yHi = (cy - rMax * cosV);
  return DrawLine(color, xLo, yLo, xHi, yHi, 0, aa);
}

bool IGraphics::IsDirty(IRECT& rect)
{
#ifndef NDEBUG
  if (mShowControlBounds)
  {
    rect = mDrawRECT;
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
      rect = rect.Union(pControl->GetRECT());
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
void IGraphics::Draw(const IRECT& rect)
{
  int i, j, n = mControls.GetSize();
  if (!n)
    return;

  if (mStrict)
  {
    mDrawRECT = rect;
    int n = mControls.GetSize();
    IControl** ppControl = mControls.GetList();
    for (int i = 0; i < n; ++i, ++ppControl)
    {
      IControl* pControl = *ppControl;
      if (!(pControl->IsHidden()) && rect.Intersects(pControl->GetRECT()))
      {
        ClipRegion(mDrawRECT);
        pControl->Draw(*this);
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
          ClipRegion(pControl2->GetRECT());
          pControl2->Draw(*this);
          pControl2->SetClean();
          ResetClipRegion();
        }
      }
    }
    else
    {
      for (i = 1; i < n; ++i)   // loop through all controls starting from one (not bg)
      {
        IControl* pControl = mControls.Get(i); // assign control i to pControl
        if (pControl->IsDirty())   // if pControl is dirty
        {
          mDrawRECT = pControl->GetRECT(); // put the rect in the mDrawRect member variable
          for (j = 0; j < n; ++j)   // loop through all controls
          {
            IControl* pControl2 = mControls.Get(j); // assign control j to pControl2

            // if control1 == control2 OR control2 is not hidden AND control2's rect intersects mDrawRect
            if (!pControl2->IsHidden() && (i == j || pControl2->GetRECT().Intersects(mDrawRECT)))
            {
              ClipRegion(mDrawRECT);
              pControl2->Draw(*this);
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
    FillIRect(c, rect);
  }
  
  if (mShowControlBounds) 
  {
    for (int j = 1; j < mControls.GetSize(); j++)
    {
      IControl* pControl = mControls.Get(j);
      DrawRect(CONTROL_BOUNDS_COLOR, pControl->GetRECT());
    }
    
    WDL_String str;
    str.SetFormatted(32, "x: %i, y: %i", mMouseX, mMouseY);
    IText txt(20, CONTROL_BOUNDS_COLOR);
    IRECT rect(Width() - 150, Height() - 20, Width(), Height());
//    DrawIText(txt, str.Get(), rect);
  }
#endif

  DrawScreen(rect);
}

void IGraphics::SetStrictDrawing(bool strict)
{
  mStrict = strict;
  SetAllControlsDirty();
}

void IGraphics::OnMouseDown(int x, int y, const IMouseMod& mod)
{
  ReleaseMouseCapture();
  int c = GetMouseControlIdx(x, y);
  if (c >= 0)
  {
    mMouseCapture = c;
    mMouseX = x;
    mMouseY = y;

    IControl* pControl = mControls.Get(c);
    int paramIdx = pControl->ParamIdx();

    #if defined OS_WIN || defined VST3_API  // on Mac, IGraphics.cpp is not compiled in a static library, so this can be #ifdef'd
    if (mPlug.GetAPI() == kAPIVST3)
    {
      if (mod.R && paramIdx >= 0)
      {
        ReleaseMouseCapture();
        mPlug.PopupHostContextMenuForParam(paramIdx, x, y);
        return;
      }
    }
    #endif
    
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
    
    if (paramIdx >= 0)
    {
      mPlug.BeginInformHostOfParamChange(paramIdx);
    }
        
    pControl->OnMouseDown(x, y, mod);
  }
}

void IGraphics::OnMouseUp(int x, int y, const IMouseMod& mod)
{
  int c = GetMouseControlIdx(x, y);
  mMouseCapture = mMouseX = mMouseY = -1;
  if (c >= 0)
  {
    IControl* pControl = mControls.Get(c);
    pControl->OnMouseUp(x, y, mod);
    pControl = mControls.Get(c); // needed if the mouse message caused a resize/rebuild
    int paramIdx = pControl->ParamIdx();
    if (paramIdx >= 0)
    {
      mPlug.EndInformHostOfParamChange(paramIdx);
    }
  }
}

bool IGraphics::OnMouseOver(int x, int y, const IMouseMod& mod)
{
  if (mHandleMouseOver)
  {
    int c = GetMouseControlIdx(x, y, true);
    if (c >= 0)
    {
      mMouseX = x;
      mMouseY = y;
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

void IGraphics::OnMouseOut()
{
  int i, n = mControls.GetSize();
  IControl** ppControl = mControls.GetList();
  for (i = 0; i < n; ++i, ++ppControl)
  {
    IControl* pControl = *ppControl;
    pControl->OnMouseOut();
  }
  mMouseOver = -1;
}

void IGraphics::OnMouseDrag(int x, int y, const IMouseMod& mod)
{
  int c = mMouseCapture;
  if (c >= 0)
  {
    int dX = x - mMouseX;
    int dY = y - mMouseY;
    if (dX != 0 || dY != 0)
    {
      mMouseX = x;
      mMouseY = y;
      mControls.Get(c)->OnMouseDrag(x, y, dX, dY, mod);
    }
  }
}

bool IGraphics::OnMouseDblClick(int x, int y, const IMouseMod& mod)
{
  ReleaseMouseCapture();
  bool newCapture = false;
  int c = GetMouseControlIdx(x, y);
  if (c >= 0)
  {
    IControl* pControl = mControls.Get(c);
    if (pControl->MouseDblAsSingleClick())
    {
      mMouseCapture = c;
      mMouseX = x;
      mMouseY = y;
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

void IGraphics::OnMouseWheel(int x, int y, const IMouseMod& mod, int d)
{
  int c = GetMouseControlIdx(x, y);
  if (c >= 0)
  {
    mControls.Get(c)->OnMouseWheel(x, y, mod, d);
  }
}

void IGraphics::ReleaseMouseCapture()
{
  mMouseCapture = mMouseX = mMouseY = -1;
}

bool IGraphics::OnKeyDown(int x, int y, int key)
{
  int c = GetMouseControlIdx(x, y);
  if (c > 0)
    return mControls.Get(c)->OnKeyDown(x, y, key);
  else if (mKeyCatcher)
    return mKeyCatcher->OnKeyDown(x, y, key);
  else
    return false;
}

int IGraphics::GetMouseControlIdx(int x, int y, bool mo)
{
  if (mMouseCapture >= 0)
  {
    return mMouseCapture;
  }

  bool allow; // this is so that mouseovers can still be called when a control is greyed out

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
      allow = !pControl->IsGrayed();
    }

    if (!pControl->IsHidden() && allow && pControl->IsHit(x, y))
    {
      return i;
    }
  }
  return -1;
}

int IGraphics::GetParamIdxForPTAutomation(int x, int y)
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
  int idx = mLastClickedParam;

  mLastClickedParam = -1;

  return idx;
}

void IGraphics::OnGUIIdle()
{
  int i, n = mControls.GetSize();
  IControl** ppControl = mControls.GetList();
  for (i = 0; i < n; ++i, ++ppControl)
  {
    IControl* pControl = *ppControl;
    pControl->OnGUIIdle();
  }
}

void IGraphics::ReScale()
{
  int i, n = mControls.GetSize();
  IControl** ppControl = mControls.GetList();
  for (i = 0; i < n; ++i, ++ppControl)
  {
    IControl* pControl = *ppControl;
    pControl->OnRescale();
  }
  
  SetAllControlsDirty();
}

IBitmap IGraphics::GetScaledBitmap(IBitmap& src)
{
  return LoadIBitmap(src.mResourceName.Get(), src.N, src.mFramesAreHorizontal, src.mSourceScale);
}
