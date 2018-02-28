#pragma once

/**
 * @file
 * @copydoc IGraphics
 */

#ifndef NO_IGRAPHICS
#if defined(IGRAPHICS_AGG) + defined(IGRAPHICS_CAIRO) + defined(IGRAPHICS_NANOVG) + defined(IGRAPHICS_LICE) != 1
#error Either NO_IGRAPHICS or one and only one choice of graphics library must be defined!
#endif
#endif

#ifdef IGRAPHICS_FREETYPE
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

#ifdef OS_MAC
#ifdef FillRect
#undef FillRect
#endif

#ifdef DrawText
#undef DrawText
#endif
#endif

class IDelegate;
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

  /** Called by platform IGraphics class when UI created and when moving to a new screen with different DPI, implementations in draw class must call the base implementation
   * @param scale An integer specifying the scale of the display, where 2 = mac retina /todo better explanation of scale */
  virtual void SetDisplayScale(int scale) { mDisplayScale = (float) scale; OnDisplayScale(); };

  /** Draw an SVG image to the graphics context
   * @param svg The image to draw
   * @param rect The rectangular area to draw the image in
   * @param pBlend Optional blend method, see IBlend documentation */
  virtual void DrawSVG(ISVG& svg, const IRECT& rect, const IBlend* pBlend = 0) = 0;

  /** Draw an SVG with rotation 
   * @param svg The svg image to draw to the graphics context
   * @param destCentreX The X coordinate of the centre point at which to rotate the image around. /todo check this
   * @param destCentreY The Y coordinate of the centre point at which to rotate the image around. /todo check this
   * @param width /todo
   * @param height /todo
   * @param angle angle the angle to rotate the bitmap mask at in degrees clockwise /see IGraphicsDrawing documentation
   * @param pBlend Optional blend method, see IBlend documentation */
  virtual void DrawRotatedSVG(ISVG& svg, float destCentreX, float destCentreY, float width, float height, double angle, const IBlend* pBlend = 0) = 0;

  /**
   /todo <#Description#>

   @param bitmap The bitmap image to draw to the graphics context
   @param rect The rectangular area to draw the image in
   @param srcX <#srcX description#>
   @param srcY <#srcY description#>
   @param pBlend Optional blend method, see IBlend documentation
  */
  virtual void DrawBitmap(IBitmap& bitmap, const IRECT& rect, int srcX, int srcY, const IBlend* pBlend = 0) = 0;

  /**
   /todo <#Description#>

   @param bitmap The bitmap image to draw to the graphics context
   @param destCentreX The X coordinate of the centre point at which to rotate the image around. /todo check this
   @param destCentreY The Y coordinate of the centre point at which to rotate the image around. /todo check this
   @param angle The angle of rotation in degrees
   @param yOffsetZeroDeg /todo
   @param pBlend Optional blend method, see IBlend documentation
  */
  virtual void DrawRotatedBitmap(IBitmap& bitmap, int destCentreX, int destCentreY, double angle, int yOffsetZeroDeg = 0, const IBlend* pBlend = 0) = 0;

  /**
   /todo <#Description#>

   @param x <#x description#>
   @param y <#y description#>
   @param angle the angle to rotate the bitmap mask at in degrees clockwise /see IGraphicsDrawing documentation
   @param pBlend Optional blend method, see IBlend documentation
  */
  virtual void DrawRotatedMask(IBitmap& base, IBitmap& mask, IBitmap& top, int x, int y, double angle, const IBlend* pBlend = 0) = 0;

  /**
   /todo <#Description#>

   @param color The colour to draw/fill the shape with>
   @param x <#x description#>
   @param y <#y description#>
   @param pBlend Optional blend method, see IBlend documentation
  */
  virtual void DrawPoint(const IColor& color, float x, float y, const IBlend* pBlend = 0) = 0;

  /**
   /todo <#Description#>

   @param color The colour to draw/fill the shape with>
   @param x <#x description#>
   @param y <#y description#>
  */
  virtual void ForcePixel(const IColor& color, int x, int y) = 0;

  /**
   /todo <#Description#>

   @param color The colour to draw/fill the shape with>
   @param x1 <#x1 description#>
   @param y1 <#y1 description#>
   @param x2 <#x2 description#>
   @param y2 <#y2 description#>
   @param pBlend Optional blend method, see IBlend documentation
  */
  virtual void DrawLine(const IColor& color, float x1, float y1, float x2, float y2, const IBlend* pBlend = 0) = 0;

  /**
   <#Description#>

   @param color <#color description#>
   @param x1 <#x1 description#>
   @param y1 <#y1 description#>
   @param x2 <#x2 description#>
   @param y2 <#y2 description#>
   @param x3 <#x3 description#>
   @param y3 <#y3 description#>
   @param pBlend Optional blend method, see IBlend documentation
   */
  virtual void DrawTriangle(const IColor& color, float x1, float y1, float x2, float y2, float x3, float y3, const IBlend* pBlend = 0) = 0;

  /**
   /todo <#Description#>

   @param color The colour to draw/fill the shape with>
   @param rect The rectangular area to draw/fill the shape in
   @param pBlend Optional blend method, see IBlend documentation
  */
  virtual void DrawRect(const IColor& color, const IRECT& rect, const IBlend* pBlend = 0) = 0;

  /**
   /todo <#Description#>

   @param color The colour to draw/fill the shape with>
   @param rect The rectangular area to draw/fill the shape in
   @param .f <#.f description#>
   @param pBlend Optional blend method, see IBlend documentation
  */
  virtual void DrawRoundRect(const IColor& color, const IRECT& rect, float cr = 5.f, const IBlend* pBlend = 0) = 0;

  /**
   /todo

   @param color The colour to draw/fill the shape with>
   @param cx <#cx description#>
   @param cy <#cy description#>
   @param r <#r description#>
   @param aMin the start angle  of the arc at in degrees clockwise where 0 is up /see IGraphicsDrawing documentation
   @param aMax the end angle  of the arc at in degrees clockwise where 0 is up /see IGraphicsDrawing documentation
   @param pBlend Optional blend method, see IBlend documentation
  */
  virtual void DrawArc(const IColor& color, float cx, float cy, float r, float aMin, float aMax, const IBlend* pBlend = 0) = 0;



  /**
   /todo <#Description#>

   @param color The colour to draw/fill the shape with>
   @param cx <#cx description#>
   @param cy <#cy description#>
   @param r <#r description#>
   @param pBlend Optional blend method, see IBlend documentation
  */
  virtual void DrawCircle(const IColor& color, float cx, float cy, float r, const IBlend* pBlend = 0) = 0;

  /**
   /todo <#Description#>

   @param color The colour to draw/fill the shape with>
   @param x <#x description#>
   @param y <#y description#>
   @param npoints <#npoints description#>
   @param pBlend Optional blend method, see IBlend documentation
  */
  virtual void DrawConvexPolygon(const IColor& color, float* x, float* y, int npoints, const IBlend* pBlend = 0) = 0;

  /**
   /todo <#Description#>

   @param color The colour to draw/fill the shape with>
   @param rect The rectangular area to draw/fill the shape in
   @param pBlend Optional blend method, see IBlend documentation
  */
  virtual void DrawDottedRect(const IColor& color, const IRECT& rect, const IBlend* pBlend = 0) = 0;

  /**
   /todo <#Description#>

   @param color The colour to draw/fill the shape with>
   @param x1 <#x1 description#>
   @param y1 <#y1 description#>
   @param x2 <#x2 description#>
   @param y2 <#y2 description#>
   @param x3 <#x3 description#>
   @param y3 <#y3 description#>
   @param pBlend Optional blend method, see IBlend documentation
  */
  virtual void FillTriangle(const IColor& color, float x1, float y1, float x2, float y2, float x3, float y3, const IBlend* pBlend = 0) = 0;

  /**
   /todo <#Description#>

   @param color The colour to draw/fill the shape with>
   @param rect The rectangular area to draw/fill the shape in
   @param pBlend Optional blend method, see IBlend documentation
  */
  virtual void FillRect(const IColor& color, const IRECT& rect, const IBlend* pBlend = 0) = 0;

  /**
   /todo <#Description#>

   @param color The colour to draw/fill the shape with>
   @param rect The rectangular area to draw/fill the shape in
   @param .f <#.f description#>
   @param pBlend Optional blend method, see IBlend documentation
  */
  virtual void FillRoundRect(const IColor& color, const IRECT& rect, float cr = 5.f, const IBlend* pBlend = 0) = 0;

  /**
   /todo <#Description#>

   @param color The colour to draw/fill the shape with>
   @param cx <#cx description#>
   @param cy <#cy description#>
   @param r <#r description#>
   @param pBlend Optional blend method, see IBlend documentation
  */
  virtual void FillCircle(const IColor& color, float cx, float cy, float r, const IBlend* pBlend = 0) = 0;

  /**
   /todo <#Description#>

   @param color The colour to draw/fill the shape with>
   @param cx <#cx description#>
   @param cy <#cy description#>
   @param r <#r description#>
   @param aMin <#aMin description#>
   @param aMax <#aMax description#>
   @param pBlend Optional blend method, see IBlend documentation
  */
  virtual void FillArc(const IColor& color, float cx, float cy, float r, float aMin, float aMax, const IBlend* pBlend = 0) = 0;

  /**
   /todo <#Description#>

   @param color The colour to draw/fill the shape with>
   @param x <#x description#>
   @param y <#y description#>
   @param npoints <#npoints description#>
   @param pBlend Optional blend method, see IBlend documentation
  */
  virtual void FillConvexPolygon(const IColor& color, float* x, float* y, int npoints, const IBlend* pBlend = 0) = 0;

  /**
   /todo <#Description#>

   @param text <#text description#>
   @param str <#str description#>
   @param destRect <#destRect description#>
   @param measure <#measure description#>
   @return <#return value description#>
  */
  virtual bool DrawText(const IText& text, const char* str, IRECT& destRect, bool measure = false) = 0;

  /**
   /todo <#Description#>

   @param text <#text description#>
   @param str <#str description#>
   @param destRect <#destRect description#>
   @return <#return value description#>
  */
  virtual bool MeasureText(const IText& text, const char* str, IRECT& destRect) = 0;

  /**
   /todo <#Description#>

   @param x <#x description#>
   @param y <#y description#>
   @return <#return value description#>
  */
  virtual IColor GetPoint(int x, int y)  = 0;

  /**
   /todo <#Description#>
  */
  virtual void* GetData() = 0;

  /**
   /todo <#Description#>

   @return <#return value description#>
  */
  virtual const char* GetDrawingAPIStr() = 0;


  /**
   /todo <#Description#>

   @param r <#r description#>
   @return <#return value description#>
  */
  inline virtual void ClipRegion(const IRECT& r) {}; // overridden in some IGraphics drawing classes to clip drawing

  /**
   <#Description#>

   @return <#return value description#>
   */
  inline virtual void ResetClipRegion() {}; // overridden in some IGraphics drawing classes to reset clip

