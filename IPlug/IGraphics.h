#pragma once

#include "IGraphicsConstants.h"
#include "IGraphicsStructs.h"
#include "IGraphicsUtilites.h"
#include "IPopupMenu.h"
#include "IControl.h"

#ifdef AAX_API
#include "IPlugAAX_view_interface.h"
#endif

class IPlugBaseGraphics;
class IControl;
class IParam;

class IGraphics
#ifdef AAX_API
: public IPlugAAXView_Interface
#endif
{
public:
  virtual void PrepDraw() = 0;

  bool IsDirty(IRECT& rect);
  virtual bool Draw(const IRECT& rect);
  virtual bool DrawScreen(const IRECT& rect) = 0;

  bool DrawBitmap(IBitmap& pBitmap, const IRECT& rect, int bmpState = 1, const IChannelBlend* pBlend = nullptr);
  
  // these are implemented in the drawing api IGraphics class
  virtual bool DrawBitmap(IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IChannelBlend* pBlend = nullptr) = 0;
  virtual bool DrawRotatedBitmap(IBitmap& pBitmap, int destCtrX, int destCtrY, double angle, int yOffsetZeroDeg = 0, const IChannelBlend* pBlend = nullptr) = 0;
  virtual bool DrawRotatedMask(IBitmap& pBase, IBitmap& pMask, IBitmap& pTop, int x, int y, double angle, const IChannelBlend* pBlend = nullptr) = 0;
  virtual bool DrawPoint(const IColor& color, float x, float y, const IChannelBlend* pBlend = nullptr, bool aa = false) = 0;

  virtual bool ForcePixel(const IColor& color, int x, int y) = 0;
  virtual bool DrawLine(const IColor& color, float x1, float y1, float x2, float y2, const IChannelBlend* pBlend = nullptr, bool aa = false) = 0;
  virtual bool DrawArc(const IColor& color, float cx, float cy, float r, float minAngle, float maxAngle, const IChannelBlend* pBlend = nullptr, bool aa = false) = 0;
  virtual bool DrawCircle(const IColor& color, float cx, float cy, float r, const IChannelBlend* pBlend = nullptr, bool aa = false) = 0;
  virtual bool RoundRect(const IColor& color, const IRECT& rect, const IChannelBlend* pBlend = nullptr, int cornerradius = 5, bool aa = false) = 0;
  virtual bool FillRoundRect(const IColor& color, const IRECT& rect, const IChannelBlend* pBlend = nullptr, int cornerradius = 5, bool aa = false) = 0;
  virtual bool FillIRect(const IColor& color, const IRECT& rect, const IChannelBlend* pBlend = nullptr) = 0;
  virtual bool FillCircle(const IColor& color, int cx, int cy, float r, const IChannelBlend* pBlend = nullptr, bool aa = false) = 0;
  virtual bool FillIConvexPolygon(const IColor& color, int* x, int* y, int npoints, const IChannelBlend* pBlend = nullptr) = 0;
  virtual bool FillTriangle(const IColor& color, int x1, int y1, int x2, int y2, int x3, int y3, const IChannelBlend* pBlend = nullptr) = 0;
  virtual bool DrawIText(const IText& text, const char* pStr, IRECT& destRect, bool measure = false) = 0;
  virtual bool MeasureIText(const IText& text, const char* pStr, IRECT& destRect) = 0;
  
  //these are helper functions implemented in the base IGraphics class
  bool DrawRect(const IColor& color, const IRECT& rect);
  bool DrawVerticalLine(const IColor& color, const IRECT& rect, float x);
  bool DrawHorizontalLine(const IColor& color, const IRECT& rect, float y);
  bool DrawVerticalLine(const IColor& color, int xi, int yLo, int yHi);
  bool DrawHorizontalLine(const IColor& color, int yi, int xLo, int xHi);
  bool DrawRadialLine(const IColor& color, float cx, float cy, float angle, float rMin, float rMax, bool aa = false);
  
  virtual IColor GetPoint(int x, int y)  = 0;
  virtual void* GetData() = 0;

  void PromptUserInput(IControl* pControl, IParam* pParam, IRECT& textRect);

  virtual void ForceEndUserEdit() = 0;
  virtual void Resize(int w, int h);
  virtual bool WindowIsOpen() { return GetWindow(); }
  virtual const char* GetGUIAPI() { return ""; }

  virtual int ShowMessageBox(const char* pStr, const char* pCaption, int type) = 0;
  IPopupMenu* CreateIPopupMenu(IPopupMenu& pMenu, int x, int y) { IRECT tempRect = IRECT(x,y,x,y); return CreateIPopupMenu(pMenu, tempRect); }
  virtual IPopupMenu* CreateIPopupMenu(IPopupMenu& pMenu, IRECT& textRect) = 0;
  virtual void CreateTextEntry(IControl* pControl, const IText& text, const IRECT& textRect, const char* pStr = "", IParam* pParam = 0) = 0;

  void SetFromStringAfterPrompt(IControl* pControl, IParam* pParam, const char* txt);
  virtual void HostPath(WDL_String& pPath) = 0;
  virtual void PluginPath(WDL_String& pPath) = 0;
  virtual void DesktopPath(WDL_String& pPath) = 0;

  virtual void AppSupportPath(WDL_String& pPath, bool isSystem = false) = 0;
  virtual void SandboxSafeAppSupportPath(WDL_String& pPath) = 0;

  virtual void PromptForFile(WDL_String& pFilename, EFileAction action = kFileOpen, WDL_String* pDir = 0, const char* extensions = 0) = 0;  // extensions = "txt wav" for example.
  virtual bool PromptForColor(IColor& pColor, const char* pStr = "") = 0;

  virtual bool OpenURL(const char* url, const char* msgWindowTitle = 0, const char* confirmMsg = 0, const char* errMsgOnFailure = 0) = 0;

  void SetStrictDrawing(bool strict);

  virtual void* OpenWindow(void* pParentWnd) = 0;
  virtual void* OpenWindow(void* pParentWnd, void* pParentControl) { return 0; }  // For OSX Carbon hosts ... ugh.

  virtual void CloseWindow() = 0;
  virtual void* GetWindow() = 0;

  virtual bool GetTextFromClipboard(WDL_String& pStr) = 0;

#pragma mark -

  IGraphics(IPlugBaseGraphics* pPlug, int w, int h, int fps = 0);
  virtual ~IGraphics();

  int Width() const { return mWidth; }
  int Height() const { return mHeight; }
  int FPS() const { return mFPS; }
//  double GetScale() const { return mScale; }
//  void SetScale(double scale) { mScale = scale; }
  double GetDisplayScale() const { return mDisplayScale; }
  void SetDisplayScale(double scale) { mDisplayScale = scale; mScale = scale; /* todo: mScale should be different*/ }

  IPlugBase* GetPlug() { return mPlug; }

  virtual IBitmap LoadIBitmap(const char* name, int nStates = 1, bool framesAreHoriztonal = false, double scale = 1.) = 0;
  virtual IBitmap ScaleIBitmap(const IBitmap& srcbitmap, const char* cacheName, double scale) = 0;
//  virtual IBitmap CropIBitmap(const IBitmap& srcbitmap, const IRECT& rect, const char* cacheName = 0) = 0;
  virtual void ReScaleBitmaps();
  
  void AttachBackground(const char* name);
  void AttachPanelBackground(const IColor& pColor);
  void AttachKeyCatcher(IControl& pControl);
  int AttachControl(IControl* pControl);

  IControl* GetControl(int idx) { return mControls.Get(idx); }
  int GetNControls() const { return mControls.GetSize(); }
  void HideControl(int paramIdx, bool hide);
  void GrayOutControl(int paramIdx, bool gray);

  void ClampControl(int paramIdx, double lo, double hi, bool normalized);
  void SetParameterFromPlug(int paramIdx, double value, bool normalized);
  void SetControlFromPlug(int controlIdx, double normalizedValue);
  
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

  int GetParamIdxForPTAutomation(int x, int y);
  int GetLastClickedParamForPTAutomation();

//  void DisplayControlValue(IControl* pControl);

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

  void OnGUIIdle();

  virtual void RetainBitmap(IBitmap& bitmap, const char* cacheName) = 0;
  virtual void ReleaseBitmap(IBitmap& bitmap) = 0;
  virtual IBitmap CreateBitmap(const char* cacheName, int w, int h) = 0;

  virtual void OSLoadBitmap(const char* name, WDL_String& fullPath) = 0;
//  virtual void OSLoadFont(const char* name, const int size, WDL_String& fullPath) = 0;
//  virtual void* OSLoadSVG(const char* name, const int size) = 0;

  IRECT GetDrawRect() const { return mDrawRECT; }
  
protected:
  WDL_PtrList<IControl> mControls;
  IPlugBaseGraphics* mPlug;
  IRECT mDrawRECT;
  bool mCursorHidden;
  int mHiddenMousePointX, mHiddenMousePointY;
  double mScale; // scale deviation from plug-in width and height i.e .stretching the gui by dragging
  double mDisplayScale; // the scaling of the display that the ui is currently on e.g. 2. for retina
  
  bool CanHandleMouseOver() const { return mHandleMouseOver; }
  inline int GetMouseOver() const { return mMouseOver; }
  inline int GetMouseX() const { return mMouseX; }
  inline int GetMouseY() const { return mMouseY; }
  inline bool TooltipsEnabled() const { return mEnableTooltips; }
  
  virtual void RenderAPIBitmap(void* pContext) = 0;

private:
  int mWidth, mHeight, mFPS, mIdleTicks;
  int GetMouseControlIdx(int x, int y, bool mo = false);
  int mMouseCapture, mMouseOver, mMouseX, mMouseY, mLastClickedParam;
  bool mHandleMouseOver, mStrict, mEnableTooltips, mShowControlBounds;
  IControl* mKeyCatcher;
};
