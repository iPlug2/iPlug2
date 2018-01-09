#pragma once

#ifdef AAX_API
#include "IPlugAAX_view_interface.h"
#endif

#include "IGraphicsConstants.h"
#include "IGraphicsStructs.h"
#include "IGraphicsUtilites.h"
#include "IPopupMenu.h"
#include "IControl.h"

class IPlugBaseGraphics;
class IControl;
class IParam;

/**
 * \defgroup DrawClasses IGraphics::DrawClasses
 * \defgroup PlatformClasses IGraphics::PlatformClasses
*/

class IGraphics
#ifdef AAX_API
: public IPlugAAXView_Interface
#endif
{
public:
  virtual void PrepDraw() = 0;
  
  //These are NanoVG only, may be refactored
  virtual void BeginFrame() {};
  virtual void EndFrame() {};
  virtual void ViewInitialized(void* layer) {};
  //

  bool IsDirty(IRECT& rect);
  virtual void Draw(const IRECT& rect);
  virtual void DrawScreen(const IRECT& rect) = 0;
  
  void DrawBitmap(IBitmap& bitmap, const IRECT& rect, int bmpState = 1, const IBlend* pBlend = 0);
  void DrawBitmapedText(IBitmap& bitmap, IRECT& rect, IText& text, IBlend* pBlend, const char* str, bool vCenter = true, bool multiline = false, int charWidth = 6, int charHeight = 12, int charOffset = 0);
  
#pragma mark - IGraphics API  impl drawing (pure virtual)
  virtual void DrawBitmap(IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend = 0) = 0;
  virtual void DrawRotatedBitmap(IBitmap& bitmap, int destCtrX, int destCtrY, double angle, int yOffsetZeroDeg = 0, const IBlend* pBlend = 0) = 0;
  virtual void DrawRotatedMask(IBitmap& base, IBitmap& mask, IBitmap& top, int x, int y, double angle, const IBlend* pBlend = 0) = 0;
  virtual void DrawPoint(const IColor& color, float x, float y, const IBlend* pBlend = 0, bool aa = false) = 0;

  virtual void ForcePixel(const IColor& color, int x, int y) = 0;
 
  virtual void DrawLine(const IColor& color, float x1, float y1, float x2, float y2, const IBlend* pBlend = 0, bool aa = false) = 0;
  virtual void DrawTriangle(const IColor& color, int x1, int y1, int x2, int y2, int x3, int y3, const IBlend* pBlend = 0) = 0;
  virtual void DrawArc(const IColor& color, float cx, float cy, float r, float minAngle, float maxAngle, const IBlend* pBlend = 0, bool aa = false) = 0;
  virtual void DrawCircle(const IColor& color, float cx, float cy, float r, const IBlend* pBlend = 0, bool aa = false) = 0;
  virtual void DrawRoundRect(const IColor& color, const IRECT& rect, const IBlend* pBlend = 0, int cr = 5, bool aa = false) = 0;
  
  virtual void FillRoundRect(const IColor& color, const IRECT& rect, const IBlend* pBlend = 0, int cr = 5, bool aa = false) = 0;
  virtual void FillIRect(const IColor& color, const IRECT& rect, const IBlend* pBlend = 0) = 0;
  virtual void FillCircle(const IColor& color, int cx, int cy, float r, const IBlend* pBlend = 0, bool aa = false) = 0;
  virtual void FillIConvexPolygon(const IColor& color, int* x, int* y, int npoints, const IBlend* pBlend = 0) = 0;
  virtual void FillTriangle(const IColor& color, int x1, int y1, int x2, int y2, int x3, int y3, const IBlend* pBlend = 0) = 0;

  virtual bool DrawIText(const IText& text, const char* str, IRECT& destRect, bool measure = false) = 0;
  virtual bool MeasureIText(const IText& text, const char* str, IRECT& destRect) = 0;

  virtual IColor GetPoint(int x, int y)  = 0;
  virtual void* GetData() = 0;
  
  virtual const char* GetDrawingAPIStr() = 0;

#pragma mark - IGraphics impl drawing helpers
  virtual void DrawRect(const IColor& color, const IRECT& rect);
  void DrawVerticalLine(const IColor& color, const IRECT& rect, float x);
  void DrawHorizontalLine(const IColor& color, const IRECT& rect, float y);
  void DrawVerticalLine(const IColor& color, int xi, int yLo, int yHi);
  void DrawHorizontalLine(const IColor& color, int yi, int xLo, int xHi);
  void DrawRadialLine(const IColor& color, float cx, float cy, float angle, float rMin, float rMax, bool aa = false);

#pragma mark - IGraphics impl
  void PromptUserInput(IControl* pControl, IParam* pParam, IRECT& textRect);
  void SetFromStringAfterPrompt(IControl* pControl, IParam* pParam, const char* txt);
  void SetStrictDrawing(bool strict);

  virtual void ForceEndUserEdit() = 0;
  virtual void Resize(int w, int h);

#pragma mark - IGraphicsPlatform impl
  virtual int ShowMessageBox(const char* str, const char* caption, int type) = 0;
  IPopupMenu* CreateIPopupMenu(IPopupMenu& menu, int x, int y) { IRECT tempRect = IRECT(x,y,x,y); return CreateIPopupMenu(menu, tempRect); }
  virtual IPopupMenu* CreateIPopupMenu(IPopupMenu& menu, IRECT& textRect) = 0;
  virtual void CreateTextEntry(IControl* pControl, const IText& text, const IRECT& textRect, const char* str = "", IParam* pParam = 0) = 0;
  
  //TODO: should pPath be a pointer or reference?
  virtual void PromptForFile(WDL_String& filename, WDL_String& path, EFileAction action = kFileOpen, const char* extensions = 0) = 0;
  virtual bool PromptForColor(IColor& color, const char* str = "") = 0;
  virtual bool OpenURL(const char* url, const char* msgWindowTitle = 0, const char* confirmMsg = 0, const char* errMsgOnFailure = 0) = 0;
  virtual const char* GetGUIAPI() { return ""; }
  virtual bool WindowIsOpen() { return GetWindow(); }
  virtual void HostPath(WDL_String& path) = 0;
  virtual void PluginPath(WDL_String& path) = 0;
  virtual void DesktopPath(WDL_String& path) = 0;
  virtual void AppSupportPath(WDL_String& path, bool isSystem = false) = 0;
  virtual void SandboxSafeAppSupportPath(WDL_String& path) = 0;
  virtual void VST3PresetsPath(WDL_String& path, bool isSystem = true) { path.Set(""); }

  virtual bool RevealPathInExplorerOrFinder(WDL_String& path, bool select = false) = 0;
  
  virtual void* OpenWindow(void* pParentWnd) = 0;
  virtual void CloseWindow() = 0;
  virtual void* GetWindow() = 0;
  virtual bool GetTextFromClipboard(WDL_String& str) = 0;

#pragma mark -
  IGraphics(IPlugBaseGraphics& plug, int w, int h, int fps = 0);
  virtual ~IGraphics();

  int Width() const { return mWidth; }
  int Height() const { return mHeight; }
  int FPS() const { return mFPS; }
  double GetScale() const { return mScale; }
  void SetScale(double scale) { mScale = scale; }
  double GetDisplayScale() const { return mDisplayScale; }
  void SetDisplayScale(double scale) { mDisplayScale = scale; }
  IPlugBase& GetPlug() { return mPlug; }

  virtual IBitmap LoadIBitmap(const char* name, int nStates = 1, bool framesAreHoriztonal = false, double scale = 1.) = 0;
  virtual IBitmap ScaleIBitmap(const IBitmap& srcbitmap, const char* cacheName, double targetScale) = 0;
  virtual IBitmap CropIBitmap(const IBitmap& bitmap, const IRECT& rect, const char* name, double targetScale) = 0;
  IBitmap GetScaledBitmap(IBitmap& src);
  virtual void ReScale();
  
  void AttachBackground(const char* name, double scale = 1.);
  void AttachPanelBackground(const IColor& color);
  void AttachKeyCatcher(IControl& control);
  int AttachControl(IControl* control);

  IControl* GetControl(int idx) { return mControls.Get(idx); }
  int GetNControls() const { return mControls.GetSize(); }
  void HideControl(int paramIdx, bool hide);
  void GrayOutControl(int paramIdx, bool gray);

  void ClampControl(int paramIdx, double lo, double hi, bool normalized);
  void SetParameterFromPlug(int paramIdx, double value, bool normalized);
  void SetControlFromPlug(int controlIdx, double normalizedValue);
  
  inline virtual void ClipRegion(const IRECT& r) {}; // overridden in some IGraphics classes to clip drawing
  inline virtual void ResetClipRegion() {}; // overridden in some IGraphics classes to clip drawing
  void SetAllControlsDirty();
  
  void SetParameterFromGUI(int paramIdx, double normalizedValue);

  void OnMouseDown(int x, int y, const IMouseMod& mod);
  void OnMouseUp(int x, int y, const IMouseMod& mod);
  void OnMouseDrag(int x, int y, const IMouseMod& mod);
  bool OnMouseDblClick(int x, int y, const IMouseMod& mod);
  void OnMouseWheel(int x, int y, const IMouseMod& mod, int d);
  bool OnKeyDown(int x, int y, int key);

  virtual void HideMouseCursor() {};
  virtual void ShowMouseCursor() {};

  // AAX only
  int GetParamIdxForPTAutomation(int x, int y);
  int GetLastClickedParamForPTAutomation();
  void SetPTParameterHighlight(int paramIdx, bool isHighlighted, int color);
  
  // VST3 primarily
  void PopupHostContextMenuForParam(int controlIdx, int paramIdx, int x, int y);
  
  void HandleMouseOver(bool canHandle) { mHandleMouseOver = canHandle; }
  bool OnMouseOver(int x, int y, const IMouseMod& mod);
  void OnMouseOut();
  void ReleaseMouseCapture();

  void EnableTooltips(bool enable)
  {
    mEnableTooltips = enable;
    if (enable)
      mHandleMouseOver = enable;
  }
  
  void AssignParamNameToolTips();
  virtual void UpdateTooltips() = 0;

  inline void ShowControlBounds(bool enable) { mShowControlBounds = enable; }
  inline void ShowAreaDrawn(bool enable) { mShowAreaDrawn = enable; }

  void OnGUIIdle();
  void OnDrop(const char* str, int x, int y);
  
  virtual void RetainIBitmap(IBitmap& bitmap, const char* cacheName) = 0;
  virtual void ReleaseIBitmap(IBitmap& bitmap) = 0;

  virtual void OSLoadBitmap(const char* name, WDL_String& result) = 0;
//  virtual void OSLoadFont(const char* name, const int size, WDL_String& fullPath) = 0;
//  virtual void* OSLoadSVG(const char* name, const int size) = 0;

  IRECT GetDrawRect() const { return mDrawRECT; }
  void* GetPlatformContext() { return mPlatformContext; }
  virtual void SetPlatformContext(void* pContext) { mPlatformContext = pContext; }
 
protected:
  bool CanHandleMouseOver() const { return mHandleMouseOver; }
  inline int GetMouseOver() const { return mMouseOver; }
  inline int GetMouseX() const { return mMouseX; }
  inline int GetMouseY() const { return mMouseY; }
  inline bool TooltipsEnabled() const { return mEnableTooltips; }
  
  // this is called by some drawing API classes to blit the bitmap onto the screen (IGraphicsLice)
  virtual void RenderAPIBitmap(void* pContext) {}
  
  WDL_PtrList<IControl> mControls;
  IRECT mDrawRECT;
  void* mPlatformContext = nullptr;
  IPlugBaseGraphics& mPlug;
  
  bool mCursorHidden = false;
  int mHiddenMousePointX = -1;
  int mHiddenMousePointY = -1;
  double mScale = 1.; // scale deviation from plug-in width and height i.e .stretching the gui by dragging
  double mDisplayScale = 1.; // the scaling of the display that the ui is currently on e.g. 2. for retina

private:
  int GetMouseControlIdx(int x, int y, bool mo = false);

  int mWidth, mHeight, mFPS;
  int mIdleTicks = 0;
  int mMouseCapture = -1;
  int mMouseOver = -1;
  int mMouseX = 0;
  int mMouseY = 0;
  int mLastClickedParam = kNoParameter;
  bool mHandleMouseOver = false;
  bool mStrict = true;
  bool mEnableTooltips;
  bool mShowControlBounds = false;
  bool mShowAreaDrawn = false;
  IControl* mKeyCatcher = nullptr;
};