#pragma mark - IGraphics drawing API implementation (bitmap handling)
  virtual IBitmap LoadBitmap(const char* name, int nStates = 1, bool framesAreHorizontal = false);
  virtual IBitmap ScaleBitmap(const IBitmap& srcbitmap, const char* cacheName, int targetScale);
  //virtual IBitmap CropBitmap(const IBitmap& bitmap, const IRECT& rect, const char* name, int targetScale) = 0;
  virtual void RetainBitmap(const IBitmap& bitmap, const char* cacheName);
  virtual void ReleaseBitmap(const IBitmap& bitmap);
  IBitmap GetScaledBitmap(IBitmap& src);

  /**
   <#Description#>
   */
  virtual void OnDisplayScale();

  /** Called by some drawing API classes to finally blit the draw bitmap onto the screen */


  /**
   <#Description#>
   */
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

  /**
   /todo <#Description#>

   @param color The colour to draw/fill the shape with>
   @param rect The rectangular area to draw/fill the shape in
   @param x <#x description#>
   @param pBlend Optional blend method, see IBlend documentation
  */
  void DrawVerticalLine(const IColor& color, const IRECT& rect, float x, const IBlend* pBlend = 0);

  /**
   /todo <#Description#>

   @param color The colour to draw/fill the shape with>
   @param rect The rectangular area to draw/fill the shape in
   @param y <#y description#>
   @param pBlend Optional blend method, see IBlend documentation
  */
  void DrawHorizontalLine(const IColor& color, const IRECT& rect, float y, const IBlend* pBlend = 0);

  /**
   /todo <#Description#>

   @param color The colour to draw/fill the shape with>
   @param xi <#xi description#>
   @param yLo <#yLo description#>
   @param yHi <#yHi description#>
   @param pBlend Optional blend method, see IBlend documentation
  */
  void DrawVerticalLine(const IColor& color, float xi, float yLo, float yHi, const IBlend* pBlend = 0);

  /**
   /todo <#Description#>

   @param color The colour to draw/fill the shape with>
   @param yi <#yi description#>
   @param xLo <#xLo description#>
   @param xHi <#xHi description#>
   @param pBlend Optional blend method, see IBlend documentation
  */
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
  
  /**
   /todo <#Description#>
   
   @param color The colour to draw/fill the shape with>
   @param rect The rectangular area to draw/fill the shape in
   @param gridSizeH <#gridSizeH description#>
   @param gridSizeV <#gridSizeV description#>
   @param pBlend Optional blend method, see IBlend documentation
   */
  void DrawGrid(const IColor& color, const IRECT& rect, int gridSizeH, int gridSizeV, const IBlend* pBlend = 0);
  
