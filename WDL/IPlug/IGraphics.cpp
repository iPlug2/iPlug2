#include "IGraphics.h"

#define DEFAULT_FPS 25

// If not dirty for this many timer ticks, we call OnGUIIDle.
// Only looked at if USE_IDLE_CALLS is defined.
#define IDLE_TICKS 20

#ifndef CONTROL_BOUNDS_COLOR
  #define CONTROL_BOUNDS_COLOR COLOR_GREEN
#endif

class BitmapStorage
{
public:

  struct BitmapKey
  {
    int id;
    LICE_IBitmap* bitmap;
  };

  WDL_PtrList<BitmapKey> m_bitmaps;
  WDL_Mutex m_mutex;

  LICE_IBitmap* Find(int id)
  {
    WDL_MutexLock lock(&m_mutex);
    int i, n = m_bitmaps.GetSize();
    for (i = 0; i < n; ++i)
    {
      BitmapKey* key = m_bitmaps.Get(i);
      if (key->id == id) return key->bitmap;
    }
    return 0;
  }

  void Add(LICE_IBitmap* bitmap, int id = -1)
  {
    WDL_MutexLock lock(&m_mutex);
    BitmapKey* key = m_bitmaps.Add(new BitmapKey);
    key->id = id;
    key->bitmap = bitmap;
  }

  void Remove(LICE_IBitmap* bitmap)
  {
    WDL_MutexLock lock(&m_mutex);
    int i, n = m_bitmaps.GetSize();
    for (i = 0; i < n; ++i)
    {
      if (m_bitmaps.Get(i)->bitmap == bitmap)
      {
        m_bitmaps.Delete(i, true);
        delete(bitmap);
        break;
      }
    }
  }

  ~BitmapStorage()
  {
    int i, n = m_bitmaps.GetSize();
    for (i = 0; i < n; ++i)
    {
      delete(m_bitmaps.Get(i)->bitmap);
    }
    m_bitmaps.Empty(true);
  }
};

static BitmapStorage s_bitmapCache;

class FontStorage
{
public:

  struct FontKey
  {
    int size, orientation;
    IText::EStyle style;
    char face[FONT_LEN];
    LICE_IFont* font;
  };

  WDL_PtrList<FontKey> m_fonts;
  WDL_Mutex m_mutex;

  LICE_IFont* Find(IText* pTxt)
  {
    WDL_MutexLock lock(&m_mutex);
    int i = 0, n = m_fonts.GetSize();
    for (i = 0; i < n; ++i)
    {
      FontKey* key = m_fonts.Get(i);
      if (key->size == pTxt->mSize && key->orientation == pTxt->mOrientation && key->style == pTxt->mStyle && !strcmp(key->face, pTxt->mFont)) return key->font;
    }
    return 0;
  }

  void Add(LICE_IFont* font, IText* pTxt)
  {
    WDL_MutexLock lock(&m_mutex);
    FontKey* key = m_fonts.Add(new FontKey);
    key->size = pTxt->mSize;
    key->orientation = pTxt->mOrientation;
    key->style = pTxt->mStyle;
    strcpy(key->face, pTxt->mFont);
    key->font = font;
  }

  ~FontStorage()
  {
    int i, n = m_fonts.GetSize();
    for (i = 0; i < n; ++i)
    {
      delete(m_fonts.Get(i)->font);
    }
    m_fonts.Empty(true);
  }
};

static FontStorage s_fontCache;

inline LICE_pixel LiceColor(const IColor* pColor)
{
  return LICE_RGBA(pColor->R, pColor->G, pColor->B, pColor->A);
}

inline float LiceWeight(const IChannelBlend* pBlend)
{
  return (pBlend ? pBlend->mWeight : 1.0f);
}

inline int LiceBlendMode(const IChannelBlend* pBlend)
{
  if (!pBlend)
  {
    return LICE_BLIT_MODE_COPY | LICE_BLIT_USE_ALPHA;
  }
  switch (pBlend->mMethod)
  {
    case IChannelBlend::kBlendClobber:
    {
      return LICE_BLIT_MODE_COPY;
    }
    case IChannelBlend::kBlendAdd:
    {
      return LICE_BLIT_MODE_ADD | LICE_BLIT_USE_ALPHA;
    }
    case IChannelBlend::kBlendColorDodge:
    {
      return LICE_BLIT_MODE_DODGE | LICE_BLIT_USE_ALPHA;
    }
    case IChannelBlend::kBlendNone:
    default:
    {
      return LICE_BLIT_MODE_COPY | LICE_BLIT_USE_ALPHA;
    }
  }
}

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
  , mDrawBitmap(0)
  , mTmpBitmap(0)
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
  DELETE_NULL(mDrawBitmap);
  DELETE_NULL(mTmpBitmap);
}

