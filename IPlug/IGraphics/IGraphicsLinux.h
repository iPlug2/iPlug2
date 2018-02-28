#pragma once

#ifndef NO_IGRAPHICS

#ifdef IGRAPHICS_AGG
  #include "IGraphicsAGG.h"
  typedef IGraphicsAGG IGRAPHICS_DRAW_CLASS;
#elif defined IGRAPHICS_CAIRO
  #include "IGraphicsCairo.h"
  typedef IGraphicsCairo IGRAPHICS_DRAW_CLASS;
#elif defined IGRAPHICS_NANOVG
  #include "IGraphicsNanoVG.h"
  typedef IGraphicsNanoVG IGRAPHICS_DRAW_CLASS;
#else
  #include "IGraphicsLice.h"
  typedef IGraphicsLice IGRAPHICS_DRAW_CLASS;
#endif

/** IGraphics platform class for linux  
*   @ingroup PlatformClasses
*/
class IGraphicsLinux
{
public:
  IGraphicsLinux(IGraphicsDelegate& dlg, int w, int h, int fps)  final;
  virtual ~IGraphicsLinux);

  void* OpenWindow(void* pWindow) override;
  void CloseWindow() override;
  bool WindowIsOpen() override;
  void Resize(int w, int h, float scale) override;

  void HideMouseCursor() override;
  void ShowMouseCursor() override;

  int ShowMessageBox(const char* str, const char* caption, int type) override;
  void ForceEndUserEdit() override;

  const char* GetUIAPI() override;
  
  void UpdateTooltips() override;

  void HostPath(WDL_String& path) override;
  void PluginPath(WDL_String& path) override;
  void DesktopPath(WDL_String& path) override;
  void UserHomePath(WDL_String& path) override;
  void AppSupportPath(WDL_String& path, bool isSystem) override;
  void SandboxSafeAppSupportPath(WDL_String& path) override;
  void VST3PresetsPath(WDL_String& path, const char* mfrName, const char* pluginName, bool isSystem) override;
  bool RevealPathInExplorerOrFinder(WDL_String& path, bool select) override;

  void PromptForFile(WDL_String& fileName, WDL_String& path, EFileAction action,  const char* ext) override;
  bool PromptForColor(IColor& color, const char* str) override;

  IPopupMenu* CreateIPopupMenu(IPopupMenu& menu, IRECT& rect) override;
  void CreateTextEntry(IControl* pControl, const IText& text, const IRECT& textRect, const char* str) override;

  bool OpenURL(const char* url, const char* msgWindowTitle, const char* confirmMsg, const char* errMsgOnFailure) override;

  static int GetUserOSVersion();
  bool GetTextFromClipboard(WDL_String& str) override;

protected:
  bool OSFindResource(const char* name, const char* type, WDL_String& result) override;
}

#endif // NO_IGRAPHICS