#pragma mark - IGraphics drawing API Path support
  
  virtual bool HasPathSupport() const { return false; }
  
  virtual void PathStart() {}
  virtual void PathClose() {}
  
  void PathLine(float x1, float y1, float x2, float y2)
  {
    PathMoveTo(x1, y1);
    PathLineTo(x2, y2);
  }
    
  void PathRadialLine(float cx, float cy, float angle, float rMin, float rMax);
  
  virtual void PathTriangle(float x1, float y1, float x2, float y2, float x3, float y3) {}
  virtual void PathRect(const IRECT& rect) {}
  virtual void PathRoundRect(const IRECT& rect, float cr = 5.f) {}
  virtual void PathArc(float cx, float cy, float r, float aMin, float aMax) {}
  virtual void PathCircle(float cx, float cy, float r) {}
  virtual void PathConvexPolygon(float* x, float* y, int npoints) {}
  
  virtual void PathMoveTo(float x, float y) {}
  virtual void PathLineTo(float x, float y) {}
  virtual void PathCurveTo(float x1, float y1, float x2, float y2, float x3, float y3) {}
  
  virtual void PathStroke(const IPattern& pattern, float thickness, const IStrokeOptions& options = IStrokeOptions(), const IBlend* pBlend = 0) {}
  virtual void PathFill(const IPattern& pattern, const IFillOptions& options = IFillOptions(), const IBlend* pBlend = 0) {}

