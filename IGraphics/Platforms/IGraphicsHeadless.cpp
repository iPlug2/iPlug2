/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include "IGraphicsHeadless.h"
#include "IPlugPaths.h"

#include "include/core/SkImage.h"
#include "include/encode/SkPngEncoder.h"
#include "include/core/SkStream.h"

#if defined(OS_MAC) || defined(OS_IOS)
#include "IGraphicsCoreText.h"
#endif

using namespace iplug;
using namespace igraphics;

#if !defined(OS_MAC) && !defined(OS_IOS)
// Simple font wrapper for headless mode on non-Apple platforms
// Stores font data and returns it via GetFontData() for use by IGraphicsSkia
class HeadlessFont : public PlatformFont
{
public:
  HeadlessFont(void* pData, int dataSize, int faceIdx)
  : PlatformFont(false)
  , mFontData(new IFontData(pData, dataSize, faceIdx))
  {}

  IFontDataPtr GetFontData() override
  {
    // Create a copy of the font data to return
    if (mFontData && mFontData->IsValid())
      return IFontDataPtr(new IFontData(mFontData->Get(), mFontData->GetSize(), mFontData->GetFaceIdx()));
    return IFontDataPtr(new IFontData());
  }

private:
  IFontDataPtr mFontData;
};
#endif

IGraphicsHeadless::IGraphicsHeadless(IGEditorDelegate& dlg, int w, int h, int fps, float scale)
: IGRAPHICS_DRAW_CLASS(dlg, w, h, fps, scale)
{
  DBGMSG("IGraphics Headless (CPU)\n");
}

IGraphicsHeadless::~IGraphicsHeadless()
{
  CloseWindow();
}

void* IGraphicsHeadless::OpenWindow(void* pWindow)
{
  InitHeadless();
  return GetWindow();
}

void IGraphicsHeadless::CloseWindow()
{
  mWindowOpen = false;
}

void IGraphicsHeadless::InitHeadless(float scale)
{
  if (mWindowOpen)
    return;

  mWindowOpen = true;

  // Set screen scale (1.0 = standard DPI, 2.0 = retina/HiDPI)
  SetScreenScale(scale);

  // This triggers DrawResize() which creates the Skia CPU surface
  OnViewInitialized(nullptr);
  GetDelegate()->LayoutUI(this);
  GetDelegate()->OnUIOpen();
}

void IGraphicsHeadless::SetScale(float scale)
{
  if (!mWindowOpen)
    return;

  // Update the screen scale and resize to recreate the surface
  SetScreenScale(scale);

  // Trigger a resize to recreate the Skia surface at the new scale
  int w = WindowWidth();
  int h = WindowHeight();
  Resize(w, h, scale, true);

  // Re-layout the UI at the new size
  GetDelegate()->LayoutUI(this);
}

void IGraphicsHeadless::RenderUI()
{
  if (!mWindowOpen)
    return;

  // Mark all controls dirty and draw
  SetAllControlsDirty();

  IRECT bounds = GetBounds();
  IRECTList rects;
  rects.Add(bounds);

  Draw(rects);
}

float IGraphicsHeadless::MeasureText(const IText& text, const char* str, IRECT& bounds) const
{
  // Use the base class implementation from IGraphicsSkia
  return IGRAPHICS_DRAW_CLASS::MeasureText(text, str, bounds);
}

PlatformFontPtr IGraphicsHeadless::LoadPlatformFont(const char* fontID, const char* fileNameOrResID)
{
#if defined(OS_MAC) || defined(OS_IOS)
  return CoreTextHelpers::LoadPlatformFont(fontID, fileNameOrResID, GetBundleID(), GetSharedResourcesSubPath());
#else
  // Linux/Windows: Use LocateResource to find the font file, then load from data
  WDL_String fullPath;
  EResourceLocation loc = LocateResource(fileNameOrResID, "ttf", fullPath, GetBundleID(), GetWinModuleHandle(), GetSharedResourcesSubPath());

  if (loc == EResourceLocation::kNotFound)
    return nullptr;

  // Read the font file into memory
  FILE* f = fopen(fullPath.Get(), "rb");
  if (!f)
    return nullptr;

  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);

  void* pData = malloc(size);
  if (pData && fread(pData, 1, size, f) == static_cast<size_t>(size))
  {
    fclose(f);
    PlatformFontPtr font = LoadPlatformFont(fontID, pData, static_cast<int>(size));
    free(pData);
    return font;
  }

  fclose(f);
  if (pData)
    free(pData);
  return nullptr;
#endif
}

PlatformFontPtr IGraphicsHeadless::LoadPlatformFont(const char* fontID, const char* fontName, ETextStyle style)
{
#if defined(OS_MAC) || defined(OS_IOS)
  return CoreTextHelpers::LoadPlatformFont(fontID, fontName, style);
#else
  // System font loading is not supported in headless mode on non-Apple platforms.
  return nullptr;
#endif
}

PlatformFontPtr IGraphicsHeadless::LoadPlatformFont(const char* fontID, void* pData, int dataSize)
{
#if defined(OS_MAC) || defined(OS_IOS)
  return CoreTextHelpers::LoadPlatformFont(fontID, pData, dataSize);
#else
  // Use default face index 0 for headless font loading
  return PlatformFontPtr(new HeadlessFont(pData, dataSize, 0));
#endif
}

bool IGraphicsHeadless::SaveScreenshot(const char* path)
{
  sk_sp<SkSurface>& surface = GetSurface();
  if (!surface)
    return false;

  // Get image snapshot from surface
  sk_sp<SkImage> image = surface->makeImageSnapshot();
  if (!image)
    return false;

  // Get pixel data from surface
  SkPixmap pixmap;
  if (!surface->peekPixels(&pixmap))
    return false;

  // Open file for writing
  SkFILEWStream fileStream(path);
  if (!fileStream.isValid())
    return false;

  // Encode as PNG
  return SkPngEncoder::Encode(&fileStream, pixmap, {});
}
