#pragma once

/**
 * @file
 * @copydoc IGraphics
 */

#ifndef NO_FREETYPE
#include "ft2build.h"
#include FT_FREETYPE_H
#endif

#ifdef AAX_API
#include "IPlugAAX_view_interface.h"
#endif

#include "IGraphicsConstants.h"
#include "IGraphicsStructs.h"
#include "IGraphicsUtilites.h"
#include "IPopupMenu.h"
#include "IControl.h"

#ifdef FillRect
#undef FillRect
#endif

#ifdef DrawText
#undef DrawText
#endif

class IPlugBaseGraphics;
class IControl;
class IParam;

/**
 * \defgroup DrawClasses IGraphics::DrawClasses
 * \defgroup PlatformClasses IGraphics::PlatformClasses
*/

/**  The lowest level base class of an IGraphics context */
class IGraphics
#ifdef AAX_API
: public IPlugAAXView_Interface
#endif
{

public:
#pragma mark - IGraphics drawing API implementation
  //These are NanoVG only, may be refactored
  virtual void BeginFrame() {};
  virtual void EndFrame() {};
  virtual void ViewInitialized(void* layer) {};
  //
  
  /** Called by platform IGraphics class when UI created and when moving to a new screen with different DPI, implementations in draw class must call the base implementation */
  virtual void SetDisplayScale(int scale) { mDisplayScale = (float) scale; OnDisplayScale(); };

  virtual void DrawSVG(ISVG& svg, const IRECT& dest, const IBlend* pBlend = 0) = 0;
  
  /**
   /todo
   
   @param base <#base description#>
   @param mask <#mask description#>
   @param top <#top description#>
   @param x <#x description#>
   @param y <#y description#>
   @param angle the angle to rotate the svg in degrees clockwise /see IGraphicsDrawing documentation
   @param pBlend <#pBlend description#>
   */
  virtual void DrawRotatedSVG(ISVG& svg, float destCtrX, float destCtrY, float width, float height, double angle, const IBlend* pBlend = 0) = 0;
    
  virtual void DrawBitmap(IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend = 0) = 0;
  virtual void DrawRotatedBitmap(IBitmap& bitmap, int destCtrX, int destCtrY, double angle, int yOffsetZeroDeg = 0, const IBlend* pBlend = 0) = 0;
  
  
  /**
   /todo

   @param base <#base description#>
   @param mask <#mask description#>
   @param top <#top description#>
   @param x <#x description#>
   @param y <#y description#>
   @param angle the angle to rotate the bitmap mask at in degrees clockwise /see IGraphicsDrawing documentation
   @param pBlend <#pBlend description#>
   */
  virtual void DrawRotatedMask(IBitmap& base, IBitmap& mask, IBitmap& top, int x, int y, double angle, const IBlend* pBlend = 0) = 0;
  virtual void DrawPoint(const IColor& color, float x, float y, const IBlend* pBlend = 0) = 0;
  virtual void ForcePixel(const IColor& color, int x, int y) = 0;
 
  virtual void DrawLine(const IColor& color, float x1, float y1, float x2, float y2, const IBlend* pBlend = 0) = 0;
  virtual void DrawTriangle(const IColor& color, float x1, float y1, float x2, float y2, float x3, float y3, const IBlend* pBlend = 0) = 0;
  virtual void DrawRect(const IColor& color, const IRECT& rect, const IBlend* pBlend = 0) = 0;
  virtual void DrawRoundRect(const IColor& color, const IRECT& rect, float cr = 5.f, const IBlend* pBlend = 0) = 0;
  
  
  
  /**
   /todo

   @param color <#color description#>
   @param cx <#cx description#>
   @param cy <#cy description#>
   @param r <#r description#>
   @param aMin the start angle  of the arc at in degrees clockwise where 0 is up /see IGraphicsDrawing documentation
   @param aMax the end angle  of the arc at in degrees clockwise where 0 is up /see IGraphicsDrawing documentation
   @param pBlend <#pBlend description#>
   */
  virtual void DrawArc(const IColor& color, float cx, float cy, float r, float aMin, float aMax, const IBlend* pBlend = 0) = 0;
  
  
  virtual void DrawCircle(const IColor& color, float cx, float cy, float r, const IBlend* pBlend = 0) = 0;
  virtual void DrawConvexPolygon(const IColor& color, float* x, float* y, int npoints, const IBlend* pBlend = 0) = 0;

  virtual void DrawDottedRect(const IColor& color, const IRECT& rect, const IBlend* pBlend = 0) = 0;

  virtual void FillTriangle(const IColor& color, float x1, float y1, float x2, float y2, float x3, float y3, const IBlend* pBlend = 0) = 0;
  virtual void FillRect(const IColor& color, const IRECT& rect, const IBlend* pBlend = 0) = 0;
  virtual void FillRoundRect(const IColor& color, const IRECT& rect, float cr = 5.f, const IBlend* pBlend = 0) = 0;
  virtual void FillCircle(const IColor& color, float cx, float cy, float r, const IBlend* pBlend = 0) = 0;
  virtual void FillArc(const IColor& color, float cx, float cy, float r, float aMin, float aMax, const IBlend* pBlend = 0) = 0;
  virtual void FillConvexPolygon(const IColor& color, float* x, float* y, int npoints, const IBlend* pBlend = 0) = 0;

  virtual bool DrawText(const IText& text, const char* str, IRECT& destRect, bool measure = false) = 0;
  virtual bool MeasureText(const IText& text, const char* str, IRECT& destRect) = 0;

  virtual IColor GetPoint(int x, int y)  = 0;
  virtual void* GetData() = 0;
  virtual const char* GetDrawingAPIStr() = 0;
  
  inline virtual void ClipRegion(const IRECT& r) {}; // overridden in some IGraphics drawing classes to clip drawing
  inline virtual void ResetClipRegion() {}; // overridden in some IGraphics drawing classes to reset clip

#pragma mark - IGraphics drawing API implementation (bitmap handling)
  virtual IBitmap LoadBitmap(const char* name, int nStates = 1, bool framesAreHorizontal = false);
  virtual IBitmap ScaleBitmap(const IBitmap& srcbitmap, const char* cacheName, int targetScale);
  //virtual IBitmap CropBitmap(const IBitmap& bitmap, const IRECT& rect, const char* name, int targetScale) = 0;
  virtual void RetainBitmap(IBitmap& bitmap, const char* cacheName);
  virtual void ReleaseBitmap(IBitmap& bitmap);
  IBitmap GetScaledBitmap(IBitmap& src);
  virtual void OnDisplayScale();
  
  /** Called by some drawing API classes to finally blit the draw bitmap onto the screen */
  virtual void RenderDrawBitmap() {}

#pragma mark - IGraphics base implementation - drawing helpers
  /** Draws a bitmap into the graphics context
   
   @param bitmap - the bitmap to draw
   @param rect - where to draw the bitmap
   @param bmpState - the frame of the bitmap to draw
   @param pBlend - blend operation
   */
  void DrawBitmap(IBitmap& bitmap, const IRECT& rect, int bmpState = 1, const IBlend* pBlend = 0);
  
  
  /** Draws monospace bitmapped text. Useful for identical looking text on multiple platforms.
   @param bitmap the bitmap containing glyphs to draw
   @param rect where to draw the bitmap
   @param text text properties (note - many of these are irrelevant for bitmapped text)
   @param pBlend blend operation
   @param str the string to draw
   @param vCenter centre the text vertically
   @param multiline should the text spill onto multiple lines
   @param charWidth how wide is a character in the bitmap
   @param charHeight how high is a character in the bitmap
   @param charOffset what is the offset between characters drawn
   */
  void DrawBitmapedText(IBitmap& bitmap, IRECT& rect, IText& text, IBlend* pBlend, const char* str, bool vCenter = true, bool multiline = false, int charWidth = 6, int charHeight = 12, int charOffset = 0);
  
  void DrawVerticalLine(const IColor& color, const IRECT& rect, float x, const IBlend* pBlend = 0);
  void DrawHorizontalLine(const IColor& color, const IRECT& rect, float y, const IBlend* pBlend = 0);
  void DrawVerticalLine(const IColor& color, float xi, float yLo, float yHi, const IBlend* pBlend = 0);
  void DrawHorizontalLine(const IColor& color, float yi, float xLo, float xHi, const IBlend* pBlend = 0);
  
  /**
   Helper function to draw a radial line, useful for pointers on dials

   @param color the colour of the line
   @param cx centre point x coordinate
   @param cy centre point y coordinate
   @param angle the angle to draw at in degrees clockwise where 0 is up /see IGraphicsDrawing documentation
   @param rMin minima of the radial line (distance from cx,cy)
   @param rMax maxima of the radial line (distance from cx,cy)
   @param pBlend blend operation
   */
  void DrawRadialLine(const IColor& color, float cx, float cy, float angle, float rMin, float rMax, const IBlend* pBlend = 0);
  void DrawGrid(const IColor& color, const IRECT& rect, int gridSizeH, int gridSizeV, const IBlend* pBlend);
  
#pragma mark - IGraphics platform implementation
  virtual void HideMouseCursor() {};
  virtual void ShowMouseCursor() {};
  virtual void ForceEndUserEdit() = 0;
  virtual void Resize(int w, int h, float scale);
  virtual void* OpenWindow(void* pParentWnd) = 0;
  virtual void CloseWindow() = 0;
  virtual void* GetWindow() = 0;
  virtual bool GetTextFromClipboard(WDL_String& str) = 0;
  virtual void UpdateTooltips() = 0;
  virtual int ShowMessageBox(const char* str, const char* caption, int type) = 0;
  virtual IPopupMenu* CreateIPopupMenu(IPopupMenu& menu, IRECT& textRect) = 0;
  virtual void CreateTextEntry(IControl* pControl, const IText& text, const IRECT& textRect, const char* str = "", IParam* pParam = 0) = 0;
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

  /** Used on Windows to set the HINSTANCE handle, which allows graphics APIs to load resources from the binary*/
  virtual void SetPlatformInstance(void* instance) {}
  virtual void* GetPlatformInstance() { return nullptr; }

  /** Used with IGraphicsLice (possibly others) in order to set the core graphics draw context on macOS and the GDI HDC draw context handle on Windows
   * On macOS, this is called by the platform IGraphics class IGraphicsMac, on Windows it is called by the drawing class e.g. IGraphicsLice.*/
  virtual void SetPlatformContext(void* pContext) { mPlatformContext = pContext; }
  void* GetPlatformContext() { return mPlatformContext; }
  
  /** Try to ascertain the full path of a resource.*/
  virtual bool OSFindResource(const char* name, const char* type, WDL_String& result) = 0;
  
#pragma mark - IGraphics base implementation
  IGraphics(IPlugBaseGraphics& plug, int w, int h, int fps = 0);
  virtual ~IGraphics();

  bool IsDirty(IRECT& rect);
  virtual void Draw(const IRECT& rect);
  
  virtual ISVG LoadSVG(const char* name); // TODO: correct place?

  void PromptUserInput(IControl* pControl, IParam* pParam, IRECT& textRect);
  void SetFromStringAfterPrompt(IControl* pControl, IParam* pParam, const char* txt);
  IPopupMenu* CreateIPopupMenu(IPopupMenu& menu, float x, float y) { IRECT tempRect = IRECT(x,y,x,y); return CreateIPopupMenu(menu, tempRect); }
  
  void SetStrictDrawing(bool strict);

  int Width() const { return mWidth; }
  int Height() const { return mHeight; }
  int WindowWidth() const { return int((float) mWidth * mScale); }
  int WindowHeight() const { return int((float) mHeight * mScale); }
  int FPS() const { return mFPS; }
  float GetScale() const { return mScale; }
  float GetDisplayScale() const { return mDisplayScale; }
  IPlugBaseGraphics& GetPlug() { return mPlug; }

  void AttachBackground(const char* name);
  void AttachPanelBackground(const IColor& color);
  void AttachKeyCatcher(IControl& control);
  int AttachControl(IControl* control);

  IControl* GetControl(int idx) { return mControls.Get(idx); }
  int NControls() const { return mControls.GetSize(); }
  void HideControl(int paramIdx, bool hide);
  void GrayOutControl(int paramIdx, bool gray);
  void ClampControl(int paramIdx, double lo, double hi, bool normalized);
  void SetAllControlsDirty();
  void SetControlFromPlug(int controlIdx, double normalizedValue);

  void SetParameterFromPlug(int paramIdx, double value, bool normalized);
  void SetParameterFromGUI(int paramIdx, double normalizedValue);

  void OnMouseDown(float x, float y, const IMouseMod& mod);
  void OnMouseUp(float x, float y, const IMouseMod& mod);
  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod);
  bool OnMouseDblClick(float x, float y, const IMouseMod& mod);
  void OnMouseWheel(float x, float y, const IMouseMod& mod, float d);
  bool OnKeyDown(float x, float y, int key);
  bool OnMouseOver(float x, float y, const IMouseMod& mod);
  void OnMouseOut();
  void OnDrop(const char* str, float x, float y);
  void OnGUIIdle();

  // AAX only
  int GetParamIdxForPTAutomation(float x, float y);
  int GetLastClickedParamForPTAutomation();
  void SetPTParameterHighlight(int paramIdx, bool isHighlighted, int color);
  
  // VST3 primarily
  void PopupHostContextMenuForParam(int controlIdx, int paramIdx, float x, float y);
  
  void HandleMouseOver(bool canHandle) { mHandleMouseOver = canHandle; }
  void ReleaseMouseCapture();

  void EnableTooltips(bool enable);
  
  void AssignParamNameToolTips();

  //debugging tools
  inline void ShowControlBounds(bool enable) { mShowControlBounds = enable; }
  inline void ShowAreaDrawn(bool enable) { mShowAreaDrawn = enable; }
  void EnableLiveEdit(bool enable, const char* file = 0, int gridsize = 10);

  IRECT GetDrawRect() const { return mDrawRECT; }
 
  bool CanHandleMouseOver() const { return mHandleMouseOver; }
  inline int GetMouseOver() const { return mMouseOver; }
  inline bool TooltipsEnabled() const { return mEnableTooltips; }

  virtual void LoadFont(const char* name);
  
