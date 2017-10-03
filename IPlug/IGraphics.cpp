#include "IGraphics.h"

#define DEFAULT_FPS 25

// If not dirty for this many timer ticks, we call OnGUIIDle.
// Only looked at if USE_IDLE_CALLS is defined.
#define IDLE_TICKS 20

#ifdef AAX_API
static uint32_t GetAAXModifiersFromIMouseMod(const IMouseMod& pMod)
{
  uint32_t aax_mods = 0;
  
  if (pMod.A) aax_mods |= AAX_eModifiers_Option; // ALT Key on Windows, ALT/Option key on mac
  
#ifdef OS_WIN
  if (pMod.C) aax_mods |= AAX_eModifiers_Command;
#else
  if (pMod.C) aax_mods |= AAX_eModifiers_Control;
  if (pMod.R) aax_mods |= AAX_eModifiers_Command;
#endif
  if (pMod.S) aax_mods |= AAX_eModifiers_Shift;
  if (pMod.R) aax_mods |= AAX_eModifiers_SecondaryButton;
  
  return aax_mods;
}
#endif

#ifndef CONTROL_BOUNDS_COLOR
  #define CONTROL_BOUNDS_COLOR COLOR_GREEN
#endif

IGraphics::IGraphics(IPlugBase* pPlug, int w, int h, int refreshFPS)
  : mPlug(pPlug)
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
  , mKeyCatcher(0)
  , mCursorHidden(false)
  , mHiddenMousePointX(-1)
  , mHiddenMousePointY(-1)
  , mEnableTooltips(false)
  , mShowControlBounds(false)
{
  mFPS = (refreshFPS > 0 ? refreshFPS : DEFAULT_FPS);
}

IGraphics::~IGraphics()
{
  if (mKeyCatcher)
    DELETE_NULL(mKeyCatcher);

  mControls.Empty(true);
}

void IGraphics::Resize(int w, int h)
{
//  mWidth = w;
//  mHeight = h;
//  ReleaseMouseCapture();
//  mControls.Empty(true);
//  DELETE_NULL(mDrawBitmap);
//  DELETE_NULL(mTmpBitmap);
//  PrepDraw();
//  mPlug->ResizeGraphics(w, h);
}

void IGraphics::SetFromStringAfterPrompt(IControl* pControl, IParam* pParam, char *txt)
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
      if (pParam->DisplayIsNegated()) v = -v;
    }

    pControl->SetValueFromUserInput(pParam->GetNormalized(v));
  }
}


void IGraphics::AttachBackground(int ID, const char* name)
{
  IBitmap bg = LoadIBitmap(ID, name);
  IControl* pBG = new IBitmapControl(mPlug, 0, 0, -1, &bg, IChannelBlend::kBlendClobber);
  mControls.Insert(0, pBG);
}

void IGraphics::AttachPanelBackground(const IColor *pColor)
{
  IControl* pBG = new IPanelControl(mPlug, IRECT(0, 0, mWidth, mHeight), pColor);
  mControls.Insert(0, pBG);
}

int IGraphics::AttachControl(IControl* pControl)
{
  mControls.Add(pControl);
  return mControls.GetSize() - 1;
}

