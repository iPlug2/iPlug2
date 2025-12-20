/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file IGraphicsHeadless.h
 * @brief Headless IGraphics platform for CPU-only rendering without a window.
 * Used for CLI screenshot generation and automated UI testing.
 */

#include "IGraphics_select.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** IGraphics platform class for headless (no window) rendering
 *  Uses Skia CPU backend to render UI to memory for screenshot capture
 *  @ingroup PlatformClasses */
class IGraphicsHeadless final : public IGRAPHICS_DRAW_CLASS
{
public:
  IGraphicsHeadless(IGEditorDelegate& dlg, int w, int h, int fps, float scale);
  virtual ~IGraphicsHeadless();

  // Platform interface - minimal stubs for headless operation
  void* OpenWindow(void* pWindow) override;
  void CloseWindow() override;
  bool WindowIsOpen() override { return mWindowOpen; }
  void PlatformResize(bool parentHasResized) override {}

  void HideMouseCursor(bool hide, bool lock) override {}
  void MoveMouseCursor(float x, float y) override {}
  ECursor SetMouseCursor(ECursor cursorType) override { return ECursor::ARROW; }
  void GetMouseLocation(float& x, float& y) const override { x = 0; y = 0; }

  EMsgBoxResult ShowMessageBox(const char* str, const char* title, EMsgBoxType type,
                               IMsgBoxCompletionHandlerFunc completionHandler) override
  {
    return EMsgBoxResult::kOK;
  }

  void ForceEndUserEdit() override {}

  const char* GetPlatformAPIStr() override { return "Headless"; }

  void UpdateTooltips() override {}

  bool RevealPathInExplorerOrFinder(WDL_String& path, bool select) override { return false; }

  void PromptForFile(WDL_String& fileName, WDL_String& path, EFileAction action,
                     const char* ext, IFileDialogCompletionHandlerFunc completionHandler) override {}

  void PromptForDirectory(WDL_String& dir, IFileDialogCompletionHandlerFunc completionHandler) override {}

  bool PromptForColor(IColor& color, const char* str, IColorPickerHandlerFunc func) override { return false; }

  bool OpenURL(const char* url, const char* msgWindowTitle,
               const char* confirmMsg, const char* errMsgOnFailure) override { return false; }

  void* GetWindow() override { return mWindowOpen ? (void*)1 : nullptr; }

  bool GetTextFromClipboard(WDL_String& str) override { return false; }
  bool SetTextInClipboard(const char* str) override { return false; }

  float MeasureText(const IText& text, const char* str, IRECT& bounds) const override;

  /** Initialize the graphics for headless rendering without opening a window
   *  This sets up the Skia CPU surface and prepares for rendering
   *  @param scale Screen scale factor (1.0 = standard, 2.0 = retina/HiDPI) */
  void InitHeadless(float scale = 1.0f);

  /** Render the current UI state to the internal surface
   *  Call this before SaveScreenshot to ensure the UI is drawn */
  void RenderUI();

  /** Set the screen scale and reinitialize the surface
   *  @param scale Screen scale factor (1.0 = standard, 2.0 = retina/HiDPI) */
  void SetScale(float scale);

  /** Save the current rendered surface to a PNG file
   * @param path Output file path (should end in .png)
   * @return true on success */
  bool SaveScreenshot(const char* path);

protected:
  IPopupMenu* CreatePlatformPopupMenu(IPopupMenu& menu, const IRECT bounds, bool& isAsync) override
  {
    isAsync = false;
    return nullptr;
  }

  void CreatePlatformTextEntry(int paramIdx, const IText& text, const IRECT& bounds,
                               int length, const char* str) override {}

private:
  PlatformFontPtr LoadPlatformFont(const char* fontID, const char* fileNameOrResID) override;
  PlatformFontPtr LoadPlatformFont(const char* fontID, const char* fontName, ETextStyle style) override;
  PlatformFontPtr LoadPlatformFont(const char* fontID, void* pData, int dataSize) override;
  void CachePlatformFont(const char* fontID, const PlatformFontPtr& font) override {}

  bool mWindowOpen = false;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
