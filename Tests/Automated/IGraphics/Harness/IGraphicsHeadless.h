/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @brief An IGraphics implementation with no window and no renderer, for the automated tests.
 *
 * IGraphicsHeadless implements the drawing and platform halves of IGraphics in memory. Drawing calls are recorded
 * rather than rasterised, text is measured with a fixed-pitch model, and platform services (clipboard, cursor,
 * dialogs, text entry, pop-up menus) are faked and recorded. Everything above that line - the control stack, hit
 * testing, mouse/keyboard dispatch, parameter linking, dirty tracking, layers, resizing - is the real IGraphics code.
 */

#include <string>
#include <vector>
#include <cstdio>
#include <cstring>

#include "IGraphics.h"

#ifdef OS_LINUX
  #include "IGraphicsLinux.h" // The test shim: see Harness/LinuxShim
  #define IGRAPHICS_HEADLESS_BASE IGraphicsLinux
#else
  #define IGRAPHICS_HEADLESS_BASE IGraphics
#endif

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** A bitmap that only knows its dimensions */
class HeadlessBitmap : public APIBitmap
{
public:
  HeadlessBitmap(int w, int h, float scale, float drawScale)
  : APIBitmap(0, w, h, scale, drawScale)
  {}
};

class IGraphicsHeadless : public IGRAPHICS_HEADLESS_BASE
{
public:
  /** A recorded drawing operation */
  struct DrawOp
  {
    enum class EType { Fill, Stroke, Text, Bitmap };
    EType mType;
    IColor mColor;     // first colour of the pattern (fills and strokes) or the text colour
    IRECT mBounds;     // bounds of the path points (untransformed), the text rectangle, or the bitmap destination
    float mThickness;  // strokes only
    std::string mText; // text only
  };

  /** The width of one character as a proportion of the font size in the fake text measurement */
  static constexpr float kCharWidth = 0.5f;

  IGraphicsHeadless(IGEditorDelegate& dlg, int w, int h, int fps = DEFAULT_FPS, float scale = 1.f)
  : IGRAPHICS_HEADLESS_BASE(dlg, w, h, fps, scale)
  {}

  ~IGraphicsHeadless()
  {
    // As the real platform classes do, since the base class destructor can no longer reach the bitmap virtuals
    RemoveAllControls();
  }

#pragma mark - Test helpers

  /** Runs one frame as a platform timer would: draws whatever is dirty.
   * @return true if anything was drawn */
  bool RenderFrame()
  {
    IRECTList rects;

    if (!IsDirty(rects))
      return false;

    SetAllControlsClean();
    BeginFrame();
    Draw(rects);
    EndFrame();
    mNumFrames++;
    mLastDirtyBounds = rects.Bounds();
    return true;
  }

  void ClearRecorded()
  {
    mDrawOps.clear();
    mNumFrames = 0;
  }

  int CountOps(DrawOp::EType type) const
  {
    int n = 0;
    for (auto& op : mDrawOps)
      n += (op.mType == type);
    return n;
  }

  bool DidDrawText(const char* str) const
  {
    for (auto& op : mDrawOps)
    {
      if (op.mType == DrawOp::EType::Text && op.mText == str)
        return true;
    }
    return false;
  }

  /** Convenience mouse gestures, in UI coordinates */
  void Click(float x, float y, const IMouseMod& mod = IMouseMod(true))
  {
    mMouseX = x; mMouseY = y;
    OnMouseDown({MakeMouseInfo(x, y, 0.f, 0.f, mod)});
    OnMouseUp({MakeMouseInfo(x, y, 0.f, 0.f, mod)});
  }

