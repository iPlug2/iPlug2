/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#include "IGraphics_select.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

extern void GetScreenDimensions(int& width, int& height);

extern float GetScaleForScreen(int height);

/** IGraphics platform class for IOS
*   @ingroup PlatformClasses */
class IGraphicsIOS final : public IGRAPHICS_DRAW_CLASS
{
public:  
  IGraphicsIOS(IGEditorDelegate& dlg, int w, int h, int fps, float scale);
  virtual ~IGraphicsIOS();
  
  void SetBundleID(const char* bundleID) { mBundleID.Set(bundleID); }

  void* OpenWindow(void* pWindow) override;
  void CloseWindow() override;
  bool WindowIsOpen() override;
  void PlatformResize(bool parentHasResized) override;
  void AttachPlatformView(const IRECT& r, void* pView) override;
  void RemovePlatformView(void* pView) override;

  void GetMouseLocation(float& x, float&y) const override;

  EMsgBoxResult ShowMessageBox(const char* str, const char* caption, EMsgBoxType type, IMsgBoxCompletionHanderFunc completionHandler) override;
  void ForceEndUserEdit() override;

  const char* GetPlatformAPIStr() override;

  void UpdateTooltips() override {};

  void PromptForFile(WDL_String& fileName, WDL_String& path, EFileAction action, const char* ext) override;
  void PromptForDirectory(WDL_String& dir) override;
  bool PromptForColor(IColor& color, const char* str, IColorPickerHandlerFunc func) override;
  
  void HideMouseCursor(bool hide, bool lock) override {}; // NOOP
  void MoveMouseCursor(float x, float y) override {}; // NOOP
  
  bool OpenURL(const char* url, const char* msgWindowTitle, const char* confirmMsg, const char* errMsgOnFailure) override;
  
  void* GetWindow() override;
  
  const char* GetBundleID() override { return mBundleID.Get(); }
  static int GetUserOSVersion();
  
  bool GetTextFromClipboard(WDL_String& str) override;
  bool SetTextInClipboard(const char* str) override;

  void CreatePlatformImGui() override;

  void LaunchBluetoothMidiDialog(float x, float y);
  
  void AttachGestureRecognizer(EGestureType type) override;
  
  bool PlatformSupportsMultiTouch() const override { return true; }

protected:
  PlatformFontPtr LoadPlatformFont(const char* fontID, const char* fileNameOrResID) override;
  PlatformFontPtr LoadPlatformFont(const char* fontID, const char* fontName, ETextStyle style) override;
  void CachePlatformFont(const char* fontID, const PlatformFontPtr& font) override;
  
  IPopupMenu* CreatePlatformPopupMenu(IPopupMenu& menu, const IRECT& bounds, bool& isAsync) override;
  void CreatePlatformTextEntry(int paramIdx, const IText& text, const IRECT& bounds, int length, const char* str) override;

private:
  void* mView = nullptr;
  void* mImGuiView = nullptr;

  WDL_String mBundleID;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