#pragma mark - IGraphics platform implementation
  /**
   /todo <#Description#>
  */
  virtual void HideMouseCursor() {};

  /**
   /todo <#Description#>
  */
  virtual void ShowMouseCursor() {};

  /**
   /todo <#Description#>
  */
  virtual void MoveMouseCursor(float x, float y) = 0;

  /**
  /todo <#Description#>
  */
  void SetTabletInput(bool tablet) { mTabletInput = tablet; }
    
  /**
  /todo <#Description#>
  */
  virtual void ForceEndUserEdit() = 0;

  /**
   /todo <#Description#>

   @param w <#w description#>
   @param h <#h description#>
   @param scale <#scale description#>
  */
  virtual void Resize(int w, int h, float scale);

  /**
   /todo <#Description#>

   @param pParentWnd <#pParentWnd description#>
  */
  virtual void* OpenWindow(void* pParentWnd) = 0;

  /**
   /todo <#Description#>
  */
  virtual void CloseWindow() = 0;

  /**
   /todo <#Description#>
  */
  virtual void* GetWindow() = 0;

  /**
   /todo <#Description#>

   @param str <#str description#>
   @return <#return value description#>
  */
  virtual bool GetTextFromClipboard(WDL_String& str) = 0;

  /**
   /todo <#Description#>
  */
  virtual void UpdateTooltips() = 0;

  /**
   /todo <#Description#>

   @param str <#str description#>
   @param caption <#caption description#>
   @param type <#type description#>
   @return <#return value description#>
  */
  virtual int ShowMessageBox(const char* str, const char* caption, int type) = 0;

  /**
   /todo <#Description#>

   @param menu <#menu description#>
   @param textRect <#textRect description#>
   @return <#return value description#>
  */
  virtual IPopupMenu* CreateIPopupMenu(IPopupMenu& menu, IRECT& textRect) = 0;

  /**
   /todo <#Description#>

   @param pControl <#pControl description#>
   @param text <#text description#>
   @param textRect <#textRect description#>
   @param "" <#"" description#>
   @param pParam <#pParam description#>
  */
  virtual void CreateTextEntry(IControl& control, const IText& text, const IRECT& textRect, const char* str = "") = 0;

  /**
   /todo <#Description#>

   @param filename <#filename description#>
   @param path <#path description#>
   @param action <#action description#>
   @param extensions <#extensions description#>
  */
  virtual void PromptForFile(WDL_String& filename, WDL_String& path, EFileAction action = kFileOpen, const char* extensions = 0) = 0;

  /**
   /todo <#Description#>

   @param color The colour to draw/fill the shape with>
   @param "" <#"" description#>
   @return <#return value description#>
  */
  virtual bool PromptForColor(IColor& color, const char* str = "") = 0;

  /**
   /todo <#Description#>

   @param url <#url description#>
   @param msgWindowTitle <#msgWindowTitle description#>
   @param confirmMsg <#confirmMsg description#>
   @param errMsgOnFailure <#errMsgOnFailure description#>
   @return <#return value description#>
  */
  virtual bool OpenURL(const char* url, const char* msgWindowTitle = 0, const char* confirmMsg = 0, const char* errMsgOnFailure = 0) = 0;

  /**
   /todo <#Description#>

   @return <#return value description#>
  */
  virtual const char* GetUIAPI() { return ""; }


  /**
   /todo <#Description#>

   @return <#return value description#>
  */
  virtual bool WindowIsOpen() { return GetWindow(); }


  /**
   /todo <#Description#>

   @param path <#path description#>
  */
  virtual void HostPath(WDL_String& path) = 0;

  /**
   /todo <#Description#>

   @param path <#path description#>
  */
  virtual void PluginPath(WDL_String& path) = 0;

  /**
   /todo <#Description#>

   @param path <#path description#>
  */
  virtual void DesktopPath(WDL_String& path) = 0;

  /**
   /todo <#Description#>

   @param path <#path description#>
   */
  virtual void UserHomePath(WDL_String& path) = 0;

  /**
   /todo <#Description#>

   @param path <#path description#>
   @param isSystem <#isSystem description#>
  */
  virtual void AppSupportPath(WDL_String& path, bool isSystem = false) = 0;

  /**
   /todo <#Description#>

   @param path <#path description#>
  */
  virtual void SandboxSafeAppSupportPath(WDL_String& path) = 0;

  /**
   /todo <#Description#>

   @param path <#path description#>
   @param isSystem <#isSystem description#>
  */
  virtual void VST3PresetsPath(WDL_String& path, const char* mfrName, const char* pluginName, bool isSystem = true) { path.Set(""); }


  /**
   /todo <#Description#>

   @param path <#path description#>
   @param select <#select description#>
   @return <#return value description#>
  */
  virtual bool RevealPathInExplorerOrFinder(WDL_String& path, bool select = false) = 0;

  /*
   Used on Windows to set the HINSTANCE handle, which allows graphics APIs to load resources from the binary

   @param instance <#instance description#>
  */
  virtual void SetPlatformInstance(void* pInstance) {}


  /**
   /todo <#Description#>
  */
  virtual void* GetPlatformInstance() { return nullptr; }

  /**
   Used with IGraphicsLice (possibly others) in order to set the core graphics draw context on macOS and the GDI HDC draw context handle on Windows
   On macOS, this is called by the platform IGraphics class IGraphicsMac, on Windows it is called by the drawing class e.g. IGraphicsLice.

   @param pContext <#pContext description#>
  */
  virtual void SetPlatformContext(void* pContext) { mPlatformContext = pContext; }

  /**
   /todo <#Description#>

   @return <#return value description#>
  */
  void* GetPlatformContext() { return mPlatformContext; }

  /**
   Try to ascertain the full path of a resource.

   @param name <#name description#>
   @param type <#type description#>
   @param result <#result description#>
   @return <#return value description#>
  */
  virtual bool OSFindResource(const char* name, const char* type, WDL_String& result) = 0;