  void Drag(float x1, float y1, float x2, float y2, int nSteps = 4, const IMouseMod& mod = IMouseMod(true))
  {
    mMouseX = x1; mMouseY = y1;
    OnMouseDown({MakeMouseInfo(x1, y1, 0.f, 0.f, mod)});

    float prevX = x1, prevY = y1;

    for (int i = 1; i <= nSteps; i++)
    {
      const float x = x1 + (x2 - x1) * static_cast<float>(i) / static_cast<float>(nSteps);
      const float y = y1 + (y2 - y1) * static_cast<float>(i) / static_cast<float>(nSteps);
      mMouseX = x; mMouseY = y;
      OnMouseDrag({MakeMouseInfo(x, y, x - prevX, y - prevY, mod)});
      prevX = x; prevY = y;
    }

    OnMouseUp({MakeMouseInfo(x2, y2, 0.f, 0.f, mod)});
  }

  static IMouseInfo MakeMouseInfo(float x, float y, float dX, float dY, const IMouseMod& mod)
  {
    IMouseInfo info;
    info.x = x; info.y = y; info.dX = dX; info.dY = dY; info.ms = mod;
    return info;
  }

  std::vector<DrawOp> mDrawOps;
  IRECT mLastDirtyBounds; // the union of the dirty rectangles drawn by the last RenderFrame()
  int mNumFrames = 0;

  // Recorded platform requests
  std::string mClipboard;
  std::string mLastOpenedURL;
  std::string mLastMessageBoxText;
  EMsgBoxResult mMessageBoxResult = kOK;
  std::string mTextEntryStr;       // the initial string of the last platform text entry
  IRECT mTextEntryBounds;
  int mNumTextEntries = 0;
  int mNumPopupMenus = 0;
  IRECT mPopupMenuBounds;
  int mPopupMenuChoice = -1;       // the item CreatePlatformPopupMenu() "chooses", -1 to dismiss
  bool mCursorHidden = false;
  bool mCursorLocked = false;
  float mMouseX = 0.f;
  float mMouseY = 0.f;
  int mNumPlatformResizes = 0;
  int mNumDrawResizes = 0;
  IRECT mClipRect;
  IMatrix mTransform;

#pragma mark - Drawing

  void DrawBitmap(const IBitmap& bitmap, const IRECT& bounds, int srcX, int srcY, const IBlend* pBlend) override
  {
    mDrawOps.push_back({DrawOp::EType::Bitmap, IColor(), bounds, 0.f, ""});
  }

  IColor GetPoint(int x, int y) override { return COLOR_BLACK; }
  void* GetDrawContext() override { return nullptr; }
  const char* GetDrawingAPIStr() override { return "Headless"; }
  bool BitmapExtSupported(const char* ext) override { return strstr(ext, "png") != nullptr; }

  void GetLayerBitmapData(const ILayerPtr& layer, RawBitmapData& data) override
  {
    const APIBitmap* pBitmap = layer->GetAPIBitmap();
    data.Resize(pBitmap->GetWidth() * pBitmap->GetHeight() * 4);
    memset(data.Get(), 0, static_cast<size_t>(data.GetSize()));
  }

  void ApplyShadowMask(ILayerPtr& layer, RawBitmapData& mask, const IShadow& shadow) override {}

  void PathClear() override { mPathBounds = IRECT(); mPathHasPoints = false; }
  void PathClose() override {}
  void PathMoveTo(float x, float y) override { AddPathPoint(x, y); }
  void PathLineTo(float x, float y) override { AddPathPoint(x, y); }

  void PathArc(float cx, float cy, float r, float a1, float a2, EWinding winding) override
  {
    AddPathPoint(cx - r, cy - r);
    AddPathPoint(cx + r, cy + r);
  }

  void PathCubicBezierTo(float c1x, float c1y, float c2x, float c2y, float x2, float y2) override { AddPathPoint(x2, y2); }
  void PathQuadraticBezierTo(float cx, float cy, float x2, float y2) override { AddPathPoint(x2, y2); }

