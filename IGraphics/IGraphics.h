/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc IGraphics
 * @defgroup IGraphicsStructs IGraphics::Structs
 * @defgroup DrawClasses IGraphics::DrawClasses
 * @defgroup PlatformClasses IGraphics::PlatformClasses
 * @defgroup Controls IGraphics::IControls
 * @defgroup BaseControls IGraphics::IControls::BaseControls
 * @defgroup SpecialControls IGraphics::IControls::SpecialControls
 * @defgroup TestControls IGraphics::IControls::TestControls
 */

#ifndef NO_IGRAPHICS
#if defined(IGRAPHICS_AGG) + defined(IGRAPHICS_CAIRO) + defined(IGRAPHICS_NANOVG) + defined(IGRAPHICS_LICE) + defined(IGRAPHICS_CANVAS) != 1
#error Either NO_IGRAPHICS or one and only one choice of graphics library must be defined!
#endif
#endif

#ifdef AAX_API
#include "IPlugAAX_view_interface.h"
#endif

#include "IPlugConstants.h"
#include "IPlugLogger.h"

#include "IGraphicsConstants.h"
#include "IGraphicsStructs.h"
#include "IGraphicsUtilities.h"
#include "IGraphicsPopupMenu.h"
#include "IGraphicsEditorDelegate.h"

#include "heapbuf.h"
#include <stack>
#include <memory>

#ifdef FillRect
#undef FillRect
#endif

#ifdef DrawText
#undef DrawText
#endif

class IControl;
class IPopupMenuControl;
class ITextEntryControl;
class ICornerResizerControl;
class IFPSDisplayControl;
class IParam;

/**  The lowest level base class of an IGraphics context */
class IGraphics
#ifdef AAX_API
: public IPlugAAXView_Interface
#endif
{

public:
#pragma mark - IGraphics drawing API implementation

  /** Called at the beginning of drawing. Call base implementation if overridden. */
  virtual void BeginFrame();
  
  /** Called after platform view initialization, so that drawing classes can e.g. create an OpenGL context. */
  virtual void OnViewInitialized(void* pContext) {};
  
  /** Called after a platform view is destroyed, so that drawing classes can e.g. free any resources */
  virtual void OnViewDestroyed() {}; //TODO: should happen before platform view is destroyed?

  /** Called by some drawing API classes to finally blit the draw bitmap onto the screen or perform other cleanup after drawing */
  virtual void EndFrame() {};

  /** Draw an SVG image to the graphics context
   * @param svg The SVG image to the graphics context
   * @param bounds The rectangular region to draw the image in
   * @param pBlend Optional blend method, see IBlend documentation */
  virtual void DrawSVG(ISVG& svg, const IRECT& bounds, const IBlend* pBlend = 0) = 0;

  /** Draw an SVG image to the graphics context with rotation
   * @param svg The SVG image to draw to the graphics context
   * @param destCentreX The X coordinate in the graphics context of the centre point at which to rotate the image around. \todo check this
   * @param destCentreY The Y coordinate in the graphics context of the centre point at which to rotate the image around. \todo check this
   * @param width \todo
   * @param height \todo
   * @param angle The angle to rotate the bitmap mask at in degrees clockwise
   * @param pBlend Optional blend method, see IBlend documentation */
  virtual void DrawRotatedSVG(ISVG& svg, float destCentreX, float destCentreY, float width, float height, double angle, const IBlend* pBlend = 0) = 0;

  /** Draw a bitmap (raster) image to the graphics context
   * @param bitmap The bitmap image to draw to the graphics context
   * @param bounds The rectangular region to draw the image in
   * @param srcX The X coordinate in the source image to draw from \todo
   * @param srcY The Y coordinate in the source image to draw from \todo
   * @param pBlend Optional blend method, see IBlend documentation */
  virtual void DrawBitmap(IBitmap& bitmap, const IRECT& bounds, int srcX, int srcY, const IBlend* pBlend = 0) = 0;

  /** Draw a bitmap (raster) image to the graphics context, scaling the image to fit the bounds
   * @param bitmap The bitmap image to draw to the graphics context
   * @param bounds The rectangular region to draw the image in
   * @param pBlend Optional blend method, see IBlend documentation */
  virtual void DrawFittedBitmap(IBitmap& bitmap, const IRECT& bounds, const IBlend* pBlend = 0) = 0;
  
  /** Draw a bitmap (raster) image to the graphics context with rotation
   * @param bitmap The bitmap image to draw to the graphics context
   * @param destCentreX The X coordinate in the graphics context of the centre point at which to rotate the image around. \todo check this
   * @param destCentreY The Y coordinate in the graphics context of the centre point at which to rotate the image around. \todo check this
   * @param angle The angle of rotation in degrees
   * @param yOffsetZeroDeg \todo
   * @param pBlend Optional blend method, see IBlend documentation */
  virtual void DrawRotatedBitmap(IBitmap& bitmap, float destCentreX, float destCentreY, double angle, int yOffsetZeroDeg = 0, const IBlend* pBlend = 0) = 0;

  /** Draw a rotated, masked bitmap to the graphics context
   * @param base The base bitmap image to draw to the graphics context \todo explain base
   * @param mask The mask bitmap image to draw to the graphics context \todo explain mask
   * @param top The bitmap image to draw to the graphics context \todo explain top
   * @param x The X coordinate in the graphics context at which to draw
   * @param y The Y coordinate in the graphics context at which to draw
   * @param angle The angle to rotate the bitmap mask at in degrees clockwise
   * @param pBlend Optional blend method, see IBlend documentation */
  virtual void DrawRotatedMask(IBitmap& base, IBitmap& mask, IBitmap& top, float x, float y, double angle, const IBlend* pBlend = 0) = 0;

  /** Fill a rectangle corresponding to a pixel on a 1:1 screen with a color
   * @param color The color to fill the point with
   * @param x The X coordinate in the graphics context at which to draw
   * @param y The Y coordinate in the graphics context at which to draw
   * @param pBlend Optional blend method, see IBlend documentation */
  virtual void DrawPoint(const IColor& color, float x, float y, const IBlend* pBlend = 0) = 0;

  /** Draw a line to the graphics context
   * @param color The color to draw the line with
   * @param x1 The X coordinate in the graphics context of the start of the line
   * @param y1 The Y coordinate in the graphics context of the start of the line
   * @param x2 The X coordinate in the graphics context of the end of the line
   * @param y2 The Y coordinate in the graphics context of the end of the line
   * @param pBlend Optional blend method, see IBlend documentation
   * @param thickness Optional line thickness */
  virtual void DrawLine(const IColor& color, float x1, float y1, float x2, float y2, const IBlend* pBlend = 0, float thickness = 1.f) = 0;

  /** Draw a dotted line to the graphics context
   * @param color The color to draw the shape with
   * @param x1 The X coordinate in the graphics context of the start of the line
   * @param y1 The Y coordinate in the graphics context of the start of the line
   * @param x2 The X coordinate in the graphics context of the end of the line
   * @param y2 The Y coordinate in the graphics context of the end of the line
   * @param pBlend Optional blend method, see IBlend documentation
   * @param thickness Optional line thickness */
  virtual void DrawDottedLine(const IColor& color, float x1, float y1, float x2, float y2, const IBlend* pBlend = 0, float thickness = 1.f, float dashLen = 2.f) = 0;
  
  /** Draw a triangle to the graphics context
   * @param color The color to draw the shape with
   * @param x1 The X coordinate in the graphics context of the first vertex
   * @param y1 The Y coordinate in the graphics context of the first vertex
   * @param x2 The X coordinate in the graphics context of the second vertex
   * @param y2 The Y coordinate in the graphics context of the second vertex
   * @param x3 The X coordinate in the graphics context of the third vertex
   * @param y3 The Y coordinate in the graphics context of the third vertex
   * @param pBlend Optional blend method, see IBlend documentation
   * @param thickness Optional line thickness */
  virtual void DrawTriangle(const IColor& color, float x1, float y1, float x2, float y2, float x3, float y3, const IBlend* pBlend = 0, float thickness = 1.f) = 0;

  /** Draw a rectangle to the graphics context
   * @param bounds The rectangular area in which to draw the shape
   * @param color The color to draw the shape to draw the shape with
   * @param pBlend Optional blend method, see IBlend documentation
   * @param thickness Optional line thickness */
  virtual void DrawRect(const IColor& color, const IRECT& bounds, const IBlend* pBlend = 0, float thickness = 1.f) = 0;

  /** Draw a rounded rectangle to the graphics context
   * @param color The color to draw the shape with
   * @param bounds The rectangular region to draw the shape in
   * @param cornerRadius The corner radius in pixels
   * @param pBlend Optional blend method, see IBlend documentation
   * @param thickness Optional line thickness */
  virtual void DrawRoundRect(const IColor& color, const IRECT& bounds, float cornerRadius = 5.f, const IBlend* pBlend = 0, float thickness = 1.f) = 0;

  /** Draw a rounded rectangle to the graphics context with individual corner roundness
   * @param color The color to draw the shape with
   * @param bounds The rectangular region to draw the shape in
   * @param cRTL The top left corner radius in pixels
   * @param cRTR The top right corner radius in pixels
   * @param cRBR The bottom right corner radius in pixels
   * @param cRBL The bottom left corner radius in pixels
   * @param pBlend Optional blend method, see IBlend documentation
   * @param thickness Optional line thickness */
  virtual void DrawRoundRect(const IColor& color, const IRECT& bounds, float cRTL, float cRTR, float cRBR, float cRBL, const IBlend* pBlend = 0, float thickness = 1.f) = 0;
  
  /** Draw an arc to the graphics context
   * @param color The color to draw the shape with
   * @param cx The X coordinate in the graphics context of the centre of the circle on which the arc lies
   * @param cy The Y coordinate in the graphics context of the centre of the circle on which the arc lies
   * @param r The radius of the circle on which the arc lies
   * @param aMin the start angle  of the arc at in degrees clockwise where 0 is up
   * @param aMax the end angle  of the arc at in degrees clockwise where 0 is up
   * @param pBlend Optional blend method, see IBlend documentation
   * @param thickness Optional line thickness */
  virtual void DrawArc(const IColor& color, float cx, float cy, float r, float aMin, float aMax, const IBlend* pBlend = 0, float thickness = 1.f) = 0;

  /** Draw a circle to the graphics context
   * @param color The color to draw the shape with
   * @param cx The X coordinate in the graphics context of the centre of the circle
   * @param cy The Y coordinate in the graphics context of the centre of the circle
   * @param r The radius of the circle
   * @param pBlend Optional blend method, see IBlend documentation
   * @param thickness Optional line thickness */
  virtual void DrawCircle(const IColor& color, float cx, float cy, float r, const IBlend* pBlend = 0, float thickness = 1.f) = 0;
  
  /** Draw an ellipse within a rectangular region of the graphics context
   * @param color The color to draw the shape with
   * @param bounds The rectangular region to draw the shape in
   * @param pBlend Optional blend method, see IBlend documentation
   * @param thickness Optional line thickness */
  virtual void DrawEllipse(const IColor& color, const IRECT& bounds, const IBlend* pBlend = 0, float thickness = 1.f) = 0;
  
  /** Draw an ellipse around a central point given two radii and an angle of orientation
   * @param color The color to draw the shape with
   * @param x The X coordinate in the graphics context of the centre of the ellipse
   * @param y The Y coordinate in the graphics context of the centre of the ellipse
   * @param r1 The radius of the ellipse along the line found by rotating the x-axis by the angle
   * @param r2 The radius of the ellipse along the line found by rotating the y-axis by the angle
   * @param angle The angle rotates the radii r1 and r2 clockwise in degrees to adjust the orientation
   * @param pBlend Optional blend method, see IBlend documentation
   * @param thickness Optional line thickness */
  virtual void DrawEllipse(const IColor& color, float x, float y, float r1, float r2, float angle = 0.0, const IBlend* pBlend = 0, float thickness = 1.f) = 0;

  /** Draw a convex polygon to the graphics to the graphics context
   * @param color The color to draw the shape with
   * @param x Pointer to the first element in an array of X coordinates for the vertices of the polygon
   * @param y Pointer to the first element in an array of Y coordinates for the vertices of the polygon
   * @param nPoints The number of points in the coordinate arrays
   * @param pBlend Optional blend method, see IBlend documentation
   * @param thickness Optional line thickness */
  virtual void DrawConvexPolygon(const IColor& color, float* x, float* y, int nPoints, const IBlend* pBlend = 0, float thickness = 1.f) = 0;

  /** Draw a dotted rectangle to the graphics context
   * @param color The color to draw the shape with
   * @param bounds The rectangular region to draw the shape in
   * @param pBlend Optional blend method, see IBlend documentation
   * @param thickness Optional line thickness */
  virtual void DrawDottedRect(const IColor& color, const IRECT& bounds, const IBlend* pBlend = 0, float thickness = 1.f, float dashLen = 2.f) = 0;

  /** Fill a triangle in the graphics context with a color
   * @param color The color to fill the shape with
   * @param x1 The X coordinate in the graphics context of the first vertex
   * @param y1 The Y coordinate in the graphics context of the first vertex
   * @param x2 The X coordinate in the graphics context of the second vertex
   * @param y2 The Y coordinate in the graphics context of the second vertex
   * @param x3 The X coordinate in the graphics context of the third vertex
   * @param y3 The Y coordinate in the graphics context of the third vertex
   * @param pBlend Optional blend method, see IBlend documentation */
  virtual void FillTriangle(const IColor& color, float x1, float y1, float x2, float y2, float x3, float y3, const IBlend* pBlend = 0) = 0;

  /** Fill a rectangular region of the graphics context with a color
   * @param color The color to fill the shape with
   * @param bounds The rectangular region to fill the shape in
   * @param pBlend Optional blend method, see IBlend documentation */
  virtual void FillRect(const IColor& color, const IRECT& bounds, const IBlend* pBlend = 0) = 0;

  /** Fill a rounded rectangle in the graphics context with a color
   * @param color The color to fill the shape with
   * @param bounds The rectangular region to fill the shape in
   * @param cornerRadius The corner radius in pixels
   * @param pBlend Optional blend method, see IBlend documentation */
  virtual void FillRoundRect(const IColor& color, const IRECT& bounds, float cornerRadius = 5.f, const IBlend* pBlend = 0) = 0;

  /** Fill a rounded rectangle in the graphics context with a color
   * @param color The color to fill the shape with
   * @param bounds The rectangular region to fill the shape in
   * @param cRTL The top left corner radius in pixels
   * @param cRTR The top right corner radius in pixels
   * @param cRBR The bottom right corner radius in pixels
   * @param cRBL The bottom left corner radius in pixels
   * @param pBlend Optional blend method, see IBlend documentation */
  virtual void FillRoundRect(const IColor& color, const IRECT& bounds, float cRTL, float cRTR, float cRBR, float cRBL, const IBlend* pBlend = 0) = 0;
  
  /** Fill a circle in the graphics context with a color
   * @param color The color to fill the shape with
   * @param cx The X coordinate in the graphics context of the centre of the circle
   * @param cy The Y coordinate in the graphics context of the centre of the circle
   * @param r The radius of the circle
   * @param pBlend Optional blend method, see IBlend documentation */
  virtual void FillCircle(const IColor& color, float cx, float cy, float r, const IBlend* pBlend = 0) = 0;

  /** Fill an ellipse within a rectangular region of the graphics context
   * @param color The color to fill the shape with
   * @param bounds The rectangular region to fill the shape in
   * @param pBlend Optional blend method, see IBlend documentation */
  virtual void FillEllipse(const IColor& color, const IRECT& bounds, const IBlend* pBlend = 0) = 0;
  
  /** Fill an ellipse in the graphics context
   * @param color The color to draw the shape with
   * @param x The X coordinate in the graphics context of the centre of the ellipse
   * @param y The Y coordinate in the graphics context of the centre of the ellipse
   * @param r1 The radius of the ellipse along the line found by rotating the x-axis by the angle
   * @param r2 The radius of the ellipse along the line found by rotating the y-axis by the angle
   * @param angle The angle rotates the radii r1 and r2 clockwise in degrees to adjust the orientation
   * @param pBlend Optional blend method, see IBlend documentation */
  virtual void FillEllipse(const IColor& color, float x, float y, float r1, float r2, float angle = 0.0, const IBlend* pBlend = 0) = 0;
  
  /** Fill an arc segment in the graphics context with a color
   * @param color The color to fill the shape with
   * @param cx The X coordinate in the graphics context of the centre of the circle on which the arc lies
   * @param cy The Y coordinate in the graphics context of the centre of the circle on which the arc lies
   * @param r The radius of the circle on which the arc lies
   * @param aMin the start angle  of the arc at in degrees clockwise where 0 is up
   * @param aMax the end angle  of the arc at in degrees clockwise where 0 is up
   * @param pBlend Optional blend method, see IBlend documentation */
  virtual void FillArc(const IColor& color, float cx, float cy, float r, float aMin, float aMax, const IBlend* pBlend = 0) = 0;

  /** Fill a convex polygon in the graphics context with a color
   * @param color The color to fill the shape with
   * @param x Pointer to the first element in an array of X coordinates for the vertices of the polygon
   * @param y Pointer to the first element in an array of Y coordinates for the vertices of the polygon
   * @param nPoints The number of points in the coordinate arrays
   * @param pBlend Optional blend method, see IBlend documentation */
  virtual void FillConvexPolygon(const IColor& color, float* x, float* y, int nPoints, const IBlend* pBlend = 0) = 0;

  /** Draw some text to the graphics context in a specific rectangle
   * @param text An IText struct containing font and text properties and layout info
   * @param str The text string to draw in the graphics context
   * @param bounds The rectangular region in the graphics where you would like to draw the text
   * @return \todo */
  bool DrawText(const IText& text, const char* str, const IRECT& bounds, const IBlend* pBlend = 0);

  /** Draw some text to the graphics context at a point
   * @param text An IText struct containing font and text properties and layout info
   * @param str The text string to draw in the graphics context
   * @param x The x position in the graphics where you would like to draw the text
   * @param y The y position in the graphics where you would like to draw the text
   * @return \todo */
  bool DrawText(const IText& text, const char* str, float x, float y, const IBlend* pBlend = 0);
  
  /** Measure the rectangular region that some text will occupy
   * @param text An IText struct containing font and text properties and layout info
   * @param str The text string to draw in the graphics context
   * @param bounds after calling the method this IRECT will be updated with the rectangular region the text will occupy
   * @return \todo */
  virtual bool MeasureText(const IText& text, const char* str, IRECT& bounds);

  /** Get the color of a point in the graphics context. On a 1:1 screen this corresponds to a pixel. \todo check this
   * @param x The X coordinate in the graphics context of the pixel
   * @param y The Y coordinate in the graphics context of the pixel
   * @return An IColor specifiying the color of the pixel at x,y */
  virtual IColor GetPoint(int x, int y)  = 0;

  /** Gets a void pointer to IGraphics Draw Class context data (e.g raw framebuffer).
   * See draw class implementation headers (e.g. IGraphicsLice.h) for what you can cast the void pointer to */
   virtual void* GetDrawContext() = 0;

  /** @return A CString representing the Drawing API in use e.g. "LICE" */
  virtual const char* GetDrawingAPIStr() = 0;

#pragma mark - IGraphics drawing API implementation (bitmap handling)
  virtual IBitmap ScaleBitmap(const IBitmap& srcbitmap, const char* cacheName, int targetScale);
  virtual void RetainBitmap(const IBitmap& bitmap, const char* cacheName);
  virtual void ReleaseBitmap(const IBitmap& bitmap);
  IBitmap GetScaledBitmap(IBitmap& src);
  
  /** Checks a file extension and reports whether this drawing API supports loading that extension */
  virtual bool BitmapExtSupported(const char* ext) = 0;
  
#pragma mark - IGraphics base implementation - drawing helpers

  /** Draws a bitmap into the graphics context. NOTE: this helper method handles multi-frame bitmaps, indexable via frame
   * @param bitmap - the bitmap to draw
   * @param bounds - where to draw the bitmap
   * @param frame - the frame index of the bitmap to draw (when bitmap is multi-frame)
   * @param pBlend - blend operation */
  void DrawBitmap(IBitmap& bitmap, const IRECT& bounds, int frame = 1, const IBlend* pBlend = 0);

  /** Draws mono spaced bitmap text. Useful for identical looking text on multiple platforms.
   * @param bitmap the bitmap containing glyphs to draw
   * @param bounds where to draw the bitmap
   * @param text text properties (note - many of these are irrelevant for bitmapped text)
   * @param pBlend blend operation
   * @param str the string to draw
   * @param vCenter centre the text vertically
   * @param multiline should the text spill onto multiple lines
   * @param charWidth how wide is a character in the bitmap
   * @param charHeight how high is a character in the bitmap
   * @param charOffset what is the offset between characters drawn */
  void DrawBitmapedText(IBitmap& bitmap, IRECT& bounds, IText& text, IBlend* pBlend, const char* str, bool vCenter = true, bool multiline = false, int charWidth = 6, int charHeight = 12, int charOffset = 0);

  /** Draw a vertical line, within a rectangular region of the graphics context
   * @param color The color to draw the line with
   * @param bounds The rectangular region to draw the line in
   * @param x \todo
   * @param pBlend Optional blend method, see IBlend documentation
   * @param thickness Optional line thickness */
  void DrawVerticalLine(const IColor& color, const IRECT& bounds, float x, const IBlend* pBlend = 0, float thickness = 1.f);

  /** Draw a horizontal line, within a rectangular region of the graphics context
   * @param color The color to draw the line with
   * @param bounds The rectangular region to draw the line in
   * @param y \todo
   * @param pBlend Optional blend method, see IBlend documentation
   * @param thickness Optional line thickness */
  void DrawHorizontalLine(const IColor& color, const IRECT& bounds, float y, const IBlend* pBlend = 0, float thickness = 1.f);

  /** \todo
   * @param color The color to draw the line with
   * @param xi \todo
   * @param yLo \todo
   * @param yHi \todo
   * @param pBlend Optional blend method, see IBlend documentation*/
  void DrawVerticalLine(const IColor& color, float xi, float yLo, float yHi, const IBlend* pBlend = 0, float thickness = 1.f);

  /** \todo
   * @param color The color to draw the line with
   * @param xi \todo
   * @param yLo \todo
   * @param yHi \todo
   * @param pBlend Optional blend method, see IBlend documentation*/
  void DrawHorizontalLine(const IColor& color, float yi, float xLo, float xHi, const IBlend* pBlend = 0, float thickness = 1.f);

  /** Draw a radial line to the graphics context, useful for pointers on dials
   * @param color The color to draw the line with
   * @param cx centre point x coordinate
   * @param cy centre point y coordinate
   * @param angle The angle to draw at in degrees clockwise where 0 is up
   * @param rMin minima of the radial line (distance from cx,cy)
   * @param rMax maxima of the radial line (distance from cx,cy)
   * @param pBlend Optional blend method, see IBlend documentation
   * @param thickness Optional line thickness */
  void DrawRadialLine(const IColor& color, float cx, float cy, float angle, float rMin, float rMax, const IBlend* pBlend = 0, float thickness = 1.f);

  /** Draw a grid to the graphics context
   * @param color The color to draw the grid lines with
   * @param bounds The rectangular region to fill the grid in
   * @param gridSizeH The width of the grid cells
   * @param gridSizeV The height of the grid cells
   * @param pBlend Optional blend method, see IBlend documentation
   * @param thickness Optional line thickness */
  virtual void DrawGrid(const IColor& color, const IRECT& bounds, float gridSizeH, float gridSizeV, const IBlend* pBlend = 0, float thickness = 1.f);

  virtual void DrawData(const IColor& color, const IRECT& bounds, float* normYPoints, int nPoints, float* normXPoints = nullptr, const IBlend* pBlend = 0, float thickness = 1.f);
  
#pragma mark - IGraphics drawing API layer support
    
  void StartLayer(const IRECT& r);
  void ResumeLayer(ILayerPtr& layer);
  ILayerPtr EndLayer();
  bool CheckLayer(const ILayerPtr& layer);
  void DrawLayer(const ILayerPtr& layer);
  void DrawRotatedLayer(const ILayerPtr& layer, double angle);
    
  /** Applies a dropshadow directly onto a layer
  * @param layer - the layer to add the shadow to 
  * @param shadow - the shadow to add */
  void ApplyLayerDropShadow(ILayerPtr& layer, const IShadow& shadow);
    
private:
  virtual void UpdateLayer() {}

public:
  
#pragma mark - IGraphics drawing API Path support

  virtual bool HasPathSupport() const { return false; }

  virtual void PathClear() {}
  virtual void PathClose() {}

  void PathLine(float x1, float y1, float x2, float y2)
  {
    PathMoveTo(x1, y1);
    PathLineTo(x2, y2);
  }

  void PathRadialLine(float cx, float cy, float angle, float rMin, float rMax);

  virtual void PathTriangle(float x1, float y1, float x2, float y2, float x3, float y3) {}
  virtual void PathRect(const IRECT& bounds) {}
  virtual void PathRoundRect(const IRECT& bounds, float ctl, float ctr, float cbl, float cbr) {}
  virtual void PathRoundRect(const IRECT& bounds, float cr = 5.f) {}
  virtual void PathArc(float cx, float cy, float r, float aMin, float aMax) {}
  virtual void PathCircle(float cx, float cy, float r) {}
  virtual void PathEllipse(float x, float y, float r1, float r2, float angle = 0.0) {}
  virtual void PathEllipse(const IRECT& bounds) {}
  virtual void PathConvexPolygon(float* x, float* y, int nPoints) {}

  virtual void PathMoveTo(float x, float y) {}
  virtual void PathLineTo(float x, float y) {}
  virtual void PathCurveTo(float x1, float y1, float x2, float y2, float x3, float y3) {}

  virtual void PathStroke(const IPattern& pattern, float thickness, const IStrokeOptions& options = IStrokeOptions(), const IBlend* pBlend = 0) {}
  virtual void PathFill(const IPattern& pattern, const IFillOptions& options = IFillOptions(), const IBlend* pBlend = 0) {}

  virtual void PathTransformSave() {}
  virtual void PathTransformRestore() {}
  virtual void PathTransformReset(bool clearStates = false) {}
  virtual void PathTransformTranslate(float x, float y) {}
  virtual void PathTransformScale(float scaleX, float scaleY) {}
  virtual void PathTransformScale(float scale) {}
  virtual void PathTransformRotate(float angle) {}
  virtual void PathTransformSkew(float xAngle, float yAngle) {}
  virtual void PathTransformMatrix(const IMatrix& matrix) {}

  virtual void PathClipRegion(const IRECT r = IRECT()) {}
  
  virtual void DrawBoxShadow(const IRECT& bounds, float cr = 0.f, float ydrop = 2.f, float pad = 10.f, const IBlend* pBlend = 0) {};

private:
    
  /** This is used to prepare a particular area of the display for drawing, normally resulting in clipping of the region.
   * @param bounds The rectangular region to prepare  */
  virtual void PrepareRegion(const IRECT& bounds) = 0;
 
public:
    
#pragma mark - IGraphics platform implementation
  /** Call to hide the mouse cursor */ 
  virtual void HideMouseCursor(bool hide = true, bool lock = true) = 0;

  /** Force move the mouse cursor to a specific position in the graphics context
   * @param x New X position in pixels
   * @param y New Y position in pixels */
  virtual void MoveMouseCursor(float x, float y) = 0;
  
  /** Sets the mouse cursor to one of ECursor
   * @param cursor The cursor type */
  virtual void SetMouseCursor(ECursor cursor = ECursor::ARROW) = 0;

  /** Call to force end text entry (will cancel any half input text \todo check) */
  virtual void ForceEndUserEdit() = 0;
    
  /** Open a new platform view for this graphics context
   * @param pParentWnd void pointer to parent platform window or view handle (if applicable) \todo check
   * @return void pointer to newly created IGraphics platform view */
  virtual void* OpenWindow(void* pParentWnd) = 0;

  /** Close the platform view for this graphics context */
  virtual void CloseWindow() = 0;

  /** Get a pointer to the platform view e.g. HWND or NSView for this graphics context
   * return void pointer to platform window or view handle */
  virtual void* GetWindow() = 0;

  /** @return /true if the platform window/view is open */
  virtual bool WindowIsOpen() { return GetWindow(); }

  /** Get text from the clipboard
   * @param str A WDL_String that will be filled with the text that is currently on the clipboard
   * @return /c true on success */
  virtual bool GetTextFromClipboard(WDL_String& str) = 0;

  /** Call this if you modify control tool tips at runtime. \todo explain */
  virtual void UpdateTooltips() = 0;

  /** Pop up a modal platform message box dialog. NOTE: this method will block the main thread
   * @param str The text message to display in the dialogue
   * @param caption The title of the message box window \todo check
   * @param type EMessageBoxType describing the button options available \see EMessageBoxType
   * @return \todo check */
  virtual int ShowMessageBox(const char* str, const char* caption, EMessageBoxType type) = 0;

  /** Create a text entry box
   * @param control The control that the text entry belongs to. If this control is linked to a parameter, the text entry will be configured with initial text matching the parameter value
   * @param text An IText struct to set the formatting of the text entry box
   * @param bounds The rectangular region in the graphics context that the text entry will occupy.
   * @param str A CString to specify the default text to display when the text entry box is opened (unless the control specified by the first argument is linked to a parameter) */
  void CreateTextEntry(IControl& control, const IText& text, const IRECT& bounds, const char* str = "");

  /** Create a platform file prompt dialog to choose a file/directory path for opening/saving a file/directory. NOTE: this method will block the main thread
   * @param fileName Non const WDL_String reference specifying the file name. Set this prior to calling the method for save dialogs, to provide a default file name. For load dialogs, on successful selection of a file this will get set to the file’s name.
   * @param path WDL_String reference where the path will be put on success or empty string on failure/user cancelled
   * @param action Determines whether this is an open dialog or a save dialog
   * @param extensions A comma separated CString list of file extensions to filter in the dialog (e.g. “.wav, .aif” \todo check */
  virtual void PromptForFile(WDL_String& fileName, WDL_String& path, EFileAction action = kFileOpen, const char* extensions = 0) = 0;

  /** Create a platform file prompt dialog to choose a directory path for opening/saving a directory. NOTE: this method will block the main thread
   * @param dir Non const WDL_String reference specifying the directory path. Set this prior to calling the method for save dialogs, to provide a default path. For load dialogs, on successful selection of a directory this will get set to the full path. */
  virtual void PromptForDirectory(WDL_String& dir) = 0;

  /** Create a platform color chooser dialog. NOTE: this method will block the main thread
   * @param color When a color is chosen the IColor referenced will be updated with the new color
   * @param str The text to display in the dialog box e.g. "Please choose a color..."
   * @return /true if prompt completed successfully */
  virtual bool PromptForColor(IColor& color, const char* str = "") = 0;
  
  virtual void CreateWebView(const IRECT& bounds, const char* url) {};

  /** Open a URL in the platform’s default browser
   * @param url CString specifying the URL to open
   * @param msgWindowTitle \todo ?
   * @param confirmMsg \todo ?
   * @param errMsgOnFailure \todo ?
   * @return /true on success */
  virtual bool OpenURL(const char* url, const char* msgWindowTitle = 0, const char* confirmMsg = 0, const char* errMsgOnFailure = 0) = 0;

  /** @return A CString representing the Platform API in use e.g. "macOS" */
  virtual const char* GetPlatformAPIStr() { return ""; }

  /** @param path WDL_String reference where the path will be put on success or empty string on failure
   * @param select et \c true if you want to select the item in Explorer/Finder
   * @return \c true on success (if the path was valid) */
  virtual bool RevealPathInExplorerOrFinder(WDL_String& path, bool select = false) { return false; }

  /** Used on Windows to set the HINSTANCE module handle, which allows graphics APIs to load resources from the binary.
   * @param pHinstance void pointer to the platform instance */
  virtual void SetWinModuleHandle(void* pHinstance) {}

  /** @return a void pointer that can be cast back to HINSTANCE to get the module handle on windows, returns nullptr on other platforms */
  virtual void* GetWinModuleHandle() { return nullptr; }

  /** Set the platform draw context
   * Used in order to set the platform level draw context - CGContextRef context on macOS and the GDI HDC draw context handle on Windows.
   * @param pContext void pointer to CGContextRef or HDC */
  virtual void SetPlatformContext(void* pContext) { mPlatformContext = pContext; }

  /** Get the platform level draw context - an HDC or CGContextRef
   * @return void pointer to an HDC or CGContext */
  void* GetPlatformContext() { return mPlatformContext; }
  
  /** Convert an x, y position in the view to screen coordinates
   * @param x the x position to convert
   * @param y the y position to convert */
  virtual void ClientToScreen(float& x, float& y) {};

  /** Find the absolute path of a resource based on it's file name (e.g. “background.png”) and type (e.g. “png”), or in the case of windows, 
   * confirm the existence of a particular resource in the binary. If it fails to find the resource with the binary it will test the fileNameOrResID argument
   * as an absolute path, to see if the file exists in that place.
   * On macOS resources are usually included inside the bundle resources folder.
   * On Windows resources are usually baked into the binary via the resource compiler. In this case the fileName argument is the resource id to look for. 
   * The .rc file must include these ids, otherwise you may hit a runtime assertion when you come to load the file.
   * In some cases you may want to provide an absolute path to a file in a shared resources folder
   * here (for example if you want to reduce the disk footprint of multiple bundles, such as when you have multiple plug-in formats installed).
   *
   * @param fileNameOrResID The filename or resourceID including extension. If no resource is found this argument is tested as an absolute path.
   * @param type The resource type (file extension) in lower or upper case, e.g. ttf or TTF for a truetype font 
   * @param result WDL_String which will either contain the full path to the resource on disk, or the ful Windows resourceID on success
   * @return \c true on success */
  virtual EResourceLocation OSFindResource(const char* fileNameOrResID, const char* type, WDL_String& result) = 0;

  /** Load a resource from the binary (windows only).
   * @param type The resource type in lower or upper case, e.g. ttf or TTF for a truetype font
   * @return const void pointer to the data if successfull on windows. Returns nullptr if unsuccessfull or on platforms other than windows */
  virtual const void* LoadWinResource(const char* resID, const char* type, int& sizeInBytes) { return nullptr; }

  /** Get the bundle ID on macOS and iOS, returns emtpy string on other OSs */
  virtual const char* GetBundleID() { return ""; }
#pragma mark - IGraphics base implementation
  IGraphics(IGEditorDelegate& dlg, int w, int h, int fps = 0, float scale = 1.);

  virtual ~IGraphics();

  /** Called by the platform IGraphics class XXXXX /todo and when moving to a new screen with different DPI
   * @param scale The scale of the display, typically 2 on a macOS retina screen */
  void SetScreenScale(int scale);
    
  /** Called repeatedly at frame rate by the platform class to check what the graphics context says is dirty
   * @param bounds The rectangular region which will be added to to mark what is dirty in the context
   * @return /c true if a control is dirty */
  bool IsDirty(IRECTList& rects);

  /** Called by the platform class when an area needs to be redrawn
   * @param rects A set of rectangular regions to draw */
  void Draw(IRECTList& rects);

  /** This method is called after interacting with a control, so that any other controls linked to the same parameter index, will also be set dirty, and have their values updated.
   * @param pCaller The control that triggered the parameter change. */
  void UpdatePeers(IControl* pCaller);

  /** Prompt for user input either using a text entry or pop up menu
   * @param control Reference to the control which the prompt relates to
   * @param bounds Rectangular region of the graphics context that the prompt (e.g. text entry box) should occupy */
  void PromptUserInput(IControl& control, const IRECT& bounds);

  /** Called by the platform class after returning from a prompt (typically a text entry) in order to update a control with a new value
   * @param control Reference to the control which the call relates to
   * @param str The new value as a CString */
  void SetControlValueFromStringAfterPrompt(IControl& control, const char* str);

  /** Shows a pop up/contextual menu in relation to a rectangular region of the graphics context
   * @param menu Reference to an IPopupMenu class populated with the items for the platform menu
   * @param bounds The platform menu will popup at the bottom left hand corner of this rectangular region
   * @param pCaller A pointed to the IControl creating this pop-up menu. If it exists IControl::OnPopupMenuSelection() will be called on successful selection
   * @return Pointer to an IPopupMenu that represents the menu that user finally clicked on (might not be the same as menu if they clicked a submenu) */
  IPopupMenu* CreatePopupMenu(IPopupMenu& menu, const IRECT& bounds, IControl* pCaller = nullptr);

  /** Shows a pop up/contextual menu at point in the graphics context
   * @param x The X coordinate in the graphics context at which to pop up the menu
   * @param y The Y coordinate in the graphics context at which to pop up the menu
   * @param pCaller A pointer to the IControl creating this pop-up menu. If it exists IControl::OnPopupMenuSelection() will be called on successful selection
   * @return Pointer to an IPopupMenu that represents the menu that user finally clicked on (might not be the same as menu if they clicked a submenu) */
  IPopupMenu* CreatePopupMenu(IPopupMenu& menu, float x, float y, IControl* pCaller = nullptr) { const IRECT bounds = IRECT(x,y,x,y); return CreatePopupMenu(menu, bounds, pCaller); }

  void SetScaleConstraints(float lo, float hi)
  {
    mMinScale = std::min(lo, hi);
    mMaxScale = std::max(lo, hi);
  }
  
  void SetSizeConstraints(int widthLo, int widthHi, int heightLo, int heightHi)
  {
    mMinWidth = std::min(widthLo, widthHi);
    mMaxWidth = std::max(widthLo, widthHi);
    mMinHeight = std::min(heightLo, heightHi);
    mMaxHeight = std::max(heightLo, heightHi);
  }
  
  /** \todo detailed description of how this works
   * @param w New width in pixels
   * @param h New height in pixels
   * @param scale New scale ratio */
  void Resize(int w, int h, float scale);
  
  /** Enables strict drawing mode. \todo explain strict drawing
   * @param strict Set /true to enable strict drawing mode */
  void SetStrictDrawing(bool strict);

  /** Gets the width of the graphics context
   * @return A whole number representing the width of the graphics context in pixels on a 1:1 screen */
  int Width() const { return mWidth; }

  /** Gets the height of the graphics context
   * @return A whole number representing the height of the graphics context in pixels on a 1:1 screen */
  int Height() const { return mHeight; }

  /** Gets the width of the graphics context including scaling (not display scaling!)
   * @return A whole number representing the width of the graphics context with scaling in pixels on a 1:1 screen */
  int WindowWidth() const { return int((float) mWidth * mDrawScale); }

  /** Gets the height of the graphics context including scaling (not display scaling!)
   * @return A whole number representing the height of the graphics context with scaling in pixels on a 1:1 screen */
  int WindowHeight() const { return int((float) mHeight * mDrawScale); }

  /** Gets the drawing frame rate
   * @return A whole number representing the desired frame rate at which the graphics context is redrawn. NOTE: the actual frame rate might be different */
  int FPS() const { return mFPS; }

  /** Gets the graphics context scaling factor.
   * @return The scaling applied to the graphics context */
  float GetDrawScale() const { return mDrawScale; }

  /** Gets the display scaling factor
    * @return The scale factor of the display on which this graphics context is currently located */
  int GetScreenScale() const { return mScreenScale; }

  /** Gets a pointer to the delegate class that handles communication to and from this graphics context.
   * @return pointer to the delegate */
  IGEditorDelegate* GetDelegate() { return &mDelegate; }

  /** Attach an IBitmapControl as the lowest IControl in the control stack to be the background for the graphics context
   * @param fileName CString fileName resource id for the bitmap image \todo check this */
  void AttachBackground(const char* fileName);

  /** Attach an IPanelControl as the lowest IControl in the control stack to fill the background with a solid color
   * @param color The color to fill the panel with */
  void AttachPanelBackground(const IColor& color);
  
  /** Attach the default control to scale or increase the UI size by dragging the plug-in bottom right-hand corner
   * @param sizeMode Choose whether to scale or size the UI */
  void AttachCornerResizer(EUIResizerMode sizeMode = EUIResizerMode::kUIResizerScale, bool layoutOnResize = false);

  /** Attach your own control to scale or increase the UI size by dragging the plug-in bottom right-hand corner
   * @param pControl control a control that inherits from ICornerResizerControl
   * @param sizeMode Choose whether to scale or size the UI */
  void AttachCornerResizer(ICornerResizerControl* pControl, EUIResizerMode sizeMode = EUIResizerMode::kUIResizerScale, bool layoutOnResize = false);

  /** Attach a control for pop-up menus, to override platform style menus
   * @param pControl A control that inherits from IPopupMenuControl */
  void AttachPopupMenuControl(const IText& text = DEFAULT_TEXT, const IRECT& bounds = IRECT());
  
  void SetKeyHandlerFunc(std::function<bool(const IKeyPress& key)> keyHandlerFunc) { mKeyHandlerFunc = keyHandlerFunc; }
  
  /** Shows a control to display the frame rate of drawing
   * @param enable \c true to show */
  void ShowFPSDisplay(bool enable);
  
  /** @return \c true if performance display is shown */
  bool ShowingFPSDisplay() { return mPerfDisplay != nullptr; }
  /** Attach a control for displaying the FPS on top of the UI */
  void AttachPerformanceDisplay();
  
  /** Attach a control for text entry, to override platform text entry */
  void AttachTextEntryControl();
  
  /** Attach an IControl to the graphics context and add it to the top of the control stack. The control is owned by the graphics context and will be deleted when the context is deleted.
   * @param pControl A pointer to an IControl to attach.
   * @param controlTag An integer tag that you can use to identify the control
   * @param group A CString that you can use to address controlled by group
   * @return The index of the control (and the number of controls in the stack) */
  int AttachControl(IControl* pControl, int controlTag = kNoTag, const char* group = "");

  /** @param idx The index of the control to get
   * @return A pointer to the IControl object at idx or nullptr if not found */
  IControl* GetControl(int idx) { return mControls.Get(idx); }

  /** @param controlTag The tag to look for
   * @return A pointer to the IControl object with the tag of nullptr if not found */
  IControl* GetControlWithTag(int controlTag);
  
  /** Get a pointer to the IControl that is currently captured i.e. during dragging
   * @return Pointer to currently captured control */
  IControl* GetCapturedControl() { return mMouseCapture; }

  /* Get the first control in the control list, the background */
  IControl* GetBackgroundControl() { return GetControl(0);  }
  
  /** @return The number of controls that have been added to this graphics context */
  int NControls() const { return mControls.GetSize(); }

  /** Remove controls from the control list above a particular index, (frees memory).  */
  void RemoveControls(int fromIdx);
  
  /** Removes all regular IControls from the control list, as well as special controls (frees memory). */
  void RemoveAllControls();
  
  /** @param paramIdx <#paramIdx>
   * @param hide <#hide> */
  void HideControl(int paramIdx, bool hide);

  /** @param paramIdx <#paramIdx>
   * @param gray <#gray>
  */
  void GrayOutControl(int paramIdx, bool gray);

  /** @param paramIdx <#paramIdx>
   * @param lo <#lo>
   * @param hi <#hi>
   * @param normalized <#normalized>*/
  void ClampControl(int paramIdx, double lo, double hi, bool normalized);

  /***/
  void ForControlWithParam(int paramIdx, std::function<void(IControl& control)> func);

  /***/
  void ForControlInGroup(const char* group, std::function<void(IControl& control)> func);

  /***/
  void SetAllControlsDirty();
  
  /***/
  void SetAllControlsClean();
  
  /** @param x The X coordinate in the graphics context at which the mouse event occurred
   * @param y The Y coordinate in the graphics context at which the mouse event occurred
   * @param mod IMouseMod struct contain information about the modifiers held */
  void OnMouseDown(float x, float y, const IMouseMod& mod);

  /** @param x The X coordinate in the graphics context at which the mouse event occurred
   * @param y The Y coordinate in the graphics context at which the mouse event occurred
   * @param mod IMouseMod struct contain information about the modifiers held */
  void OnMouseUp(float x, float y, const IMouseMod& mod);

  /** @param x The X coordinate in the graphics context at which the mouse event occurred
   * @param y The Y coordinate in the graphics context at which the mouse event occurred
   * @param dX Delta X value \todo explain
   * @param dY Delta Y value \todo explain
   * @param mod IMouseMod struct contain information about the modifiers held */
  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod);

  /** @param x The X coordinate in the graphics context at which the mouse event occurred
   * @param y The Y coordinate in the graphics context at which the mouse event occurred
   * @param mod IMouseMod struct contain information about the modifiers held
   * @return <#return value> */
  bool OnMouseDblClick(float x, float y, const IMouseMod& mod);

  /** @param x The X coordinate in the graphics context at which the mouse event occurred
   * @param y The Y coordinate in the graphics context at which the mouse event occurred
   * @param mod IMouseMod struct contain information about the modifiers held
   * @param delta Delta value \todo explain */
  void OnMouseWheel(float x, float y, const IMouseMod& mod, float delta);

  /** @param x The X coordinate in the graphics context of the mouse cursor at the time of the key press
   * @param y The Y coordinate in the graphics context of the mouse cursor at the time of the key press
   * @param \todo
   * @return \c true if handled \todo check this */
  bool OnKeyDown(float x, float y, const IKeyPress& key);

  /** @param x The X coordinate in the graphics context at which to draw
   * @param y The Y coordinate in the graphics context at which to draw
   * @param mod IMouseMod struct contain information about the modifiers held
   * @return \c true if handled \todo check this */
  bool OnMouseOver(float x, float y, const IMouseMod& mod);

  /** TODO: not called on mac*/
  void OnMouseOut();

  /** @param str A CString with the absolute path of the dropped item
   * @param x The X coordinate in the graphics context where the drag and drop occurred
   * @param y The Y coordinate in the graphics context where the drag and drop occurred */
  void OnDrop(const char* str, float x, float y);

  /***/
  void OnGUIIdle();
  
  /***/
  void OnResizeGesture(float x, float y);

  /** @param enable Set \c true if you want to handle mouse over messages. Note: this may increase the amount CPU usage if you redraw on mouse overs etc */
  void HandleMouseOver(bool canHandle) { mHandleMouseOver = canHandle; }

  /** Used to tell the graphics context to stop tracking mouse interaction with a control \todo internal only? */
  void ReleaseMouseCapture();

  /** @param enable Set \c true to enable tool tips when the user mouses over a control */
  void EnableTooltips(bool enable);

  /** Call this method in order to create tool tips for every IControl that show the associated parameter's name */
  void AssignParamNameToolTips();

  /** @param enable Set \c true if you wish to draw the rectangular region of the graphics context occupied by each IControl in mControls  */
  inline void ShowControlBounds(bool enable) { mShowControlBounds = enable; SetAllControlsDirty(); }

  /** @param enable Set \c true if you wish to show the rectangular region that is drawn on each frame, in order to debug redraw problems */
  inline void ShowAreaDrawn(bool enable) { mShowAreaDrawn = enable; if(!enable) SetAllControlsDirty(); }
  
  /**@return \c true if showning the area drawn on each frame */
  bool ShowAreaDrawnEnabled() const { return mShowAreaDrawn; }

  /**@return \c true if showning the control bounds */
  bool ShowControlBoundsEnabled() const { return mShowControlBounds; }
  
  /** Live edit mode allows you to relocate controls at runtime in debug builds and save the locations to a predefined file (e.g. main plugin .cpp file) \todo we need a separate page for liveedit info
   * @param enable Set \c true if you wish to enable live editing mode
   * @param file The absolute path of the file which contains the layout info (correctly tagged) for live editing
   * @param gridsize The size of the layout grid in pixels */
  void EnableLiveEdit(bool enable/*, const char* file = 0, int gridsize = 10*/);
  
  /**@return \c true if live edit mode is enabled */
  bool LiveEditEnabled() const { return mLiveEdit != nullptr; }

  /** Returns an IRECT that represents the entire UI bounds
   * This is useful for programatically arranging UI elements by slicing up the IRECT using the various IRECT methods
   * @return An IRECT that corresponds to the entire UI area, with, L = 0, T = 0, R = Width() and B  = Height() */
  IRECT GetBounds() const { return IRECT(0.f, 0.f, (float) Width(), (float) Height()); }

  /** @return \c true if the context can handle mouse overs */
  bool CanHandleMouseOver() const { return mHandleMouseOver; }

  /** @return An integer representing the control index in IGraphics::mControls which the mouse is over, or -1 if it is not */
  inline int GetMouseOver() const { return mMouseOverIdx; }

  /** Get the x, y position in the graphics context of the last mouse down message. Does not get cleared on mouse up etc.
   * @param x Where the X position will be stored
   * @param float&y Where the Y position will be stored */
  void GetMouseDownPoint(float& x, float&y) const { x = mMouseDownX; y = mMouseDownY; }
  
  /** @return Get a persistant IPopupMenu (remember to clear it before use) */
  IPopupMenu& GetPromptMenu() { return mPromptPopupMenu; }
  
  /** @return \c true if tool tips are enabled */
  inline bool TooltipsEnabled() const { return mEnableTooltips; }

  /**  Set by the platform class if the mouse input is coming from a tablet/stylus
   * @param tablet, \c true means input is from a tablet */
  void SetTabletInput(bool tablet) { mTabletInput = tablet; }
  
  EUIResizerMode GetResizerMode() const { return mGUISizeMode; }
  
  IPopupMenuControl* GetPopupMenuControl() { return mPopupControl; }
  ITextEntryControl* GetTextEntryControl() { return mTextEntryControl; }
  
  void StyleAllVectorControls(bool drawFrame, bool drawShadow, bool emboss, float roundness, float frameThickness, float shadowOffset, const IVColorSpec& spec = DEFAULT_SPEC);