#pragma mark - IGraphics base implementation
  IGraphics(IDelegate& dlg, int w, int h, int fps = 0);
  virtual ~IGraphics();

  /**
   /todo <#Description#>

   @param rect The rectangular area to draw/fill the shape in
   @return <#return value description#>
  */
  bool IsDirty(IRECT& rect);

  /**
   /todo <#Description#>

   @param rect The rectangular area to draw/fill the shape in
  */
  virtual void Draw(const IRECT& rect);

  /**
   /todo <#Description#>

   @param name <#name description#>
   @return <#return value description#>
  */
  virtual ISVG LoadSVG(const char* name); // TODO: correct place?


  /** This method is called after interacting with a control, so that any other controls linked to the same parameter index, will also be set dirty, and have their values updated.
   * @param caller The control that triggered the parameter change. */
  void UpdatePeers(IControl* pCaller);

  /**
   /todo <#Description#>

   @param pControl <#pControl description#>
   @param pParam <#pParam description#>
   @param textRect <#textRect description#>
  */
  void PromptUserInput(IControl& control, IRECT& textRect);

  /**
   /todo <#Description#>

   @param pControl <#pControl description#>
   @param pParam <#pParam description#>
   @param txt <#txt description#>
  */
  void SetControlValueFromStringAfterPrompt(IControl& control, const char* txt);

  /**
   /todo <#Description#>

   @param menu <#menu description#>
   @param x <#x description#>
   @param y <#y description#>
   @return <#return value description#>
  */
  IPopupMenu* CreateIPopupMenu(IPopupMenu& menu, float x, float y) { IRECT tempRect = IRECT(x,y,x,y); return CreateIPopupMenu(menu, tempRect); }

  /**
   /todo <#Description#>

   @param strict <#strict description#>
  */
  void SetStrictDrawing(bool strict);

  /**
   /todo <#Description#>

   @return <#return value description#>
  */
  int Width() const { return mWidth; }

  /**
   /todo <#Description#>

   @return <#return value description#>
  */
  int Height() const { return mHeight; }

  /**
   /todo <#Description#>

   @return <#return value description#>
  */
  int WindowWidth() const { return int((float) mWidth * mScale); }

  /**
   /todo <#Description#>

   @return <#return value description#>
  */
  int WindowHeight() const { return int((float) mHeight * mScale); }

  /**
   /todo <#Description#>

   @return <#return value description#>
  */
  int FPS() const { return mFPS; }

  /**
   /todo <#Description#>

   @return <#return value description#>
  */
  float GetScale() const { return mScale; }

  /**
   /todo <#Description#>

   @return <#return value description#>
  */
  float GetDisplayScale() const { return mDisplayScale; }

  /**
   /todo <#Description#>

   @return <#return value description#>
  */
  IDelegate& GetDelegate() { return mDelegate; }

  void AttachBackground(const char* name);
  void AttachPanelBackground(const IColor& color);
  void AttachKeyCatcher(IControl& control);

  /**
   /todo <#Description#>

   @param control <#control description#>
   @return <#return value description#>
  */
  int AttachControl(IControl* pControl);

  /**
   /todo <#Description#>

   @param idx <#idx description#>
   @return <#return value description#>
  */
  IControl* GetControl(int idx) { return mControls.Get(idx); }

  /**
   /todo <#Description#>

   @return <#return value description#>
  */
  int NControls() const { return mControls.GetSize(); }


  /**
   /todo <#Description#>

   @param paramIdx <#paramIdx description#>
   @param hide <#hide description#>
  */
  void HideControl(int paramIdx, bool hide);

  /**
   /todo <#Description#>

   @param paramIdx <#paramIdx description#>
   @param gray <#gray description#>
  */
  void GrayOutControl(int paramIdx, bool gray);

  /**
   /todo <#Description#>

   @param paramIdx <#paramIdx description#>
   @param lo <#lo description#>
   @param hi <#hi description#>
   @param normalized <#normalized description#>
  */
  void ClampControl(int paramIdx, double lo, double hi, bool normalized);

  /**
   /todo <#Description#>
  */
  void SetAllControlsDirty();

  /**
   /todo <#Description#>

   @param x <#x description#>
   @param y <#y description#>
   @param mod <#mod description#>
  */
  void OnMouseDown(float x, float y, const IMouseMod& mod);

  /**
   /todo <#Description#>

   @param x <#x description#>
   @param y <#y description#>
   @param mod <#mod description#>
  */
  void OnMouseUp(float x, float y, const IMouseMod& mod);

  /**
   /todo <#Description#>

   @param x <#x description#>
   @param y <#y description#>
   @param dX <#dX description#>
   @param dY <#dY description#>
   @param mod <#mod description#>
  */
  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod);

  /**
   /todo <#Description#>

   @param x <#x description#>
   @param y <#y description#>
   @param mod <#mod description#>
   @return <#return value description#>
  */
  bool OnMouseDblClick(float x, float y, const IMouseMod& mod);

  /**
   /todo <#Description#>

   @param x <#x description#>
   @param y <#y description#>
   @param mod <#mod description#>
   @param d <#d description#>
  */
  void OnMouseWheel(float x, float y, const IMouseMod& mod, float d);

  /**
   /todo <#Description#>

   @param x <#x description#>
   @param y <#y description#>
   @param key <#key description#>
   @return <#return value description#>
  */
  bool OnKeyDown(float x, float y, int key);

  /**
   /todo <#Description#>

   @param x <#x description#>
   @param y <#y description#>
   @param mod <#mod description#>
   @return <#return value description#>
  */
  bool OnMouseOver(float x, float y, const IMouseMod& mod);

  /**
   /todo <#Description#>
  */
  void OnMouseOut();

  /**
   /todo <#Description#>

   @param str <#str description#>
   @param x <#x description#>
   @param y <#y description#>
  */

  /**
   /todo <#Description#>

   @param str <#str description#>
   @param x <#x description#>
   @param y <#y description#>
  */
  void OnDrop(const char* str, float x, float y);

  /**
   /todo <#Description#>
  */
  void OnGUIIdle();

  /**
   [AAX only]

   @param x <#x description#>
   @param y <#y description#>
   @return <#return value description#>
  */
  int GetParamIdxForPTAutomation(float x, float y);

  /**
   [AAX only]

   @return <#return value description#>
  */
  int GetLastClickedParamForPTAutomation();

  /**
   [AAX only]

   @param paramIdx <#paramIdx description#>
   @param isHighlighted <#isHighlighted description#>
   @param color The colour to draw/fill the shape with>
  */
  void SetPTParameterHighlight(int paramIdx, bool isHighlighted, int color);

  /**
   [VST3 primarily]

   @param controlIdx <#controlIdx description#>
   @param paramIdx <#paramIdx description#>
   @param x <#x description#>
   @param y <#y description#>
  */
  void PopupHostContextMenuForParam(int controlIdx, int paramIdx, float x, float y);

  /**
  /todo <#Description#>

  @param canHandle <#canHandle description#>
  */
  void HandleMouseOver(bool canHandle) { mHandleMouseOver = canHandle; }

  /**
   /todo <#Description#>
  */
  void ReleaseMouseCapture();

  /**
   /todo <#Description#>

   @param enable <#enable description#>
  */
  void EnableTooltips(bool enable);

  /**
   /todo <#Description#>
  */
  void AssignParamNameToolTips();

  /**
   /todo <#Description#>

   @param enable <#enable description#>
  */
  inline void ShowControlBounds(bool enable) { mShowControlBounds = enable; }


  /**
   /todo <#Description#>

   @param enable <#enable description#>
  */
  inline void ShowAreaDrawn(bool enable) { mShowAreaDrawn = enable; }

  /**
   /todo <#Description#>

   @param enable <#enable description#>
   @param file <#file description#>
   @param gridsize <#gridsize description#>
  */
  void EnableLiveEdit(bool enable, const char* file = 0, int gridsize = 10);

  /**
   /todo Get the area marked for drawing

   @return An IRECT that corresponds to the area currently marked for drawing
  */
  IRECT GetDrawRect() const { return mDrawRECT; }


  /**
   Get an IRECT that represents the entire UI bounds

   @return An IRECT that corresponds to the entire UI area, with, L = 0, T = 0, R = Width() and B  = Height()
   */
  IRECT GetBounds() const { return IRECT(0.f, 0.f, (float) Width(), (float) Height()); }

  /**
   /todo <#Description#>

   @return <#return value description#>
  */
  bool CanHandleMouseOver() const { return mHandleMouseOver; }

  /**
   /todo <#Description#>

   @return <#return value description#>
  */
  inline int GetMouseOver() const { return mMouseOver; }

  /**
   /todo <#Description#>

   @return <#return value description#>
  */
  inline bool TooltipsEnabled() const { return mEnableTooltips; }

  /**
   /todo <#Description#>

   @param name <#name description#>
  */
  virtual void LoadFont(const char* name);