protected:
    
  virtual APIBitmap* LoadAPIBitmap(const WDL_String& resourcePath, int scale) = 0;
  //virtual void* CreateAPIBitmap(int w, int h) = 0;
  virtual APIBitmap* ScaleAPIBitmap(const APIBitmap* pBitmap, int scale) = 0;

  bool FindImage(const char* name, const char* type, WDL_String& result, int targetScale, int& sourceScale);

  WDL_PtrList<IControl> mControls;
  IRECT mDrawRECT;
  void* mPlatformContext = nullptr;
  IPlugBaseGraphics& mPlug;
  
  bool mCursorHidden = false;
private:
  friend class IGraphicsLiveEdit;
  
  int GetMouseControlIdx(float x, float y, bool mo = false);

  int mWidth, mHeight, mFPS;
  float mDisplayScale = 1.f; // the scaling of the display that the ui is currently on e.g. 2 for retina
  float mScale = 1.f; // scale deviation from plug-in width and height i.e .stretching the gui by dragging
  int mIdleTicks = 0;
  int mMouseCapture = -1;
  int mMouseOver = -1;
  int mLastClickedParam = kNoParameter;
  bool mHandleMouseOver = false;
  bool mStrict = true;
  bool mEnableTooltips;
  bool mShowControlBounds = false;
  bool mShowAreaDrawn = false;
  IControl* mKeyCatcher = nullptr;
  
#if !defined(NDEBUG) && defined(APP_API)
  IControl* mLiveEdit = nullptr;
#endif

#if !defined(NO_FREETYPE) 
protected:
  FT_Library mFTLibrary = nullptr;
  FT_Face mFTFace = nullptr;
#endif
};