#pragma mark - Plug-in API Specific

  /** [AAX only] This can be called by the ProTools API class (e.g. IPlugAAX) in order to ascertain the parameter linked to the control under the mouse.
   * The purpose is to facillitate ProTool's special contextual menus (for configuring parameter automation)
   * @param x The X coordinate in the graphics context to check
   * @param y The Y coordinate in the graphics contextto check
   * @return An integer representing the parameter index that was found (or -1 if not found) */
  int GetParamIdxForPTAutomation(float x, float y);

  /** [AAX only]
   * @return An integer representing the last clicked parameter index (or -1 if none) */
  int GetLastClickedParamForPTAutomation();

  /** [AAX only] \todo
   * @param paramIdx The index of the parameter to highlight
   * @param isHighlighted /c true if the parameter should be highlighted
   * @param color An integer corresponding to AAX_EParameterHighlight \todo check Enum name */
  void SetPTParameterHighlight(int paramIdx, bool isHighlighted, int color);

  /** [VST3 primarily]
   * @param controlIdx <#controlIdx>
   * @param x The X coordinate in the graphics context at which to popup the context menu
   * @param y The Y coordinate in the graphics context at which to popup the context menu */
  void PopupHostContextMenuForParam(int controlIdx, int paramIdx, float x, float y);

