
#include "IGraphics.h"

#define NANOSVG_IMPLEMENTATION
#include <cstdio>
#include "nanosvg.h"

#ifdef VST3_API
#include "IPlugVST3.h"
#include "pluginterfaces/base/ustring.h"
#endif

#ifndef NDEBUG
#include "IGraphicsLiveEdit.h"
#endif

struct SVGHolder
{
  NSVGimage* mImage = nullptr;
  
  SVGHolder(NSVGimage* image)
  : mImage(image)
  {
  }
  
  ~SVGHolder()
  {
    if(mImage)
      nsvgDelete(mImage);
    
    mImage = nullptr;
  }
};

static StaticStorage<SVGHolder> s_SVGCache;

IGraphics::IGraphics(IPlugBaseGraphics& plug, int w, int h, int fps)
: mPlug(plug)
, mWidth(w)
, mHeight(h)
{
  mFPS = (fps > 0 ? fps : DEFAULT_FPS);
}

IGraphics::~IGraphics()
{
  if (mKeyCatcher)
    DELETE_NULL(mKeyCatcher);
  
#if !defined(NDEBUG) && defined(SA_API)
  if (mLiveEdit)
    DELETE_NULL(mLiveEdit);
#endif
  
  mControls.Empty(true);
}

void IGraphics::Resize(int w, int h, double scale)
{
  ReleaseMouseCapture();

  double oldScale = mScale;
  mScale = scale;
    
  if (oldScale != scale)
      ReScale();
    
  for (int i = 0; i < mPlug.NParams(); ++i)
    SetParameterFromPlug(i, mPlug.GetParam(i)->GetNormalized(), true);
    
  mPlug.ResizeGraphics(w, h, scale);
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

void IGraphics::AttachBackground(const char* name, double scale)
{
  IBitmap bg = LoadIBitmap(name, 1, false, scale);
  mControls.Insert(0, new IBitmapControl(mPlug, 0, 0, -1, bg, IBlend::kBlendClobber));
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

void IGraphics::DrawBitmap(IBitmap& bitmap, const IRECT& rect, int bmpState, const IBlend* pBlend)
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

void IGraphics::DrawBitmapedText(IBitmap& bitmap, IRECT& rect, IText& text, IBlend* pBlend, const char* str, bool vCenter, bool multiline, int charWidth, int charHeight, int charOffset)
{
  if (CSTR_NOT_EMPTY(str))
  {
    int stringLength = (int) strlen(str);
    
    float basicYOffset, basicXOffset;
    
    if (vCenter)
      basicYOffset = rect.T + ((rect.H() - charHeight) / 2.);
    else
      basicYOffset = rect.T;
    
    if (text.mAlign == IText::kAlignCenter)
      basicXOffset = rect.L + ((rect.W() - (stringLength * charWidth)) / 2.);
    else if (text.mAlign == IText::kAlignNear)
      basicXOffset = rect.L;
    else if (text.mAlign == IText::kAlignFar)
      basicXOffset = rect.R - (stringLength * charWidth);
    
    int widthAsOneLine = charWidth * stringLength;
    
    int nLines;
    int stridx = 0;
    
    int nCharsThatFitIntoLine;
    
    if(multiline)
    {
      if (widthAsOneLine > rect.W())
      {
        nCharsThatFitIntoLine = rect.W() / charWidth;
        nLines = (widthAsOneLine / rect.W()) + 1;
      }
      else // line is shorter than width of rect
      {
        nCharsThatFitIntoLine = stringLength;
        nLines = 1;
      }
    }
    else
    {
      nCharsThatFitIntoLine = rect.W() / charWidth;
      nLines = 1;
    }
    
    for(int line=0; line<nLines; line++)
    {
      float yOffset = basicYOffset + line * charHeight;
      
      for(int linepos=0; linepos<nCharsThatFitIntoLine; linepos++)
      {
        if (str[stridx] == '\0') return;
        
        int frameOffset = (int) str[stridx++] - 31; // calculate which frame to look up
        
        int xOffset = (linepos * (charWidth + charOffset)) + basicXOffset;    // calculate xOffset for character we're drawing
        IRECT charRect = IRECT(xOffset, yOffset, xOffset + charWidth, yOffset + charHeight);
        DrawBitmap(bitmap, charRect, frameOffset, pBlend);
      }
    }
  }
}

void IGraphics::DrawVerticalLine(const IColor& color, const IRECT& rect, float x, const IBlend* pBlend)
{
  x = BOUNDED(x, 0.0f, 1.0f);
  int xi = rect.L + int(x * (float) (rect.R - rect.L));
  return DrawVerticalLine(color, xi, rect.T, rect.B, pBlend);
}

void IGraphics::DrawHorizontalLine(const IColor& color, const IRECT& rect, float y, const IBlend* pBlend)
{
  y = BOUNDED(y, 0.0f, 1.0f);
  int yi = rect.B - int(y * (float) (rect.B - rect.T));
  return DrawHorizontalLine(color, yi, rect.L, rect.R, pBlend);
}

void IGraphics::DrawVerticalLine(const IColor& color, int xi, int yLo, int yHi, const IBlend* pBlend)
{
  DrawLine(color, xi, yLo, xi, yHi, pBlend);
}

void IGraphics::DrawHorizontalLine(const IColor& color, int yi, int xLo, int xHi, const IBlend* pBlend)
{
  DrawLine(color, xLo, yi, xHi, yi, pBlend);
}

void IGraphics::DrawRadialLine(const IColor& color, float cx, float cy, float angle, float rMin, float rMax, bool aa, const IBlend* pBlend)
{
  float sinV = sinf(angle);
  float cosV = cosf(angle);
  float xLo = (cx + rMin * sinV);
  float xHi = (cx + rMax * sinV);
  float yLo = (cy - rMin * cosV);
  float yHi = (cy - rMax * cosV);
  return DrawLine(color, xLo, yLo, xHi, yHi, pBlend, aa);
}

void IGraphics::DrawGrid(const IColor& color, const IRECT& rect, int gridSizeH, int gridSizeV, const IBlend* pBlend)
{
  // Vertical Lines grid
  if (gridSizeH > 1)
  {
    for (int x = 0; x < rect.W(); x += gridSizeH)
    {
      DrawVerticalLine(color, rect, (float)x/(float) rect.W(), pBlend);
    }
  }
    // Horizontal Lines grid
  if (gridSizeV > 1)
  {
    for (int y = 0; y < rect.H(); y += gridSizeV)
    {
      DrawHorizontalLine(color, rect, (float)y/(float) rect.H(), pBlend);
    }
  }
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
  int n = mControls.GetSize();
  
  if (!n)
    return;

  if (mStrict)
  {
    mDrawRECT = rect;
    IControl** ppControl = mControls.GetList();
    for (auto i = 0; i < n; ++i, ++ppControl)
    {
      IControl* pControl = *ppControl;
      if (!(pControl->IsHidden()) && rect.Intersects(pControl->GetRECT()))
      {
        ClipRegion(mDrawRECT);
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
          ClipRegion(pControl2->GetRECT());
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
          mDrawRECT = pControl->GetRECT(); // put the rect in the mDrawRect member variable
          for (auto j = 0; j < n; ++j)   // loop through all controls
          {
            IControl* pControl2 = mControls.Get(j); // assign control j to pControl2

            // if control1 == control2 OR control2 is not hidden AND control2's rect intersects mDrawRect
            if (!pControl2->IsHidden() && (i == j || pControl2->GetRECT().Intersects(mDrawRECT)))
            {
              ClipRegion(mDrawRECT);
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
    FillIRect(c, rect);
  }
  
  if(mShowControlBounds)
  {
    for (int j = 1; j < mControls.GetSize(); j++)
    {
      IControl* pControl = mControls.Get(j);
      DrawRect(CONTROL_BOUNDS_COLOR, pControl->GetRECT());
    }
  }
  
#if defined(SA_API)
  if(mLiveEdit)
    mLiveEdit->Draw(*this);
#endif
//  WDL_String str;
//  str.SetFormatted(32, "x: %i, y: %i", mMouseX, mMouseY);
  IText txt(20, CONTROL_BOUNDS_COLOR);
  txt.mAlign = IText::kAlignNear;
  IRECT r;
//  DrawIText(txt, str.Get(), r);
  MeasureIText(txt, GetDrawingAPIStr(), r);
  FillIRect(COLOR_BLACK, r);
  DrawIText(txt, GetDrawingAPIStr(), r);

#endif

  if (!GetPlatformContext())
    return;
        
  //TODO: this is silly, adapt api
  RenderAPIBitmap(GetPlatformContext());
}

void IGraphics::SetStrictDrawing(bool strict)
{
  mStrict = strict;
  SetAllControlsDirty();
}

void IGraphics::OnMouseDown(int x, int y, const IMouseMod& mod)
{
#if !defined(NDEBUG) && defined(SA_API)
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
    mMouseX = x;
    mMouseY = y;

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
    {
      mPlug.BeginInformHostOfParamChange(paramIdx);
    }
        
    pControl->OnMouseDown(x, y, mod);
  }
}

void IGraphics::OnMouseUp(int x, int y, const IMouseMod& mod)
{
#if !defined(NDEBUG) && defined(SA_API)
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
      mPlug.EndInformHostOfParamChange(paramIdx);
    }
  }
}

bool IGraphics::OnMouseOver(int x, int y, const IMouseMod& mod)
{
#if !defined(NDEBUG) && defined(SA_API)
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
#if !defined(NDEBUG) && defined(SA_API)
  if(mLiveEdit)
  {
    mLiveEdit->OnMouseDrag(x, y, 0, 0, mod);
    return;
  }
#endif
  
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

void IGraphics::PopupHostContextMenuForParam(int controlIdx, int paramIdx, int x, int y)
{
  IPopupMenu contextMenu;
  IControl* pControl = GetControl(controlIdx);
  
  if(pControl)
  {
    pControl->CreateContextMenu(contextMenu);
    
    if(!contextMenu.GetNItems())
      return;
    
#ifdef VST3_API
    
    IPlugVST3* pVST3 = dynamic_cast<IPlugVST3*>(&mPlug);
    
    if (!pVST3->GetComponentHandler() || !pVST3->GetView())
      return;
    
    Steinberg::FUnknownPtr<Steinberg::Vst::IComponentHandler3>handler(pVST3->GetComponentHandler() );
    
    if (handler == 0)
      return;
    
    Steinberg::Vst::ParamID p = paramIdx;
    
    Steinberg::Vst::IContextMenu* menu = handler->createContextMenu(pVST3->GetView(), &p);
    
    if (menu)
    {
      Steinberg::Vst::IContextMenu::Item item = {0};
      
      for (int i = 0; i < contextMenu.GetNItems(); i++)
      {
        Steinberg::UString128 (contextMenu.GetItemText(i)).copyTo (item.name, 128);
        item.tag = i;
        
        if (!contextMenu.GetItem(i)->GetEnabled())
          item.flags = Steinberg::Vst::IContextMenu::Item::kIsDisabled;
        else
          item.flags = 0;
        
        menu->addItem(item, GetControl(controlIdx));
      }
      
      menu->popup((Steinberg::UCoord) x,(Steinberg::UCoord) y);
      menu->release();
    }
    
    
#else
    CreateIPopupMenu(contextMenu, x, y);
    pControl->OnContextSelection(contextMenu.GetChosenItemIdx());
#endif
  }
  return;
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
    pControl->OnResize();
  }
  
  SetAllControlsDirty();
}

IBitmap IGraphics::GetScaledBitmap(IBitmap& src)
{
  return LoadIBitmap(src.mResourceName.Get(), src.N, src.mFramesAreHorizontal, src.mSourceScale);
}

void IGraphics::OnDrop(const char* str, int x, int y)
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
#if !defined(NDEBUG) && defined(SA_API)
  if(enable)
    mLiveEdit = new IGraphicsLiveEdit(GetPlug(), file, gridsize);
  else {
    if(mLiveEdit)
      DELETE_NULL(mLiveEdit);
  }
#endif
}

ISVG IGraphics::LoadISVG(const char* name)
{
#ifdef OS_OSX
  WDL_String path;
  bool found = OSFindResource(name, "svg", path);
  assert(found);
  
  SVGHolder* pHolder = s_SVGCache.Find(path.Get());
  
  if(!pHolder)
  {
    NSVGimage* pImage = nsvgParseFromFile(path.Get(), "px", 72);
    pHolder  = new SVGHolder(pImage);
    s_SVGCache.Add(pHolder, path.Get());
  }
  
  return ISVG(pHolder->mImage);
#else
  return ISVG();
#endif
}
