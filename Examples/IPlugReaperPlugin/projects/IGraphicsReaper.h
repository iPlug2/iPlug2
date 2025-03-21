
#include <CoreGraphics/CoreGraphics.h>

#include "IGraphicsSkia.h"
#include "IGraphicsCoreText.h"

#include "reaper_plugin_fx_embed.h"

#include "config.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

class IGraphicsReaper final : public IGraphicsSkia
{
public:
  
  IGraphicsReaper(IGEditorDelegate& dlg, int w, int h, int fps, float scale);
  virtual ~IGraphicsReaper();

  void SetBundleID(const char* bundleID) { mBundleID.Set(bundleID); }
  void SetAppGroupID(const char* appGroupID) { mAppGroupID.Set(appGroupID); }

  // FIX - currently returning as a nullptr
  
  void* OpenWindow(void* pWindow) override { mOpen = true; GetDelegate()->LayoutUI(this); return nullptr; }
  void CloseWindow() override { mOpen = false; }
  bool WindowIsOpen() override { return mOpen; }
  void PlatformResize(bool parentHasResized) override { UpdateTooltips(); }
  
  // FIX - where are these called?
  
  //void AttachPlatformView(const IRECT& r, void* pView) override;
  //void RemovePlatformView(void* pView) override;
  //void HidePlatformView(void* pView, bool hide) override;

  void HideMouseCursor(bool hide, bool lock) override;
  void MoveMouseCursor(float x, float y) override;
  ECursor SetMouseCursor(ECursor cursorType) override;
  
  void GetMouseLocation(float& x, float&y) const override;

  void DoCursorLock(float x, float y, float& prevX, float& prevY);
    
  EMsgBoxResult ShowMessageBox(const char* str, const char* title, EMsgBoxType type, IMsgBoxCompletionHandlerFunc completionHandler) override;

  const char* GetPlatformAPIStr() override { return "Reaper Embedded"; }

  // FIX - currently does nothing
  
  void ForceEndUserEdit() override {}
  void UpdateTooltips() override {};

  bool RevealPathInExplorerOrFinder(WDL_String& path, bool select) override;
  void PromptForFile(WDL_String& fileName, WDL_String& path, EFileAction action, const char* ext, IFileDialogCompletionHandlerFunc completionHandler) override;
  void PromptForDirectory(WDL_String& dir, IFileDialogCompletionHandlerFunc completionHandler) override;
  bool PromptForColor(IColor& color, const char* str, IColorPickerHandlerFunc func) override;
    
  bool OpenURL(const char* url, const char* msgWindowTitle, const char* confirmMsg, const char* errMsgOnFailure) override;

  // FIX - for now we never return as having a window
  
  void* GetWindow() override { return nullptr; }
  void* GetWindow() const { return nullptr; }
  
  const char* GetBundleID() const override { return mBundleID.Get(); }
  const char* GetAppGroupID() const override { return mAppGroupID.Get(); }
  static int GetUserOSVersion();

  bool GetTextFromClipboard(WDL_String& str) override;
  bool SetTextInClipboard(const char* str) override;
  bool SetFilePathInClipboard(const char* path) override;

  bool InitiateExternalFileDragDrop(const char* path, const IRECT& iconBounds) override API_AVAILABLE(macos(10.13));

  // FIX - WHY IS THIS OVERRIDEN IN MAC?
  //float MeasureText(const IText& text, const char* str, IRECT& bounds) const override;

  //EUIAppearance GetUIAppearance() const override;
  
  // Drawing - NO-OPs
  
  void OnViewInitialized(void* pContext) override {}
  void OnViewDestroyed() override {}
  void EndFrame() override {}
  
  // Drawing Resize
  
  void DrawResize() override;
  
  const char* GetDrawingAPIStr() override { return "SKIA | CPU"; }
    
  // The main draw loop
  
  int DrawEmbedded(REAPER_FXEMBED_IBitmap* bitmap, REAPER_FXEMBED_DrawInfo* pInfo);
  
  IMouseInfo GetMouseInfo(void* pMsg);

protected:
  
  IPopupMenu* CreatePlatformPopupMenu(IPopupMenu& menu, const IRECT bounds, bool& isAsync) override;
  void CreatePlatformTextEntry(int paramIdx, const IText& text, const IRECT& bounds, int length, const char* str) override;
  
  //void ActivateGLContext() override;
  //void DeactivateGLContext() override;

private:
    
  void PointToScreen(float& x, float& y) const;
  void ScreenToPoint(float& x, float& y) const;

  PlatformFontPtr LoadPlatformFont(const char* fontID, const char* fileNameOrResID) override;
  PlatformFontPtr LoadPlatformFont(const char* fontID, const char* fontName, ETextStyle style) override;
  PlatformFontPtr LoadPlatformFont(const char* fontID, void* pData, int dataSize) override;
  void CachePlatformFont(const char* fontID, const PlatformFontPtr& font) override;

  void RepositionCursor(CGPoint point);
  void StoreCursorPosition();
  
  bool mOpen = false;
  CGPoint mCursorLockPosition;
  WDL_String mBundleID, mAppGroupID;
  float mLastX = -1.f;
  float mLastY = -1.f;
};

// A delegate for IGraphics Reaper

class IGReaperEditorDelegate : public IGEditorDelegate
{
public:
  
  IGReaperEditorDelegate(int nParams = 0) : IGEditorDelegate(nParams)
  {
    mMakeGraphicsFunc = [&]
    {
      IGraphicsReaper* pGraphics = new IGraphicsReaper(*this, 100, 100, 60, 1.f);
      //pGraphics->SetBundleID(BUNDLE_DOMAIN "." BUNDLE_MFR "." API_EXT "." BUNDLE_NAME API_EXT2);
      pGraphics->SetAppGroupID("group." BUNDLE_DOMAIN "." BUNDLE_MFR "." BUNDLE_NAME);
      pGraphics->SetSharedResourcesSubPath(SHARED_RESOURCES_SUBPATH);
        
      return pGraphics;
    };
  }
  
  int EmbeddedUIProc(int message, void* pMsg1, void* pMsg2);
  
  // Set Layout
  
  void SetLayoutFunc(std::function<void(IGraphics* pGraphics)> func) { mLayoutFunc = func; }
  
  // The host should not be informed of param changes from this embedded UI
  
  void BeginInformHostOfParamChangeFromUI(int paramIdx) override {}
  void EndInformHostOfParamChangeFromUI(int paramIdx) override {}
  
  // Size Hints
  
  void SetPreferredAspect(int num, int denom);
  void SetMinimumAspect(int num, int denom);
  void SetMinSize(int width, int height);
  void SetMaxSize(int width, int height);

private:
  
  int mPreferredAspect = 0;
  int mMinimumAspect = 0;
  int mMinWidth = 0;
  int mMinHeight = 0;
  int mMaxWidth = 0;
  int mMaxHeight = 0;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE

