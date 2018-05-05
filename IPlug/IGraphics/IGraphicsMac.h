#pragma once

#ifndef NO_IGRAPHICS

#include "IGraphics_select.h"

/** IGraphics platform class for macOS
*   @ingroup PlatformClasses
*/
class IGraphicsMac final : public IGRAPHICS_DRAW_CLASS
{
public:
  IGraphicsMac(IDelegate& dlg, int w, int h, int fps);
  virtual ~IGraphicsMac();

  void SetBundleID(const char* bundleID) { mBundleID.Set(bundleID); }
  void CreateMetalLayer();

  bool IsSandboxed();

  void* OpenWindow(void* pWindow) override;
  void CloseWindow() override;
  bool WindowIsOpen() override;
  void Resize(int w, int h, float scale) override;

  void HideMouseCursor(bool hide, bool returnToStartPosition) override;
  void MoveMouseCursor(float x, float y) override;

  int ShowMessageBox(const char* str, const char* caption, int type) override;
  void ForceEndUserEdit() override;

  const char* GetPlatformAPIStr() override;

  void UpdateTooltips() override;

  void HostPath(WDL_String& path) override;
  void PluginPath(WDL_String& path) override;
  void DesktopPath(WDL_String& path) override;
  void UserHomePath(WDL_String& path) override;
  void AppSupportPath(WDL_String& path, bool isSystem) override;
  void SandboxSafeAppSupportPath(WDL_String& path) override;
  void VST3PresetsPath(WDL_String& path, const char* mfrName, const char* pluginName, bool isSystem) override;
  bool RevealPathInExplorerOrFinder(WDL_String& path, bool select) override;

  void PromptForFile(WDL_String& fileName, WDL_String& path, EFileAction action, const char* ext) override;
  void PromptForDirectory(WDL_String& dir) override;
  bool PromptForColor(IColor& color, const char* str) override;

  IPopupMenu* CreatePopupMenu(IPopupMenu& menu, const IRECT& bounds, IControl* pCaller) override;
  void CreateTextEntry(IControl& control, const IText& text, const IRECT& bounds, const char* str) override;

  bool OpenURL(const char* url, const char* msgWindowTitle, const char* confirmMsg, const char* errMsgOnFailure) override;

  void* GetWindow() override;

  const char* GetBundleID()  { return mBundleID.Get(); }
  static int GetUserOSVersion();

  bool GetTextFromClipboard(WDL_String& str) override;

  bool MeasureText(const IText& text, const char* str, IRECT& bounds) override;

  void* mLayer;

  void SetMousePosition(float x, float y);

protected:
  bool OSFindResource(const char* name, const char* type, WDL_String& result) override;

private:
  bool GetResourcePathFromBundle(const char* fileName, const char* searchExt, WDL_String& fullPath);
  bool GetResourcePathFromUsersMusicFolder(const char* fileName, const char* searchExt, WDL_String& fullPath);
  void* mView; // Can't forward-declare an IGraphicsView because it's an obj-C object.
  WDL_String mBundleID;
  friend int GetMouseOver(IGraphicsMac* pGraphics);
};

inline int AdjustFontSize(int size) //TODO: sort this out
{
  return int(0.9 * (double)size);
}

#endif // NO_IGRAPHICS
