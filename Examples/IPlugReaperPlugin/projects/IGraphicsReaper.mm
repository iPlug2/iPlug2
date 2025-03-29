
#include "IGraphicsReaper.h"

#pragma warning( push )
#pragma warning( disable : 4244 )
#include "include/core/SkBitmap.h"

#include "IGraphicsMacUtilities.mm"

using namespace iplug;
using namespace igraphics;

// Platform Utilities

namespace PlatformUtilities = IGraphicsMacUtilities;

// Cursor

void IGraphicsReaper::GetMouseLocation(float& x, float&y) const
{
  x = mCursorX;
  y = mCursorY;
}

void IGraphicsReaper::HideMouseCursor(bool hide, bool lock)
{
  if (mCursorHidden == hide)
    return;
  
  mCursorHidden = hide;
  
  if (hide)
  {
    StoreCursorPosition(mCursorX, mCursorY);
    PlatformUtilities::HideCursor(hide);
    mCursorLock = lock;
  }
  else
  {
    DoCursorLock(mCursorX, mCursorY, mCursorX, mCursorY);
    PlatformUtilities::HideCursor(hide);
    mCursorLock = false;
  }
}

void IGraphicsReaper::MoveMouseCursor(float x, float y)
{
  if (mTabletInput)
    return;
    
  StoreCursorPosition(x, y);
  PointToScreen(x, y);
  PlatformUtilities::RepositionCursor(x, y);
}

void IGraphicsReaper::DoCursorLock(float x, float y, float& prevX, float& prevY)
{
  if (mCursorHidden && mCursorLock && !mTabletInput)
  {
    PlatformUtilities::RepositionCursor(mCursorLockPositionX, mCursorLockPositionY);
    prevX = mCursorX;
    prevY = mCursorY;
  }
  else
  {
    mCursorX = prevX = x;
    mCursorY = prevY = y;
  }
}

void IGraphicsReaper::StoreCursorPosition(float x, float y)
{
  PlatformUtilities::GetCursorPosition(mCursorLockPositionX, mCursorLockPositionY);
  
  mCursorX = x;
  mCursorY = y;
}

// FIX - not implemented

ECursor IGraphicsReaper::SetMouseCursor(ECursor cursorType)
{
  return IGraphics::SetMouseCursor(cursorType);
}

// FIX - this currently won't work (although the logic is correct)

void IGraphicsReaper::PointToScreen(float& x, float& y) const
{
}

// Clipboard

bool IGraphicsReaper::GetTextFromClipboard(WDL_String& str)
{
  return PlatformUtilities::GetTextFromClipboard(str);
}

bool IGraphicsReaper::SetTextInClipboard(const char* str)
{
  return PlatformUtilities::SetTextInClipboard(str);
}

bool IGraphicsReaper::SetFilePathInClipboard(const char* path)
{
  return PlatformUtilities::SetFilePathInClipboard(path);
}

// Reveal

bool IGraphicsReaper::RevealPathInExplorerOrFinder(WDL_String& path, bool select)
{
  return PlatformUtilities::RevealPathInExplorerOrFinder(path, select);
}

bool IGraphicsReaper::OpenURL(const char* url, const char* msgWindowTitle, const char* confirmMsg, const char* errMsgOnFailure)
{
  return PlatformUtilities::OpenURL(url, msgWindowTitle, confirmMsg, errMsgOnFailure);
}

// Font Loading

extern StaticStorage<PlatformUtilities::FontType> sFontDescriptorCache;

PlatformFontPtr IGraphicsReaper::LoadPlatformFont(const char* fontID, const char* fileNameOrResID)
{
  return PlatformUtilities::LoadPlatformFont(fontID, fileNameOrResID, GetBundleID(), GetSharedResourcesSubPath());
}

PlatformFontPtr IGraphicsReaper::LoadPlatformFont(const char* fontID, const char* fontName, ETextStyle style)
{
  return PlatformUtilities::LoadPlatformFont(fontID, fontName, style);
}

PlatformFontPtr IGraphicsReaper::LoadPlatformFont(const char* fontID, void* pData, int dataSize)
{
  return PlatformUtilities::LoadPlatformFont(fontID, pData, dataSize);
}

void IGraphicsReaper::CachePlatformFont(const char* fontID, const PlatformFontPtr& font)
{
  PlatformUtilities::CachePlatformFont(fontID, font, sFontDescriptorCache);
}

// Constructor / Destructor

IGraphicsReaper::IGraphicsReaper(IGEditorDelegate& dlg, int w, int h, int fps, float scale)
: IGraphicsSkia(dlg, w, h, fps, scale, true)
{
  PlatformUtilities::GUILoad();
  StaticStorage<PlatformUtilities::FontType>::Accessor storage(sFontDescriptorCache);
  storage.Retain();
}

IGraphicsReaper::~IGraphicsReaper()
{
  StaticStorage<PlatformUtilities::FontType>::Accessor storage(sFontDescriptorCache);
  storage.Release();
  
  CloseWindow();
}

// Open Window