void IGraphics::Resize(int w, int h)
{
  mWidth = w;
  mHeight = h;
  ReleaseMouseCapture();
  mControls.Empty(true);
  DELETE_NULL(mDrawBitmap);
  DELETE_NULL(mTmpBitmap);
  PrepDraw();
  mPlug->ResizeGraphics(w, h);
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

IBitmap IGraphics::LoadIBitmap(int ID, const char* name, int nStates, bool framesAreHoriztonal)
{
  LICE_IBitmap* lb = s_bitmapCache.Find(ID);
  if (!lb)
  {
    lb = OSLoadBitmap(ID, name);
    #ifndef NDEBUG
    bool imgResourceFound = lb;
    #endif
    assert(imgResourceFound); // Protect against typos in resource.h and .rc files.
    s_bitmapCache.Add(lb, ID);
  }
  return IBitmap(lb, lb->getWidth(), lb->getHeight(), nStates, framesAreHoriztonal);
}

void IGraphics::RetainBitmap(IBitmap* pBitmap)
{
  s_bitmapCache.Add((LICE_IBitmap*)pBitmap->mData);
}

void IGraphics::ReleaseBitmap(IBitmap* pBitmap)
{
  s_bitmapCache.Remove((LICE_IBitmap*)pBitmap->mData);
}

void IGraphics::PrepDraw()
{
  mDrawBitmap = new LICE_SysBitmap(Width(), Height());
  mTmpBitmap = new LICE_MemBitmap();
}

bool IGraphics::DrawBitmap(IBitmap* pIBitmap, IRECT* pDest, int srcX, int srcY, const IChannelBlend* pBlend)
{
  LICE_IBitmap* pLB = (LICE_IBitmap*) pIBitmap->mData;
  IRECT r = pDest->Intersect(&mDrawRECT);
  srcX += r.L - pDest->L;
  srcY += r.T - pDest->T;
  _LICE::LICE_Blit(mDrawBitmap, pLB, r.L, r.T, srcX, srcY, r.W(), r.H(), LiceWeight(pBlend), LiceBlendMode(pBlend));
  return true;
}

bool IGraphics::DrawRotatedBitmap(IBitmap* pIBitmap, int destCtrX, int destCtrY, double angle, int yOffsetZeroDeg,
                                  const IChannelBlend* pBlend)
{
  LICE_IBitmap* pLB = (LICE_IBitmap*) pIBitmap->mData;

  //double dA = angle * PI / 180.0;
  // Can't figure out what LICE_RotatedBlit is doing for irregular bitmaps exactly.
  //double w = (double) bitmap.W;
  //double h = (double) bitmap.H;
  //double sinA = fabs(sin(dA));
  //double cosA = fabs(cos(dA));
  //int W = int(h * sinA + w * cosA);
  //int H = int(h * cosA + w * sinA);

  int W = pIBitmap->W;
  int H = pIBitmap->H;
  int destX = destCtrX - W / 2;
  int destY = destCtrY - H / 2;

  _LICE::LICE_RotatedBlit(mDrawBitmap, pLB, destX, destY, W, H, 0.0f, 0.0f, (float) W, (float) H, (float) angle,
                          false, LiceWeight(pBlend), LiceBlendMode(pBlend) | LICE_BLIT_FILTER_BILINEAR, 0.0f, (float) yOffsetZeroDeg);

  return true;
}

bool IGraphics::DrawRotatedMask(IBitmap* pIBase, IBitmap* pIMask, IBitmap* pITop, int x, int y, double angle,
                                const IChannelBlend* pBlend)
{
  LICE_IBitmap* pBase = (LICE_IBitmap*) pIBase->mData;
  LICE_IBitmap* pMask = (LICE_IBitmap*) pIMask->mData;
  LICE_IBitmap* pTop = (LICE_IBitmap*) pITop->mData;

  double dA = angle * PI / 180.0;
  int W = pIBase->W;
  int H = pIBase->H;
  //	RECT srcR = { 0, 0, W, H };
  float xOffs = (W % 2 ? -0.5f : 0.0f);

  if (!mTmpBitmap)
  {
    mTmpBitmap = new LICE_MemBitmap();
  }
  _LICE::LICE_Copy(mTmpBitmap, pBase);
  _LICE::LICE_ClearRect(mTmpBitmap, 0, 0, W, H, LICE_RGBA(255, 255, 255, 0));

  _LICE::LICE_RotatedBlit(mTmpBitmap, pMask, 0, 0, W, H, 0.0f, 0.0f, (float) W, (float) H, (float) dA,
                          true, 1.0f, LICE_BLIT_MODE_ADD | LICE_BLIT_FILTER_BILINEAR | LICE_BLIT_USE_ALPHA, xOffs, 0.0f);
  _LICE::LICE_RotatedBlit(mTmpBitmap, pTop, 0, 0, W, H, 0.0f, 0.0f, (float) W, (float) H, (float) dA,
                          true, 1.0f, LICE_BLIT_MODE_COPY | LICE_BLIT_FILTER_BILINEAR | LICE_BLIT_USE_ALPHA, xOffs, 0.0f);

  IRECT r = IRECT(x, y, x + W, y + H).Intersect(&mDrawRECT);
  _LICE::LICE_Blit(mDrawBitmap, mTmpBitmap, r.L, r.T, r.L - x, r.T - y, r.R - r.L, r.B - r.T,
                   LiceWeight(pBlend), LiceBlendMode(pBlend));
  //	ReaperExt::LICE_Blit(mDrawBitmap, mTmpBitmap, x, y, &srcR, LiceWeight(pBlend), LiceBlendMode(pBlend));
  return true;
}

bool IGraphics::DrawPoint(const IColor* pColor, float x, float y,
                          const IChannelBlend* pBlend, bool antiAlias)
{
  float weight = (pBlend ? pBlend->mWeight : 1.0f);
  _LICE::LICE_PutPixel(mDrawBitmap, int(x + 0.5f), int(y + 0.5f), LiceColor(pColor), weight, LiceBlendMode(pBlend));
  return true;
}

bool IGraphics::ForcePixel(const IColor* pColor, int x, int y)
{
  LICE_pixel* px = mDrawBitmap->getBits();
  px += x + y * mDrawBitmap->getRowSpan();
  *px = LiceColor(pColor);
  return true;
}

bool IGraphics::DrawLine(const IColor* pColor, float x1, float y1, float x2, float y2,
                         const IChannelBlend* pBlend, bool antiAlias)
{
  _LICE::LICE_Line(mDrawBitmap, (int) x1, (int) y1, (int) x2, (int) y2, LiceColor(pColor), LiceWeight(pBlend), LiceBlendMode(pBlend), antiAlias);
  return true;
}

bool IGraphics::DrawArc(const IColor* pColor, float cx, float cy, float r, float minAngle, float maxAngle,
                        const IChannelBlend* pBlend, bool antiAlias)
{
  _LICE::LICE_Arc(mDrawBitmap, cx, cy, r, minAngle, maxAngle, LiceColor(pColor),
                  LiceWeight(pBlend), LiceBlendMode(pBlend), antiAlias);
  return true;
}

bool IGraphics::DrawCircle(const IColor* pColor, float cx, float cy, float r,
                           const IChannelBlend* pBlend, bool antiAlias)
{
  _LICE::LICE_Circle(mDrawBitmap, cx, cy, r, LiceColor(pColor), LiceWeight(pBlend), LiceBlendMode(pBlend), antiAlias);
  return true;
}

bool IGraphics::RoundRect(const IColor* pColor, IRECT* pR, const IChannelBlend* pBlend, int cornerradius, bool aa)
{
  _LICE::LICE_RoundRect(mDrawBitmap, (float) pR->L, (float) pR->T, (float) pR->W(), (float) pR->H(), cornerradius,
                        LiceColor(pColor), LiceWeight(pBlend), LiceBlendMode(pBlend), aa);
  return true;
}

bool IGraphics::FillRoundRect(const IColor* pColor, IRECT* pR, const IChannelBlend* pBlend, int cornerradius, bool aa)
{
  int x1 = pR->L;
  int y1 = pR->T;
  int h = pR->H();
  int w = pR->W();
  
  int mode = LiceBlendMode(pBlend);
  float weight = LiceWeight(pBlend);
  LICE_pixel color = LiceColor(pColor);
  
  _LICE::LICE_FillRect(mDrawBitmap, x1+cornerradius, y1, w-2*cornerradius, h, color, weight, mode);
  _LICE::LICE_FillRect(mDrawBitmap, x1, y1+cornerradius, cornerradius, h-2*cornerradius,color, weight, mode);
  _LICE::LICE_FillRect(mDrawBitmap, x1+w-cornerradius, y1+cornerradius, cornerradius, h-2*cornerradius, color, weight, mode);

  //void LICE_FillCircle(LICE_IBitmap* dest, float cx, float cy, float r, LICE_pixel color, float alpha, int mode, bool aa)
  _LICE::LICE_FillCircle(mDrawBitmap, (float) x1+cornerradius, (float) y1+cornerradius, (float) cornerradius, color, weight, mode, aa);
  _LICE::LICE_FillCircle(mDrawBitmap, (float) x1+w-cornerradius-1, (float) y1+h-cornerradius-1, (float) cornerradius, color, weight, mode, aa);
  _LICE::LICE_FillCircle(mDrawBitmap, (float) x1+w-cornerradius-1, (float) y1+cornerradius, (float) cornerradius, color, weight, mode, aa);
  _LICE::LICE_FillCircle(mDrawBitmap, (float) x1+cornerradius, (float) y1+h-cornerradius-1, (float) cornerradius, color, weight, mode, aa);
  
  return true;
}

bool IGraphics::FillIRect(const IColor* pColor, IRECT* pR, const IChannelBlend* pBlend)
{
  _LICE::LICE_FillRect(mDrawBitmap, pR->L, pR->T, pR->W(), pR->H(), LiceColor(pColor), LiceWeight(pBlend), LiceBlendMode(pBlend));
  return true;
}

bool IGraphics::FillCircle(const IColor* pColor, int cx, int cy, float r, const IChannelBlend* pBlend, bool antiAlias)
{
  _LICE::LICE_FillCircle(mDrawBitmap, (float) cx, (float) cy, r, LiceColor(pColor), LiceWeight(pBlend), LiceBlendMode(pBlend), antiAlias);
  return true;
}

bool IGraphics::FillTriangle(const IColor* pColor, int x1, int y1, int x2, int y2, int x3, int y3, IChannelBlend* pBlend)
{
  _LICE::LICE_FillTriangle(mDrawBitmap, x1, y1, x2, y2, x3, y3, LiceColor(pColor), LiceWeight(pBlend), LiceBlendMode(pBlend));
  return true;
}

bool IGraphics::FillIConvexPolygon(const IColor* pColor, int* x, int* y, int npoints, const IChannelBlend* pBlend)
{
  _LICE::LICE_FillConvexPolygon(mDrawBitmap, x, y, npoints, LiceColor(pColor), LiceWeight(pBlend), LiceBlendMode(pBlend));
  return true;
}

IColor IGraphics::GetPoint(int x, int y)
{
  LICE_pixel pix = _LICE::LICE_GetPixel(mDrawBitmap, x, y);
  return IColor(LICE_GETA(pix), LICE_GETR(pix), LICE_GETG(pix), LICE_GETB(pix));
}

bool IGraphics::DrawVerticalLine(const IColor* pColor, int xi, int yLo, int yHi)
{
  _LICE::LICE_Line(mDrawBitmap, xi, yLo, xi, yHi, LiceColor(pColor), 1.0f, LICE_BLIT_MODE_COPY, false);
  return true;
}

bool IGraphics::DrawHorizontalLine(const IColor* pColor, int yi, int xLo, int xHi)
{
  _LICE::LICE_Line(mDrawBitmap, xLo, yi, xHi, yi, LiceColor(pColor), 1.0f, LICE_BLIT_MODE_COPY, false);
  return true;
}

IBitmap IGraphics::ScaleBitmap(IBitmap* pIBitmap, int destW, int destH)
{
  LICE_IBitmap* pSrc = (LICE_IBitmap*) pIBitmap->mData;
  LICE_MemBitmap* pDest = new LICE_MemBitmap(destW, destH);
  _LICE::LICE_ScaledBlit(pDest, pSrc, 0, 0, destW, destH, 0.0f, 0.0f, (float) pIBitmap->W, (float) pIBitmap->H, 1.0f,
                         LICE_BLIT_MODE_COPY | LICE_BLIT_FILTER_BILINEAR);

  IBitmap bmp(pDest, destW, destH, pIBitmap->N);
  RetainBitmap(&bmp);
  return bmp;
}

IBitmap IGraphics::CropBitmap(IBitmap* pIBitmap, IRECT* pR)
{
  int destW = pR->W(), destH = pR->H();
  LICE_IBitmap* pSrc = (LICE_IBitmap*) pIBitmap->mData;
  LICE_MemBitmap* pDest = new LICE_MemBitmap(destW, destH);
  _LICE::LICE_Blit(pDest, pSrc, 0, 0, pR->L, pR->T, destW, destH, 1.0f, LICE_BLIT_MODE_COPY);

  IBitmap bmp(pDest, destW, destH, pIBitmap->N);
  RetainBitmap(&bmp);
  return bmp;
}

LICE_pixel* IGraphics::GetBits()
{
  return mDrawBitmap->getBits();
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

bool IGraphics::DrawRadialLine(const IColor* pColor, float cx, float cy, float angle, float rMin, float rMax,
                               const IChannelBlend* pBlend, bool antiAlias)
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

bool IGraphics::DrawIText(IText* pTxt, char* str, IRECT* pR, bool measure)
{
  if (!str || str[0] == '\0')
  {
    return true;
  }

  LICE_IFont* font = pTxt->mCached;
  
  if (!font)
  {
    font = CacheFont(pTxt);
    if (!font) return false;
  }

  LICE_pixel color = LiceColor(&pTxt->mColor);
  font->SetTextColor(color);

  UINT fmt = DT_NOCLIP;
  if (LICE_GETA(color) < 255) fmt |= LICE_DT_USEFGALPHA;
  if (pTxt->mAlign == IText::kAlignNear)
    fmt |= DT_LEFT;
  else if (pTxt->mAlign == IText::kAlignCenter)
    fmt |= DT_CENTER;
  else // if (pTxt->mAlign == IText::kAlignFar)
    fmt |= DT_RIGHT;

  if (measure) 
  {
    fmt |= DT_CALCRECT;
    RECT R = {0,0,0,0};
    font->DrawText(mDrawBitmap, str, -1, &R, fmt);
    
    if( pTxt->mAlign == IText::kAlignNear)
    {
      pR->R = R.right;
    }
    else if (pTxt->mAlign == IText::kAlignCenter)
    {
      pR->L = (int) pR->MW() - (R.right/2);
      pR->R = pR->L + R.right;
    }
    else // (pTxt->mAlign == IText::kAlignFar)
    {
      pR->L = pR->R - R.right;
      pR->R = pR->L + R.right;
    }
    
    pR->B = pR->T + R.bottom;
  }
  else 
  {
    RECT R = { pR->L, pR->T, pR->R, pR->B };
    font->DrawText(mDrawBitmap, str, -1, &R, fmt);
  }

  return true;
}

LICE_IFont* IGraphics::CacheFont(IText* pTxt)
{
  LICE_CachedFont* font = (LICE_CachedFont*)s_fontCache.Find(pTxt);
  if (!font)
  {
    font = new LICE_CachedFont;
    int h = pTxt->mSize;
    int esc = 10 * pTxt->mOrientation;
    int wt = (pTxt->mStyle == IText::kStyleBold ? FW_BOLD : FW_NORMAL);
    int it = (pTxt->mStyle == IText::kStyleItalic ? TRUE : FALSE);

    int q;
    if (pTxt->mQuality == IText::kQualityDefault)
      q = DEFAULT_QUALITY;
    #ifdef CLEARTYPE_QUALITY
    else if (pTxt->mQuality == IText::kQualityClearType)
      q = CLEARTYPE_QUALITY;
    else if (pTxt->mQuality == IText::kQualityAntiAliased)
    #else
    else if (pTxt->mQuality != IText::kQualityNonAntiAliased)
    #endif
      q = ANTIALIASED_QUALITY;
    else // if (pTxt->mQuality == IText::kQualityNonAntiAliased)
      q = NONANTIALIASED_QUALITY;

    #ifdef __APPLE__
    bool resized = false;
    Resize:
    if (h < 2) h = 2;
    #endif
    HFONT hFont = CreateFont(h, 0, esc, esc, wt, it, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, q, DEFAULT_PITCH, pTxt->mFont);
    if (!hFont)
    {
      delete(font);
      return 0;
    }
    font->SetFromHFont(hFont, LICE_FONT_FLAG_OWNS_HFONT | LICE_FONT_FLAG_FORCE_NATIVE);
    #ifdef __APPLE__
    if (!resized && font->GetLineHeight() != h)
    {
      h = int((double)(h * h) / (double)font->GetLineHeight() + 0.5);
      resized = true;
      goto Resize;
    }
    #endif
    s_fontCache.Add(font, pTxt);
  }
  pTxt->mCached = font;
  return font;
}