  void PathStroke(const IPattern& pattern, float thickness, const IStrokeOptions& options, const IBlend* pBlend) override
  {
    mDrawOps.push_back({DrawOp::EType::Stroke, pattern.GetStop(0).mColor, mPathBounds, thickness, ""});

    if (!options.mPreserve)
      PathClear();
  }

  void PathFill(const IPattern& pattern, const IFillOptions& options, const IBlend* pBlend) override
  {
    mDrawOps.push_back({DrawOp::EType::Fill, pattern.GetStop(0).mColor, mPathBounds, 0.f, ""});

    if (!options.mPreserve)
      PathClear();
  }

  void PathTransformSetMatrix(const IMatrix& matrix) override { mTransform = matrix; }
  void SetClipRegion(const IRECT& r) override { mClipRect = r; }

#pragma mark - Platform

  void GetMouseLocation(float& x, float& y) const override { x = mMouseX; y = mMouseY; }
  void HideMouseCursor(bool hide, bool lock) override { mCursorHidden = hide; mCursorLocked = hide && lock; }
  void MoveMouseCursor(float x, float y) override { mMouseX = x; mMouseY = y; }
  void ForceEndUserEdit() override {}

  void* OpenWindow(void* pParentWnd) override
  {
    mWindowOpen = true;
    OnViewInitialized(nullptr);
    SetScreenScale(mInitialScreenScale);
    GetDelegate()->LayoutUI(this);
    SetAllControlsDirty();
    GetDelegate()->OnUIOpen();
    return this;
  }

  void CloseWindow() override
  {
    if (mWindowOpen)
    {
      OnViewDestroyed();
      mWindowOpen = false;
    }
  }

  void* GetWindow() override { return mWindowOpen ? this : nullptr; }

  bool GetTextFromClipboard(WDL_String& str) override { str.Set(mClipboard.c_str()); return !mClipboard.empty(); }
  bool SetTextInClipboard(const char* str) override { mClipboard = str; return true; }
  void UpdateTooltips() override {}

  EMsgBoxResult ShowMessageBox(const char* str, const char* title, EMsgBoxType type, IMsgBoxCompletionHandlerFunc completionHandler) override
  {
    mLastMessageBoxText = str;

    if (completionHandler)
      completionHandler(mMessageBoxResult);

    return mMessageBoxResult;
  }

  void PromptForFile(WDL_String& fileName, WDL_String& path, EFileAction action, const char* ext, IFileDialogCompletionHandlerFunc completionHandler) override
  {
    if (completionHandler)
      completionHandler(fileName, path);
  }

  void PromptForDirectory(WDL_String& dir, IFileDialogCompletionHandlerFunc completionHandler) override
  {
    if (completionHandler)
      completionHandler(dir, dir);
  }

  bool PromptForColor(IColor& color, const char* str, IColorPickerHandlerFunc func) override { return false; }

  bool OpenURL(const char* url, const char* msgWindowTitle, const char* confirmMsg, const char* errMsgOnFailure) override
  {
    mLastOpenedURL = url;
    return true;
  }

  PlatformFontPtr LoadPlatformFont(const char* fontID, const char* fileNameOrResID) override { return PlatformFontPtr(new PlatformFont(false)); }
  PlatformFontPtr LoadPlatformFont(const char* fontID, void* pData, int dataSize) override { return PlatformFontPtr(new PlatformFont(false)); }
  PlatformFontPtr LoadPlatformFont(const char* fontID, const char* fontName, ETextStyle style) override { return PlatformFontPtr(new PlatformFont(true)); }
  void CachePlatformFont(const char* fontID, const PlatformFontPtr& font) override {}

  /** The screen scale applied when the window is "opened" */
  float mInitialScreenScale = 1.f;

protected:
  void CreatePlatformTextEntry(int paramIdx, const IText& text, const IRECT& bounds, int length, const char* str) override
  {
    mNumTextEntries++;
    mTextEntryStr = str;
    mTextEntryBounds = bounds;
  }