void* IGraphicsReaper::OpenWindow(void* pWindow)
{
  mOpen = true;
  GetDelegate()->LayoutUI(this);
  return nullptr;
}

// Draw Loop

int IGraphicsReaper::DrawEmbedded(REAPER_FXEMBED_IBitmap* pBitmap, REAPER_FXEMBED_DrawInfo* pInfo)
{
  auto GetContext = [](REAPER_FXEMBED_DrawInfo* pInfo)
  {
    switch (pInfo->context)
    {
      case 1:   return EmbeddedContext::TCP;
      case 2:   return EmbeddedContext::MCP;
      default:  return EmbeddedContext::Unknown;
    }
  };

  // Resize or rescale if required
  
  const float screenScale = pInfo->dpi / 256.f;
  const int width = pInfo->width / screenScale;
  const int height = pInfo->height / screenScale;

  const EmbeddedContext context = GetContext(pInfo);
  
  if (context != mEmbeddedContext)
  {
    mEmbeddedContext = context;
    GetDelegate()->LayoutUI(this);
  }
  
  if (screenScale != GetScreenScale())
    SetScreenScale(screenScale);
  
  if (width != Width() || height != Height())
    Resize(width, height, 1.f);
  
  // Do Drawing
  
  IRECTList rects;

  if (IsDirty(rects))
  {
    SetAllControlsClean();
    Draw(rects);
  }
  else if (pInfo->flags & REAPER_FXEMBED_DRAWINFO_FLAG_PAINT_OPTIONAL)
  {
    return 0;
  }
  
  // Copy to the output bitmap
  
  if (pBitmap && mSurface)
  {
    SkPixmap pixmap;
    mSurface->peekPixels(&pixmap);
    
    SkBitmap output;
    SkImageInfo info = SkImageInfo::Make(pBitmap->getWidth(), pBitmap->getHeight(), kBGRA_8888_SkColorType, kPremul_SkAlphaType);
    
    size_t rowBytes = pBitmap->getRowSpan() * sizeof(unsigned int);
    output.installPixels(info, pBitmap->getBits(), rowBytes);
    
    // Copy to the output
    
    output.writePixels(pixmap);
  }
  
  return 1;
}

// Draw Resize

void IGraphicsReaper::DrawResize()
{
  auto w = static_cast<int>(std::ceil(static_cast<float>(WindowWidth()) * GetScreenScale()));
  auto h = static_cast<int>(std::ceil(static_cast<float>(WindowHeight()) * GetScreenScale()));
  
  mSurface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(w, h));

  if (mSurface)
  {
    mCanvas = mSurface->getCanvas();
    mCanvas->save();
  }
}

void DebugMouse(const char *str, int msg, int msgL, int msgR, const IMouseInfo& info)
{
  const char* tag = "";
  
  if (msg == msgL)
    tag = "Left ";
  else if (msg == msgR)
    tag = "Right ";
   
  DBGMSG("%s%s %f %f %f %F %d %d %d %d %d\n", tag, str, info.x, info.y, info.dX, info.dY, info.ms.L, info.ms.R, info.ms.A, info.ms.C, info.ms.S);
}

// UI Proc

IMouseInfo IGraphicsReaper::GetMouseInfo(int message, void* pMsg)
{
  auto* pInfo = reinterpret_cast<REAPER_FXEMBED_DrawInfo*>(pMsg);
  
  const float scale = GetTotalScale();

  const float x = pInfo->mouse_x / scale;
  const float y = pInfo->mouse_y / scale;
  const float dX = mCursorX >= 0.f ? x - mCursorX : 0.f;
  const float dY = mCursorY >= 0.f ? y - mCursorY : 0.f;
  
  bool l = pInfo->flags & REAPER_FXEMBED_DRAWINFO_FLAG_LBUTTON_CAPTURED;
  bool r = pInfo->flags & REAPER_FXEMBED_DRAWINFO_FLAG_RBUTTON_CAPTURED;

  switch (message)
  {
    case REAPER_FXEMBED_WM_LBUTTONDOWN:
      l = true;
      break;
      
    case REAPER_FXEMBED_WM_RBUTTONDOWN:
      r = true;
      break;
      
    default:
      break;
  }
  
  DoCursorLock(x, y, mCursorX, mCursorY);
  
  return IMouseInfo{x, y, dX, dY, PlatformUtilities::GetMouseModifiers(l, r)};
}

void IGReaperEditorDelegate::LayoutUI(IGraphics* pGraphics)
{
  if (mLayoutFunc)
  {
    pGraphics->RemoveAllControls();
    mLayoutFunc(pGraphics);
  }
}