#pragma mark - Resource/File Loading
  /** Load a bitmap image from disk or from windows resource
   * @param fileNameOrResID CString file name or resource ID
   * @param nStates The number of states/frames in a multi-frame stacked bitmap
   * @param framesAreHorizontal Set \c true if the frames in a bitmap are stacked horizontally
   * @param targetScale Set \c to a number > 0 to explicity load e.g. an @2x.png
   * @return An IBitmap representing the image */
  virtual IBitmap LoadBitmap(const char* fileNameOrResID, int nStates = 1, bool framesAreHorizontal = false, int targetScale = 0);

  /** Load an SVG from disk or from windows resource
   * @param fileNameOrResID A CString absolute path or resource ID
   * @return An ISVG representing the image */
  virtual ISVG LoadSVG(const char* fileNameOrResID, const char* units = "px", float dpi = 72.f);

  /** @param fileNameOrResID A CString absolute path or resource ID
   * @return \c true on success */
  virtual bool LoadFont(const char* fileNameOrResID) { return false; }
  
protected:
  virtual void CreatePlatformTextEntry(IControl& control, const IText& text, const IRECT& bounds, const char* str = "") = 0;
  virtual IPopupMenu* CreatePlatformPopupMenu(IPopupMenu& menu, const IRECT& bounds, IControl* pCaller = nullptr) = 0;

  typedef WDL_TypedBuf<unsigned char> RawBitmapData;

  virtual APIBitmap* LoadAPIBitmap(const char* fileNameOrResID, int scale, EResourceLocation location, const char* ext) = 0;
  virtual APIBitmap* ScaleAPIBitmap(const APIBitmap* pBitmap, int scale) = 0;
  virtual APIBitmap* CreateAPIBitmap(int width, int height) = 0;
    
  virtual int AlphaChannel() const = 0;
  virtual bool FlippedBitmap() const = 0;

  virtual void GetLayerBitmapData(const ILayerPtr& layer, RawBitmapData& data) = 0;
  virtual void ApplyShadowMask(ILayerPtr& layer, RawBitmapData& mask, const IShadow& shadow) = 0;

  void PushLayer(ILayer* layer, bool clearTransforms);
  ILayer* PopLayer(bool clearTransforms);
    
  /** Utility used by SearchImageResource/SearchBitmapInCache */
  inline void SearchNextScale(int& sourceScale, int targetScale);

  /** Search for a bitmap image resource matching the target scale */
  EResourceLocation SearchImageResource(const char* fileName, const char* type, WDL_String& result, int targetScale, int& sourceScale);

  /** Search the static storage cache for a bitmap image resource matching the target scale */
  APIBitmap* SearchBitmapInCache(const char* fileName, int targetScale, int& sourceScale);

  virtual bool DoDrawMeasureText(const IText& text, const char* str, IRECT& bounds, const IBlend* pBlend = nullptr, bool measure = false) = 0;
    
  virtual float GetBackingPixelScale() const = 0;

  void ForStandardControlsFunc(std::function<void(IControl& control)> func);
  
  template<typename T, typename... Args>
  void ForMatchingControls(T method, int paramIdx, Args... args);
  
  IGEditorDelegate& mDelegate;
  void* mPlatformContext = nullptr;
  bool mCursorHidden = false;
  bool mCursorLock = false;
  bool mTabletInput = false;
  float mCursorX = -1.f;
  float mCursorY = -1.f;