protected:
  IDelegate& mDelegate;

  virtual APIBitmap* LoadAPIBitmap(const WDL_String& resourcePath, int scale) = 0;
  //virtual void* CreateAPIBitmap(int w, int h) = 0;
  virtual APIBitmap* ScaleAPIBitmap(const APIBitmap* pBitmap, int scale) = 0;

  inline void SearchNextScale(int& sourceScale, int targetScale);
  bool SearchImageResource(const char* name, const char* type, WDL_String& result, int targetScale, int& sourceScale);
  APIBitmap* SearchBitmapInCache(const char* name, int targetScale, int& sourceScale);

  WDL_PtrList<IControl> mControls;
  IRECT mDrawRECT;
  void* mPlatformContext = nullptr;
  bool mCursorHidden = false;
  bool mTabletInput = false;
    
private:
  int GetMouseControlIdx(float x, float y, bool mo = false);

  int mWidth;
  int mHeight;
  int mFPS;
  float mDisplayScale = 1.f; // the scaling of the display that the ui is currently on e.g. 2 for retina
  float mScale = 1.f; // scale deviation from dlg-in width and height i.e .stretching the gui by dragging
  int mIdleTicks = 0;
  int mMouseCapture = -1;
  int mMouseOver = -1;
  int mLastClickedParam = kNoParameter;
  bool mHandleMouseOver = false;
  bool mStrict = true;
  bool mEnableTooltips = false;
  bool mShowControlBounds = false;
  bool mShowAreaDrawn = false;
  IControl* mKeyCatcher = nullptr;

#if !defined(NDEBUG) && defined(APP_API)
  IControl* mLiveEdit = nullptr;
#endif

#ifdef IGRAPHICS_FREETYPE
protected:
  FT_Library mFTLibrary = nullptr;
  FT_Face mFTFace = nullptr;
#endif

  friend class IGraphicsLiveEdit;
};

