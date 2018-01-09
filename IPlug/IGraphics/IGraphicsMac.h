#pragma once

//TODO: would be nice not to put this here
#ifndef NO_IGRAPHICS
  #ifdef IGRAPHICS_AGG
    #define IGRAPHICS_DRAW_CLASS IGraphicsAGG
    #include "IGraphicsAGG.h"
  #elif defined IGRAPHICS_CAIRO
    #define IGRAPHICS_DRAW_CLASS IGraphicsCairo
    #include "IGraphicsCairo.h"
  #elif defined IGRAPHICS_NANOVG
    #define IGRAPHICS_DRAW_CLASS IGraphicsNanoVG
    #include "IGraphicsNanoVG.h"
  #else
    #define IGRAPHICS_DRAW_CLASS IGraphicsLice
    #include "IGraphicsLice.h"
  #endif
#endif

//objective-c has a flat namespace, we need to customise the class name for all of our objective-c classes
//so that binaries using different versions don't conflict
#ifndef OBJC_PREFIX
  #define OBJC_PREFIX vWDLOL
#endif

#if defined(VST_API)
  #define API _vst
#elif defined(AU_API)
  #define API _au
#elif defined(AAX_API)
  #define API _aax
#elif defined(VST3_API)
  #define API _vst3
#elif defined(SA_API)
  #define API _sa
#endif

#define CONCAT3(a,b,c) a##b##c
#define CONCAT2(a,b,c) CONCAT3(a,b,c)
#define CONCAT(cname) CONCAT2(cname,OBJC_PREFIX,API)

#define IGRAPHICS_VIEW CONCAT(IGraphicsView_)
#define IGRAPHICS_MENU CONCAT(IGraphicsMenu_)
#define IGRAPHICS_MENU_RCVR CONCAT(IGraphicsMenuRcvr_)
#define IGRAPHICS_FORMATTER CONCAT(IGraphicsFormatter_)

/** @brief IGraphics platform class for macOS  
*   @ingroup PlatformClasses
*/
class IGraphicsMac : public IGRAPHICS_DRAW_CLASS
{
public:
  IGraphicsMac(IPlugBaseGraphics& plug, int w, int h, int fps);
  virtual ~IGraphicsMac();

  void SetBundleID(const char* bundleID) { mBundleID.Set(bundleID); }
  void CreateMetalLayer();
  
  void DrawScreen(const IRECT& rect) override;
  
  void* OpenWindow(void* pWindow) override;
  void CloseWindow() override;
  bool WindowIsOpen() override;
  void Resize(int w, int h) override;

  void HideMouseCursor() override;
  void ShowMouseCursor() override;

  int ShowMessageBox(const char* str, const char* caption, int type) override;
  void ForceEndUserEdit() override;

  const char* GetGUIAPI() override;
  
  void UpdateTooltips() override;

  void HostPath(WDL_String& path) override;
  void PluginPath(WDL_String& path) override;
  void DesktopPath(WDL_String& path) override;
  void AppSupportPath(WDL_String& path, bool isSystem) override;
  void SandboxSafeAppSupportPath(WDL_String& path) override;
  void VST3PresetsPath(WDL_String& path, bool isSystem) override;
  bool RevealPathInExplorerOrFinder(WDL_String& path, bool select) override;

  void PromptForFile(WDL_String& fileName, WDL_String& path, EFileAction action, const char* ext) override;
  bool PromptForColor(IColor& color, const char* str) override;

  IPopupMenu* CreateIPopupMenu(IPopupMenu& menu, IRECT& rect) override;
  void CreateTextEntry(IControl* pControl, const IText& text, const IRECT& textRect, const char* str, IParam* pParam) override;

  bool OpenURL(const char* url, const char* msgWindowTitle, const char* confirmMsg, const char* errMsgOnFailure) override;

  void* GetWindow() override;

  const char* GetBundleID()  { return mBundleID.Get(); }
  static int GetUserOSVersion();
  
  bool GetTextFromClipboard(WDL_String& str) override;

  bool MeasureIText(const IText& text, const char* str, IRECT& destRect) override;
  
  void* mLayer;

protected:
  void OSLoadBitmap(const char* name, WDL_String& result) override;
//  void OSLoadFont(const char* name, const int size) override;
//  bool LoadSVGFile(const WDL_String & file, WDL_String & fileOut);
  
private:
  void* mView; // Can't forward-declare an IGraphicsView because it's an obj-C object.
  
  WDL_String mBundleID;
  
  friend int GetMouseOver(IGraphicsMac* pGraphics);
};

inline int AdjustFontSize(int size) //TODO: sort this out
{
  return int(0.9 * (double)size);
}