private:
  virtual void PlatformResize() {}
  virtual void DrawResize() {}
  
  void Draw(const IRECT& bounds, float scale);
  void DrawControl(IControl* pControl, const IRECT& bounds, float scale);
  
  int GetMouseControlIdx(float x, float y, bool mouseOver = false);
  IControl* GetMouseControl(float x, float y, bool capture, bool mouseOver = false);
  
  void StartResizeGesture() { mResizingInProcess = true; };
  
  void PopupHostContextMenuForParam(IControl* pControl, int paramIdx, float x, float y);

  void ForAllControlsFunc(std::function<void(IControl& control)> func);
    
  template<typename T, typename... Args>
  void ForAllControls(T op, Args... args);
  
  WDL_PtrList<IControl> mControls;

  // Order (front-to-back) ToolTip / PopUp / TextEntry / LiveEdit / Corner / PerfDisplay
  
  ICornerResizerControl* mCornerResizer = nullptr;
  IPopupMenuControl* mPopupControl = nullptr;
  IFPSDisplayControl* mPerfDisplay = nullptr;
  ITextEntryControl* mTextEntryControl = nullptr;
  IControl* mLiveEdit = nullptr;
  
  IPopupMenu mPromptPopupMenu;
  
  int mWidth;
  int mHeight;
  int mFPS;
  int mScreenScale = 1; // the scaling of the display that the UI is currently on e.g. 2 for retina
  float mDrawScale = 1.f; // scale deviation from  default width and height i.e stretching the UI by dragging bottom right hand corner
  int mIdleTicks = 0;
  IControl* mMouseCapture = nullptr;
  IControl* mMouseOver = nullptr;
  int mMouseOverIdx = -1;
  float mMouseDownX = -1.f;
  float mMouseDownY = -1.f;
  float mMinScale;
  float mMaxScale;
  int mMinWidth;
  int mMaxWidth;
  int mMinHeight;
  int mMaxHeight;
  int mLastClickedParam = kNoParameter;
  bool mHandleMouseOver = false;
  bool mStrict = true;
  bool mEnableTooltips = false;
  bool mShowControlBounds = false;
  bool mShowAreaDrawn = false;
  bool mResizingInProcess = false;
  bool mLayoutOnResize = false;
  EUIResizerMode mGUISizeMode = EUIResizerMode::kUIResizerScale;
  double mPrevTimestamp = 0.;
  std::function<bool(const IKeyPress& key)> mKeyHandlerFunc = nullptr;
protected:
  friend class IGraphicsLiveEdit;
  friend class ICornerResizerControl;
  
  std::stack<ILayer*> mLayers;
};