int IGReaperEditorDelegate::EmbeddedUIProc(int message, void* pMsg1, void* pMsg2)
{
  auto GetGraphics = [&, this]() { return static_cast<IGraphicsReaper*>(GetUI()); };
  
  switch (message)
  {
    case REAPER_FXEMBED_WM_IS_SUPPORTED:
      return 1;
            
    case REAPER_FXEMBED_WM_CREATE:
      OpenWindow(nullptr);
      GetUI()->SetLayoutOnResize(true);
      return 1;
      
    case REAPER_FXEMBED_WM_DESTROY:
      CloseWindow();
      return 1;
      
    //case WM_SETCURSOR: return HCURSOR;
      
    case REAPER_FXEMBED_WM_MOUSEMOVE:
    {
      auto info = GetGraphics()->GetMouseInfo(message, pMsg2);
      std::vector<IMouseInfo> list {info};
      
      if (info.ms.L || info.ms.R)
      {
        DebugMouse("Drag", message, 0, 0, info);
        GetUI()->OnMouseDrag(list);
        return REAPER_FXEMBED_RETNOTIFY_INVALIDATE;
      }
      
      if (GetUI()->OnMouseOver(info.x, info.y, info.ms))
        return REAPER_FXEMBED_RETNOTIFY_INVALIDATE;
      else
        return 0;
    }
      
    case REAPER_FXEMBED_WM_LBUTTONDOWN:
    case REAPER_FXEMBED_WM_RBUTTONDOWN:
    {
      std::vector<IMouseInfo> list { GetGraphics()->GetMouseInfo(message, pMsg2) };
      DebugMouse("Down", message, REAPER_FXEMBED_WM_LBUTTONDOWN, REAPER_FXEMBED_WM_RBUTTONDOWN, list[0]);
      GetUI()->OnMouseDown(list);
      return 0;
    }

    case REAPER_FXEMBED_WM_LBUTTONUP:
    case REAPER_FXEMBED_WM_RBUTTONUP:
    {
      std::vector<IMouseInfo> list { GetGraphics()->GetMouseInfo(message, pMsg2) };
      DebugMouse("Up", message, REAPER_FXEMBED_WM_LBUTTONUP, REAPER_FXEMBED_WM_RBUTTONUP, list[0]);
      GetUI()->OnMouseUp(list);
      return 0;
    }
      
    case REAPER_FXEMBED_WM_LBUTTONDBLCLK:
    case REAPER_FXEMBED_WM_RBUTTONDBLCLK:
    {
      auto info = GetGraphics()->GetMouseInfo(message, pMsg2);
     
      DebugMouse("Double Click", message, REAPER_FXEMBED_WM_LBUTTONDBLCLK, REAPER_FXEMBED_WM_RBUTTONDBLCLK, info);
      
      if (GetUI()->OnMouseDblClick(info.x, info.y, info.ms))
        return REAPER_FXEMBED_RETNOTIFY_INVALIDATE;
      else
        return 0;
    }
      
    case REAPER_FXEMBED_WM_MOUSEWHEEL:
    {
      static const float wheelStep = 120.f;
      auto info = GetGraphics()->GetMouseInfo(message, pMsg2);
      auto* pInfo = reinterpret_cast<REAPER_FXEMBED_DrawInfo*>(pMsg2);
     
      DebugMouse("Mouse Wheel", message, 0L, 0, info);

      if (GetUI()->OnMouseWheel(info.x, info.y, info.ms, pInfo->mousewheel_amt / wheelStep))
        return REAPER_FXEMBED_RETNOTIFY_INVALIDATE;
      else
        return 0;
    }
      
    case REAPER_FXEMBED_WM_GETMINMAXINFO:
    {
      auto* pInfo = reinterpret_cast<REAPER_FXEMBED_SizeHints*>(pMsg2);
      
      pInfo->preferred_aspect = mPreferredAspect;
      pInfo->minimum_aspect = mMinimumAspect;
      pInfo->min_width = mMinWidth;
      pInfo->min_height = mMinHeight;
      pInfo->max_width = mMaxWidth;
      pInfo->max_height = mMaxHeight;
     
      return 1;
    }
      
    case REAPER_FXEMBED_WM_PAINT:
    {
      //REAPER_FXEMBED_EXT_GET_ADVISORY_SCALING
      
      auto* pBitmap = reinterpret_cast<REAPER_FXEMBED_IBitmap*>(pMsg1);
      auto* pInfo = reinterpret_cast<REAPER_FXEMBED_DrawInfo*>(pMsg2);

      return GetGraphics()->DrawEmbedded(pBitmap, pInfo);
    }
      
    default:
      return 0;
  }
}

int GetAspect(int num, int denom)
{
  return num * 65536 / (denom ? denom : 1);
}

void IGReaperEditorDelegate::SetPreferredAspect(int num, int denom)
{
  mPreferredAspect = GetAspect(num, denom);
}

void IGReaperEditorDelegate::SetMinimumAspect(int num, int denom)
{
  mMinimumAspect = GetAspect(num, denom);
}

void IGReaperEditorDelegate::SetMinSize(int width, int height)
{
  mMinWidth = width;
  mMinHeight = height;
}

void IGReaperEditorDelegate::SetMaxSize(int width, int height)
{
  mMaxWidth = width;
  mMaxHeight = height;
}

#ifndef IGRAPHICS_SKIA
  #include "IGraphicsSkia.cpp"
#endif