void IGraphics::AttachKeyCatcher(IControl* pControl)
{
  mKeyCatcher = pControl;
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
    IParam* pParam = mPlug->GetParam(paramIdx);
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
    IParam* pParam = mPlug->GetParam(paramIdx);
    value = pParam->GetNormalized(value);
  }
  int i, n = mControls.GetSize();
  IControl** ppControl = mControls.GetList();
  for (i = 0; i < n; ++i, ++ppControl)
  {
    IControl* pControl = *ppControl;
    if (pControl->ParamIdx() == paramIdx)
    {
      //WDL_MutexLock lock(&mMutex);
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
    //WDL_MutexLock lock(&mMutex);
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

void IGraphics::PromptUserInput(IControl* pControl, IParam* pParam, IRECT* pTextRect)
{
  if (!pControl || !pParam || !pTextRect) return;

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

    if(CreateIPopupMenu(&menu, pTextRect))
    {
      pControl->SetValueFromUserInput(pParam->GetNormalized( (double) menu.GetChosenItemIdx() ));
    }
  }
  // TODO: what if there are Int/Double Params with a display text e.g. -96db = "mute"
  else // type == IParam::kTypeInt || type == IParam::kTypeDouble
  {
    pParam->GetDisplayForHostNoDisplayText(currentText);
    CreateTextEntry(pControl, pControl->GetText(), pTextRect, currentText, pParam );
  }

}

bool IGraphics::DrawBitmap(IBitmap* pBitmap, IRECT* pR, int bmpState, const IChannelBlend* pBlend)
{
  int srcX = 0;
  int srcY = 0;

  if (pBitmap->N > 1 && bmpState > 1)
  {
    if (pBitmap->mFramesAreHorizontal)
    {
      srcX = int(0.5 + (double) pBitmap->W * (double) (bmpState - 1) / (double) pBitmap->N);
    }
    else
    {
      srcY = int(0.5 + (double) pBitmap->H * (double) (bmpState - 1) / (double) pBitmap->N);
    }
  }
  return DrawBitmap(pBitmap, pR, srcX, srcY, pBlend);
}

bool IGraphics::DrawRect(const IColor* pColor, IRECT* pR)
{
  bool rc = DrawHorizontalLine(pColor, pR->T, pR->L, pR->R);
  rc &= DrawHorizontalLine(pColor, pR->B, pR->L, pR->R);
  rc &= DrawVerticalLine(pColor, pR->L, pR->T, pR->B);
  rc &= DrawVerticalLine(pColor, pR->R, pR->T, pR->B);
  return rc;
}

bool IGraphics::DrawVerticalLine(const IColor* pColor, IRECT* pR, float x)
{
  x = BOUNDED(x, 0.0f, 1.0f);
  int xi = pR->L + int(x * (float) (pR->R - pR->L));
  return DrawVerticalLine(pColor, xi, pR->T, pR->B);
}

bool IGraphics::DrawHorizontalLine(const IColor* pColor, IRECT* pR, float y)
{
  y = BOUNDED(y, 0.0f, 1.0f);
  int yi = pR->B - int(y * (float) (pR->B - pR->T));
  return DrawHorizontalLine(pColor, yi, pR->L, pR->R);
}

bool IGraphics::DrawRadialLine(const IColor* pColor, float cx, float cy, float angle, float rMin, float rMax, const IChannelBlend* pBlend, bool antiAlias)
{
  float sinV = sin(angle);
  float cosV = cos(angle);
  float xLo = cx + rMin * sinV;
  float xHi = cx + rMax * sinV;
  float yLo = cy - rMin * cosV;
  float yHi = cy - rMax * cosV;
  return DrawLine(pColor, xLo, yLo, xHi, yHi, pBlend, antiAlias);
}

bool IGraphics::IsDirty(IRECT* pR)
{
#ifndef NDEBUG
  if (mShowControlBounds)
  {
    *pR = mDrawRECT;
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
      *pR = pR->Union(pControl->GetRECT());
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
bool IGraphics::Draw(IRECT* pR)
{
//  #pragma REMINDER("Mutex set while drawing")
//  WDL_MutexLock lock(&mMutex);

  int i, j, n = mControls.GetSize();
  if (!n)
  {
    return true;
  }

  if (mStrict)
  {
    mDrawRECT = *pR;
    int n = mControls.GetSize();
    IControl** ppControl = mControls.GetList();
    for (int i = 0; i < n; ++i, ++ppControl)
    {
      IControl* pControl = *ppControl;
      if (!(pControl->IsHidden()) && pR->Intersects(pControl->GetRECT()))
      {
        pControl->Draw(this);
      }
      pControl->SetClean();
    }
  }
  else
  {
    IControl* pBG = mControls.Get(0);
    if (pBG->IsDirty())   // Special case when everything needs to be drawn.
    {
      mDrawRECT = *(pBG->GetRECT());
      for (int j = 0; j < n; ++j)
      {
        IControl* pControl2 = mControls.Get(j);
        if (!j || !(pControl2->IsHidden()))
        {
          pControl2->Draw(this);
          pControl2->SetClean();
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

          // printf("control %i is Dirty\n", i);

          mDrawRECT = *(pControl->GetRECT()); // put the rect in the mDrawRect member variable
          for (j = 0; j < n; ++j)   // loop through all controls
          {
            IControl* pControl2 = mControls.Get(j); // assign control j to pControl2

            // if control1 == control2 OR control2 is not hidden AND control2's rect intersects mDrawRect
            if (!pControl2->IsHidden() && (i == j || pControl2->GetRECT()->Intersects(&mDrawRECT)))
            {
              //if ((i == j) && (!pControl2->IsHidden())|| (!(pControl2->IsHidden()) && pControl2->GetRECT()->Intersects(&mDrawRECT))) {
              //printf("control %i and %i \n", i, j);

              pControl2->Draw(this);
            }
          }
          pControl->SetClean();
        }
      }
    }
  }

#ifndef NDEBUG
  if (mShowControlBounds) 
  {
    for (int j = 1; j < mControls.GetSize(); j++)
    {
      IControl* pControl = mControls.Get(j);
      DrawRect(&CONTROL_BOUNDS_COLOR, pControl->GetRECT());
    }
    
    WDL_String str;
    str.SetFormatted(32, "x: %i, y: %i", mMouseX, mMouseY);
    IText txt(20, &CONTROL_BOUNDS_COLOR);
    IRECT rect(Width() - 150, Height() - 20, Width(), Height());
    DrawIText(&txt, str.Get(), &rect);
  }
#endif

  return DrawScreen(pR);
}

void IGraphics::SetStrictDrawing(bool strict)
{
  mStrict = strict;
  SetAllControlsDirty();
}

void IGraphics::OnMouseDown(int x, int y, IMouseMod* pMod)
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
    if (mPlug->GetAPI() == kAPIVST3)
    {
      if (pMod->R && paramIdx >= 0)
      {
        ReleaseMouseCapture();
        mPlug->PopupHostContextMenuForParam(paramIdx, x, y);
        return;
      }
    }
    #endif
    
    #ifdef AAX_API
    if (mAAXViewContainer && paramIdx >= 0)
    {
      uint32_t mods = GetAAXModifiersFromIMouseMod(pMod);
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
      mPlug->BeginInformHostOfParamChange(paramIdx);
    }
        
    pControl->OnMouseDown(x, y, pMod);
  }
}

void IGraphics::OnMouseUp(int x, int y, IMouseMod* pMod)
{
  int c = GetMouseControlIdx(x, y);
  mMouseCapture = mMouseX = mMouseY = -1;
  if (c >= 0)
  {
    IControl* pControl = mControls.Get(c);
    pControl->OnMouseUp(x, y, pMod);
    pControl = mControls.Get(c); // needed if the mouse message caused a resize/rebuild
    int paramIdx = pControl->ParamIdx();
    if (paramIdx >= 0)
    {
      mPlug->EndInformHostOfParamChange(paramIdx);
    }
  }
}

bool IGraphics::OnMouseOver(int x, int y, IMouseMod* pMod)
{
  if (mHandleMouseOver)
  {
    int c = GetMouseControlIdx(x, y, true);
    if (c >= 0)
    {
      mMouseX = x;
      mMouseY = y;
      mControls.Get(c)->OnMouseOver(x, y, pMod);
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

void IGraphics::OnMouseDrag(int x, int y, IMouseMod* pMod)
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
      mControls.Get(c)->OnMouseDrag(x, y, dX, dY, pMod);
    }
  }
}

bool IGraphics::OnMouseDblClick(int x, int y, IMouseMod* pMod)
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
      pControl->OnMouseDown(x, y, pMod);
      newCapture = true;
    }
    else
    {
      pControl->OnMouseDblClick(x, y, pMod);
    }
  }
  return newCapture;
}

void IGraphics::OnMouseWheel(int x, int y, IMouseMod* pMod, int d)
{
  int c = GetMouseControlIdx(x, y);
  if (c >= 0)
  {
    mControls.Get(c)->OnMouseWheel(x, y, pMod, d);
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