  IPopupMenu* CreatePlatformPopupMenu(IPopupMenu& menu, const IRECT bounds, bool& isAsync) override
  {
    mNumPopupMenus++;
    mPopupMenuBounds = bounds;
    isAsync = false;

    if (mPopupMenuChoice < 0 || mPopupMenuChoice >= menu.NItems())
      return nullptr;

    menu.SetChosenItemIdx(mPopupMenuChoice);

    if (auto func = menu.GetFunction())
      func(&menu);

    return &menu;
  }

  APIBitmap* LoadAPIBitmap(const char* fileNameOrResID, int scale, EResourceLocation location, const char* ext) override
  {
    FILE* pFile = fopen(fileNameOrResID, "rb");

    if (!pFile)
      return nullptr;

    uint8_t header[24] = {};
    const size_t nRead = fread(header, 1, sizeof(header), pFile);
    fclose(pFile);

    return LoadAPIBitmap(fileNameOrResID, header, static_cast<int>(nRead), scale);
  }

  /** Reads the dimensions from a PNG header, which is all a HeadlessBitmap needs */
  APIBitmap* LoadAPIBitmap(const char* name, const void* pData, int dataSize, int scale) override
  {
    static const uint8_t kPNGSignature[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
    const uint8_t* pBytes = static_cast<const uint8_t*>(pData);

    if (dataSize < 24 || memcmp(pBytes, kPNGSignature, 8) != 0)
      return nullptr;

    auto readBE = [pBytes](int pos) {
      return (pBytes[pos] << 24) | (pBytes[pos + 1] << 16) | (pBytes[pos + 2] << 8) | pBytes[pos + 3];
    };

    return new HeadlessBitmap(readBE(16), readBE(20), static_cast<float>(scale), 1.f);
  }

  APIBitmap* CreateAPIBitmap(int width, int height, float scale, double drawScale, bool cacheable) override
  {
    return new HeadlessBitmap(width, height, scale, static_cast<float>(drawScale));
  }

  bool LoadAPIFont(const char* fontID, const PlatformFontPtr& font) override { return true; }
  int AlphaChannel() const override { return 3; }
  bool FlippedBitmap() const override { return false; }

  /** Fixed-pitch text model: every character is kCharWidth * size wide and the line is size high */
  float DoMeasureText(const IText& text, const char* str, IRECT& bounds) const override
  {
    const float w = static_cast<float>(strlen(str)) * text.mSize * kCharWidth;
    const float h = text.mSize;
    float x = bounds.L;
    float y = bounds.T;

    switch (text.mAlign)
    {
      case EAlign::Near: break;
      case EAlign::Center: x = bounds.MW() - w / 2.f; break;
      case EAlign::Far: x = bounds.R - w; break;
    }

    switch (text.mVAlign)
    {
      case EVAlign::Top: break;
      case EVAlign::Middle: y = bounds.MH() - h / 2.f; break;
      case EVAlign::Bottom: y = bounds.B - h; break;
    }

    bounds = IRECT(x, y, x + w, y + h);
    return w;
  }

  void DoDrawText(const IText& text, const char* str, const IRECT& bounds, const IBlend* pBlend) override
  {
    mDrawOps.push_back({DrawOp::EType::Text, text.mFGColor, bounds, 0.f, str});
  }

private:
  void PlatformResize(bool parentHasResized) override { mNumPlatformResizes++; }
  void DrawResize() override { mNumDrawResizes++; }

  void AddPathPoint(float x, float y)
  {
    if (!mPathHasPoints)
    {
      mPathBounds = IRECT(x, y, x, y);
      mPathHasPoints = true;
    }
    else
    {
      mPathBounds = IRECT(std::min(mPathBounds.L, x), std::min(mPathBounds.T, y), std::max(mPathBounds.R, x), std::max(mPathBounds.B, y));
    }
  }

  IRECT mPathBounds;
  bool mPathHasPoints = false;
  bool mWindowOpen = false;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
