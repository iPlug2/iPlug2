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
 * Utility structures and classes for IGraphics
 * @defgroup DrawClasses IGraphics::DrawClasses
 * The IGraphics draw classes allow the actual drawing to be performed using different drawing API back-ends.
 * A project-wide definition such as IGRAPHICS_SKIA, chooses which gets used at compile time
 * @defgroup PlatformClasses IGraphics::PlatformClasses
 * The IGraphics platform classes deal with event handling and platform specific contextual UI
 * @defgroup Controls IGraphics::IControls
 * UI Widgets, such as knobs, sliders, buttons
 * @defgroup BaseControls IGraphics::IControls::BaseControls
 * Base classes, to simplify making certain kinds of control
 * @defgroup SpecialControls IGraphics::IControls::SpecialControls
 * Special controls live outside the main stack, for implementing things like the corner resizer
 * @defgroup TestControls IGraphics::IControls::TestControls
 * The IGraphicsTest project includes lots of IControls to test functionality, which can also be used to understand how things work
 */

#ifndef NO_IGRAPHICS
#if defined(IGRAPHICS_NANOVG) + defined(IGRAPHICS_CANVAS) + defined(IGRAPHICS_SKIA) != 1
#error Either NO_IGRAPHICS or one and only one choice of graphics library must be defined!
#endif
#endif

#ifdef AAX_API
#include "IPlugAAX_view_interface.h"
#endif

#include "IPlugConstants.h"
#include "IPlugLogger.h"
#include "IPlugPaths.h"

#include "IGraphicsConstants.h"
#include "IGraphicsStructs.h"
#include "IGraphicsPopupMenu.h"
#include "IGraphicsEditorDelegate.h"

#ifdef IGRAPHICS_IMGUI
#include "IGraphicsImGui.h"
#endif

#include "nanosvg.h"

#include <stack>
#include <memory>
#include <vector>
#include <unordered_map>

#ifdef FillRect
#undef FillRect
#endif

#ifdef DrawText
#undef DrawText
#endif

#ifdef LoadBitmap
#undef LoadBitmap
#endif

BEGIN_IPLUG_NAMESPACE
class IParam;
BEGIN_IGRAPHICS_NAMESPACE
class IControl;
class IPopupMenuControl;
class ITextEntryControl;
class ICornerResizerControl;
class IFPSDisplayControl;
class IBubbleControl;

/**  The lowest level base class of an IGraphics context */
class IGraphics
#ifdef AAX_API
: public IPlugAAXView_Interface
#endif
{
public:
#pragma mark - Drawing API implementation

  /** Called at the beginning of drawing. Call base implementation if overridden. */
  virtual void BeginFrame();
  
  /** Called after platform view initialization, so that drawing classes can e.g. create an OpenGL context. */
  virtual void OnViewInitialized(void* pContext) {};
  
  /** Called after a platform view is destroyed, so that drawing classes can e.g. free any resources */
  virtual void OnViewDestroyed() {};

  /** Called by some drawing API classes to finally blit the draw bitmap onto the screen or perform other cleanup after drawing */
  virtual void EndFrame() {};

  /** Draw an SVG image to the graphics context
   * @param svg The SVG image to the graphics context
   * @param bounds The rectangular region to draw the image in
   * @param pBlend Optional blend method */
  virtual void DrawSVG(const ISVG& svg, const IRECT& bounds, const IBlend* pBlend = 0);

  /** Draw an SVG image to the graphics context with rotation
   * @param svg The SVG image to draw to the graphics context
   * @param destCentreX The X coordinate of the centre point at which to rotate the image around
   * @param destCentreY The Y coordinate of the centre point at which to rotate the image around
   * @param width The width of the drawn SVG
   * @param height The heigh of the drawn SVG
   * @param angle The angle to rotate the SVG mask at in degrees clockwise
   * @param pBlend Optional blend method */
  virtual void DrawRotatedSVG(const ISVG& svg, float destCentreX, float destCentreY, float width, float height, double angle, const IBlend* pBlend = 0);

  /** Draw a bitmap (raster) image to the graphics context
   * @param bitmap The bitmap image to draw to the graphics context
   * @param bounds The rectangular region to draw the image in
   * @param srcX The X offset in the source image to draw from
   * @param srcY The Y offset in the source image to draw from
   * @param pBlend Optional blend method */
  virtual void DrawBitmap(const IBitmap& bitmap, const IRECT& bounds, int srcX, int srcY, const IBlend* pBlend = 0) = 0;

  /** Draw a bitmap (raster) image to the graphics context, scaling the image to fit the bounds
   * @param bitmap The bitmap image to draw to the graphics context
   * @param bounds The rectangular region to draw the image in
   * @param pBlend Optional blend method */
  virtual void DrawFittedBitmap(const IBitmap& bitmap, const IRECT& bounds, const IBlend* pBlend = 0);
  
  /** Draw a bitmap (raster) image to the graphics context with rotation
   * @param bitmap The bitmap image to draw to the graphics context
   * @param destCentreX The X of the centre point at which to rotate the image around
   * @param destCentreY The Y of the centre point at which to rotate the image around
   * @param angle The angle of rotation in degrees clockwise
   * @param pBlend Optional blend method */
  virtual void DrawRotatedBitmap(const IBitmap& bitmap, float destCentreX, float destCentreY, double angle, const IBlend* pBlend = 0);

  /** Fill a rectangle corresponding to a pixel on a 1:1 screen with a color
   * @param color The color to fill the point with
   * @param x The X coordinate at which to draw
   * @param y The Y coordinate at which to draw
   * @param pBlend Optional blend method */
  virtual void DrawPoint(const IColor& color, float x, float y, const IBlend* pBlend = 0);

  /** Draw a line to the graphics context
   * @param color The color to draw the line with
   * @param x1 The X coordinate of the start of the line
   * @param y1 The Y coordinate of the start of the line
   * @param x2 The X coordinate of the end of the line
   * @param y2 The Y coordinate of the end of the line
   * @param pBlend Optional blend method
   * @param thickness Optional line thickness */
  virtual void DrawLine(const IColor& color, float x1, float y1, float x2, float y2, const IBlend* pBlend = 0, float thickness = 1.f);

  /** Draw a dotted line to the graphics context
   * @param color The color to draw the shape with
   * @param x1 The X coordinate of the start of the line
   * @param y1 The Y coordinate of the start of the line
   * @param x2 The X coordinate of the end of the line
   * @param y2 The Y coordinate of the end of the line
   * @param pBlend Optional blend method
   * @param thickness Optional line thickness */
  virtual void DrawDottedLine(const IColor& color, float x1, float y1, float x2, float y2, const IBlend* pBlend = 0, float thickness = 1.f, float dashLen = 2.f);
  
  /** Draw a triangle to the graphics context
   * @param color The color to draw the shape with
   * @param x1 The X coordinate of the first vertex
   * @param y1 The Y coordinate of the first vertex
   * @param x2 The X coordinate of the second vertex
   * @param y2 The Y coordinate of the second vertex
   * @param x3 The X coordinate of the third vertex
   * @param y3 The Y coordinate of the third vertex
   * @param pBlend Optional blend method
   * @param thickness Optional line thickness */
  virtual void DrawTriangle(const IColor& color, float x1, float y1, float x2, float y2, float x3, float y3, const IBlend* pBlend = 0, float thickness = 1.f);

  /** Draw a rectangle to the graphics context
   * @param bounds The rectangular area in which to draw the shape
   * @param color The color to draw the shape to draw the shape with
   * @param pBlend Optional blend method
   * @param thickness Optional line thickness */
  virtual void DrawRect(const IColor& color, const IRECT& bounds, const IBlend* pBlend = 0, float thickness = 1.f);

  /** Draw a rounded rectangle to the graphics context
   * @param color The color to draw the shape with
   * @param bounds The rectangular region to draw the shape in
   * @param cornerRadius The corner radius in pixels
   * @param pBlend Optional blend method
   * @param thickness Optional line thickness */
  virtual void DrawRoundRect(const IColor& color, const IRECT& bounds, float cornerRadius = 5.f, const IBlend* pBlend = 0, float thickness = 1.f);

  /** Draw a rounded rectangle to the graphics context with individual corner roundness
   * @param color The color to draw the shape with
   * @param bounds The rectangular region to draw the shape in
   * @param cRTL The top left corner radius in pixels
   * @param cRTR The top right corner radius in pixels
   * @param cRBR The bottom right corner radius in pixels
   * @param cRBL The bottom left corner radius in pixels
   * @param pBlend Optional blend method
   * @param thickness Optional line thickness */
  virtual void DrawRoundRect(const IColor& color, const IRECT& bounds, float cRTL, float cRTR, float cRBR, float cRBL, const IBlend* pBlend = 0, float thickness = 1.f);
  
  /** Draw an arc to the graphics context
   * @param color The color to draw the shape with
   * @param cx The X coordinate of the centre of the circle on which the arc lies
   * @param cy The Y coordinate of the centre of the circle on which the arc lies
   * @param r The radius of the circle on which the arc lies
   * @param a1 the start angle of the arc at in degrees clockwise where 0 is up
   * @param a2 the end angle of the arc at in degrees clockwise where 0 is up
   * @param pBlend Optional blend method
   * @param thickness Optional line thickness */
  virtual void DrawArc(const IColor& color, float cx, float cy, float r, float a1, float a2, const IBlend* pBlend = 0, float thickness = 1.f);

  /** Draw a circle to the graphics context
   * @param color The color to draw the shape with
   * @param cx The X coordinate of the centre of the circle
   * @param cy The Y coordinate of the centre of the circle
   * @param r The radius of the circle
   * @param pBlend Optional blend method
   * @param thickness Optional line thickness */
  virtual void DrawCircle(const IColor& color, float cx, float cy, float r, const IBlend* pBlend = 0, float thickness = 1.f);
  
  /** Draw an ellipse within a rectangular region of the graphics context
   * @param color The color to draw the shape with
   * @param bounds The rectangular region to draw the shape in
   * @param pBlend Optional blend method
   * @param thickness Optional line thickness */
  virtual void DrawEllipse(const IColor& color, const IRECT& bounds, const IBlend* pBlend = 0, float thickness = 1.f);
  
  /** Draw an ellipse around a central point given two radii and an angle of orientation
   * @param color The color to draw the shape with
   * @param x The X coordinate of the centre of the ellipse
   * @param y The Y coordinate of the centre of the ellipse
   * @param r1 The radius of the ellipse along the line found by rotating the x-axis by the angle
   * @param r2 The radius of the ellipse along the line found by rotating the y-axis by the angle
   * @param angle The angle rotates the radii r1 and r2 clockwise in degrees to adjust the orientation
   * @param pBlend Optional blend method
   * @param thickness Optional line thickness */
  virtual void DrawEllipse(const IColor& color, float x, float y, float r1, float r2, float angle = 0.0, const IBlend* pBlend = 0, float thickness = 1.f);

  /** Draw a convex polygon to the graphics context
   * @param color The color to draw the shape with
   * @param x Pointer to the first element in an array of X coordinates for the vertices of the polygon
   * @param y Pointer to the first element in an array of Y coordinates for the vertices of the polygon
   * @param nPoints The number of points in the coordinate arrays
   * @param pBlend Optional blend method
   * @param thickness Optional line thickness */
  virtual void DrawConvexPolygon(const IColor& color, float* x, float* y, int nPoints, const IBlend* pBlend = 0, float thickness = 1.f);

  /** Draw a dotted rectangle to the graphics context
   * @param color The color to draw the shape with
   * @param bounds The rectangular region to draw the shape in
   * @param pBlend Optional blend method
   * @param thickness Optional line thickness */
  virtual void DrawDottedRect(const IColor& color, const IRECT& bounds, const IBlend* pBlend = 0, float thickness = 1.f, float dashLen = 2.f);

  /** Fill a triangle with a color
   * @param color The color to fill the shape with
   * @param x1 The X coordinate of the first vertex
   * @param y1 The Y coordinate of the first vertex
   * @param x2 The X coordinate of the second vertex
   * @param y2 The Y coordinate of the second vertex
   * @param x3 The X coordinate of the third vertex
   * @param y3 The Y coordinate of the third vertex
   * @param pBlend Optional blend method */
  virtual void FillTriangle(const IColor& color, float x1, float y1, float x2, float y2, float x3, float y3, const IBlend* pBlend = 0);

  /** Fill a rectangular region of the graphics context with a color
   * @param color The color to fill the shape with
   * @param bounds The rectangular region to fill the shape in
   * @param pBlend Optional blend method */
  virtual void FillRect(const IColor& color, const IRECT& bounds, const IBlend* pBlend = 0);

  /** Fill a rounded rectangle with a color
   * @param color The color to fill the shape with
   * @param bounds The rectangular region to fill the shape in
   * @param cornerRadius The corner radius in pixels
   * @param pBlend Optional blend method */
  virtual void FillRoundRect(const IColor& color, const IRECT& bounds, float cornerRadius = 5.f, const IBlend* pBlend = 0);

  /** Fill a rounded rectangle with a color
   * @param color The color to fill the shape with
   * @param bounds The rectangular region to fill the shape in
   * @param cRTL The top left corner radius in pixels
   * @param cRTR The top right corner radius in pixels
   * @param cRBR The bottom right corner radius in pixels
   * @param cRBL The bottom left corner radius in pixels
   * @param pBlend Optional blend method */
  virtual void FillRoundRect(const IColor& color, const IRECT& bounds, float cRTL, float cRTR, float cRBR, float cRBL, const IBlend* pBlend = 0);
  
  /** Fill a circle with a color
   * @param color The color to fill the shape with
   * @param cx The X coordinate of the centre of the circle
   * @param cy The Y coordinate of the centre of the circle
   * @param r The radius of the circle
   * @param pBlend Optional blend method */
  virtual void FillCircle(const IColor& color, float cx, float cy, float r, const IBlend* pBlend = 0);

  /** Fill an ellipse within a rectangular region of the graphics context
   * @param color The color to fill the shape with
   * @param bounds The rectangular region to fill the shape in
   * @param pBlend Optional blend method */
  virtual void FillEllipse(const IColor& color, const IRECT& bounds, const IBlend* pBlend = 0);
  
  /** Fill an ellipse
   * @param color The color to draw the shape with
   * @param x The X coordinate of the centre of the ellipse
   * @param y The Y coordinate of the centre of the ellipse
   * @param r1 The radius of the ellipse along the line found by rotating the x-axis by the angle
   * @param r2 The radius of the ellipse along the line found by rotating the y-axis by the angle
   * @param angle The angle rotates the radii r1 and r2 clockwise in degrees to adjust the orientation
   * @param pBlend Optional blend method */
  virtual void FillEllipse(const IColor& color, float x, float y, float r1, float r2, float angle = 0.0, const IBlend* pBlend = 0);
  
  /** Fill an arc segment with a color
   * @param color The color to fill the shape with
   * @param cx The X coordinate of the centre of the circle on which the arc lies
   * @param cy The Y coordinate of the centre of the circle on which the arc lies
   * @param r The radius of the circle on which the arc lies
   * @param a1 the start angle  of the arc at in degrees clockwise where 0 is up
   * @param a2 the end angle  of the arc at in degrees clockwise where 0 is up
   * @param pBlend Optional blend method */
  virtual void FillArc(const IColor& color, float cx, float cy, float r, float a1, float a2, const IBlend* pBlend = 0);

  /** Fill a convex polygon with a color
   * @param color The color to fill the shape with
   * @param x Pointer to the first element in an array of X coordinates for the vertices of the polygon
   * @param y Pointer to the first element in an array of Y coordinates for the vertices of the polygon
   * @param nPoints The number of points in the coordinate arrays
   * @param pBlend Optional blend method */
  virtual void FillConvexPolygon(const IColor& color, float* x, float* y, int nPoints, const IBlend* pBlend = 0);

  /** Draw some text to the graphics context in a specific rectangle
   * @param text An IText struct containing font and text properties and layout info
   * @param str The text string to draw
   * @param bounds The rectangular region in the graphics where you would like to draw the text */
  void DrawText(const IText& text, const char* str, const IRECT& bounds, const IBlend* pBlend = 0);

  /** Draw some text to the graphics context at a point
   * @param text An IText struct containing font and text properties and layout info
   * @param str The text string to draw
   * @param x The x position in the graphics where you would like to draw the text
   * @param y The y position in the graphics where you would like to draw the text */
  void DrawText(const IText& text, const char* str, float x, float y, const IBlend* pBlend = 0);
  
  /** Measure the rectangular region that some text will occupy
   * @param text An IText struct containing font and text properties and layout info
   * @param str The text string to draw
   * @param bounds after calling the method this IRECT will be updated with the rectangular region the text will occupy */
  virtual float MeasureText(const IText& text, const char* str, IRECT& bounds) const;

  /** Get the color of a point. On a 1:1 screen this corresponds to a pixel. \todo check this
   * @param x The X coordinate of the pixel
   * @param y The Y coordinate of the pixel
   * @return An IColor specifiying the color of the pixel at x,y */
  virtual IColor GetPoint(int x, int y)  = 0;

  /** Gets a void pointer to underlying drawing context, for the IGraphics backend
   * See draw class implementation headers (e.g. IGraphicsNanoVG.h) for what you can cast the void pointer to */
  virtual void* GetDrawContext() = 0;

  /** @return A CString representing the Drawing API in use e.g. "NanoVG" */
  virtual const char* GetDrawingAPIStr() = 0;
  
  /** Returns a new IBitmap, an integer scaled version of the input, and adds it to the cache
   * @param inbitmap The source bitmap to be scaled
   * @param cacheName The name by which this bitmap is identified int the cache (along with targetScale)
   * @param targetScale An integer scale factor of the new bitmap
   * @return IBitmap The new IBitmap that has been added to the cache */
  virtual IBitmap ScaleBitmap(const IBitmap& inBitmap, const char* cacheName, int targetScale);

  /** Adds an IBitmap to the cache/static storage
   * @param bitmap The bitmap to cache
   * @param cacheName The name by which this bitmap is identified int the cache */
  virtual void RetainBitmap(const IBitmap& bitmap, const char* cacheName);

  /** Releases an IBitmap from the cache/static storage
   * @param bitmap The bitmap to release  */
  virtual void ReleaseBitmap(const IBitmap& bitmap);

  /** Get a version of the input bitmap from the cache that corresponds to the current screen scale
   * For example, when IControl::OnRescale() is called bitmap-based IControls can load in 
   * @param inBitmap The source bitmap to find a scaled version of
   * @return IBitmap The scaled bitmap */
  IBitmap GetScaledBitmap(IBitmap& inBitmap);
  
  /** Checks a file extension and reports whether this drawing API supports loading that extension */
  virtual bool BitmapExtSupported(const char* ext) = 0;
  
  /** NanoVG only */
  virtual void DrawFastDropShadow(const IRECT& innerBounds, const IRECT& outerBounds, float xyDrop = 5.f, float roundness = 0.f, float blur = 10.f, IBlend* pBlend = nullptr) { /* NO-OP*/ }
  
#pragma mark - Base implementation - drawing helpers

  /** Draws a bitmap into the graphics context. NOTE: this helper method handles multi-frame bitmaps, indexable via frame
   * @param bitmap - the bitmap to draw
   * @param bounds - where to draw the bitmap
   * @param frame - the frame index of the bitmap to draw (when bitmap is multi-frame)
   * @param pBlend - blend operation */
  void DrawBitmap(const IBitmap& bitmap, const IRECT& bounds, int frame = 1, const IBlend* pBlend = 0);

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
  void DrawBitmapedText(const IBitmap& bitmap, const IRECT& bounds, IText& text, IBlend* pBlend, const char* str, bool vCenter = true, bool multiline = false, int charWidth = 6, int charHeight = 12, int charOffset = 0);

  /** Draw a vertical line, within a rectangular region of the graphics context
   * @param color The color to draw the line with
   * @param bounds The rectangular region to draw the line in
   * @param x The normalized position of the line on the horizontal axis, within bounds
   * @param pBlend Optional blend method
   * @param thickness Optional line thickness */
  void DrawVerticalLine(const IColor& color, const IRECT& bounds, float x, const IBlend* pBlend = 0, float thickness = 1.f);

  /** Draw a horizontal line, within a rectangular region of the graphics context
   * @param color The color to draw the line with
   * @param bounds The rectangular region to draw the line in
   * @param y The normalized position of the line on the vertical axis, within bounds
   * @param pBlend Optional blend method
   * @param thickness Optional line thickness */
  void DrawHorizontalLine(const IColor& color, const IRECT& bounds, float y, const IBlend* pBlend = 0, float thickness = 1.f);

  /** Draw a clipped vertical line at a position bounds
   * @param color The color to draw the line with
   * @param xi The position of the line on the x axis
   * @param yLo The start of the vertical line on the y axis
   * @param yHi The end of the vertical line on the y axis
   * @param pBlend Optional blend method
   * @param thickness Optional line thickness */
  void DrawVerticalLine(const IColor& color, float xi, float yLo, float yHi, const IBlend* pBlend = 0, float thickness = 1.f);

  /** Draw a clipped horizontal line at a position bounds
   * @param color The color to draw the line with
   * @param yi The position of the line on the y axis
   * @param xLo The start of the horizontal line on the x axis
   * @param xHi The end of the horizontal line on the x axis
   * @param pBlend Optional blend method
   * @param thickness Optional line thickness */
  void DrawHorizontalLine(const IColor& color, float yi, float xLo, float xHi, const IBlend* pBlend = 0, float thickness = 1.f);

  /** Draw a radial line to the graphics context, useful for pointers on dials
   * @param color The color to draw the line with
   * @param cx centre point x coordinate
   * @param cy centre point y coordinate
   * @param angle The angle to draw at in degrees clockwise where 0 is up
   * @param rMin minima of the radial line (distance from cx,cy)
   * @param rMax maxima of the radial line (distance from cx,cy)
   * @param pBlend Optional blend method
   * @param thickness Optional line thickness */
  void DrawRadialLine(const IColor& color, float cx, float cy, float angle, float rMin, float rMax, const IBlend* pBlend = 0, float thickness = 1.f);

  /** Draw a grid to the graphics context
   * @param color The color to draw the grid lines with
   * @param bounds The rectangular region to fill the grid in
   * @param gridSizeH The width of the grid cells
   * @param gridSizeV The height of the grid cells
   * @param pBlend Optional blend method
   * @param thickness Optional line thickness */
  virtual void DrawGrid(const IColor& color, const IRECT& bounds, float gridSizeH, float gridSizeV, const IBlend* pBlend = 0, float thickness = 1.f);

  /** Draw a line between a collection of normalized points
   * @param color The color to draw the line with
   * @param bounds The rectangular region to draw the line in
   * @param normYPoints Ptr to float array - the normalized Y positions of the points
   * @param nPoints The number of points in the normYPoints / normXPoints
   * @param normXPoints Optional normailzed X positions of the points
   * @param pBlend Optional blend method
   * @param thickness Optional line thickness */
  virtual void DrawData(const IColor& color, const IRECT& bounds, float* normYPoints, int nPoints, float* normXPoints = nullptr, const IBlend* pBlend = 0, float thickness = 1.f);
  
  /** Load a font to be used by the graphics context
   * @param fontID A CString that will be used to reference the font
   * @param fileNameOrResID A CString absolute path or resource ID
   * @return \c true on success */
  virtual bool LoadFont(const char* fontID, const char* fileNameOrResID);

  /** Load a font from in-memory data to be used by the graphics context
   * @param fontID A CString that will be used to reference the font
   * @param pData Pointer to the font data in memory
   * @param dataSize Size (in bytes) of data at \c pData
   * @return \c true on success */
  virtual bool LoadFont(const char* fontID, void* pData, int dataSize);
    
  /** Load a font with a particular style (bold, italic) from a font file
   * @param fontID A CString that will be used to reference the font
   * @param fontName A CString font name
   * @param style A font style
   * @return \c true on success */
  bool LoadFont(const char* fontID, const char* fontName, ETextStyle style);

#pragma mark - Layer management
  
  /** Create an IGraphics layer. Switches drawing to an offscreen bitmap for drawing
   * IControl* pOwner The control that owns the layer
   * @param r The bounds of the layer within the IGraphics context
   * @param cacheable Used to make sure the underlying bitmap can be shared between plug-in instances */
  void StartLayer(IControl* pOwner, const IRECT& r, bool cacheable = false);
  
  /** If a layer already exists, continue drawing to it
   * @param layer the layer to resume */
  void ResumeLayer(ILayerPtr& layer);

  /** End an IGraphics layer. Switches drawing back to the main context
   * @return ILayerPtr a pointer to the layer, which should be kept around in order to draw it */
  ILayerPtr EndLayer();

  /** Test to see if a layer needs drawing, for instance if the control's bounds were changed
   * @param layer The layer to check
   * @return \c true if the layer needs to be updated */
  bool CheckLayer(const ILayerPtr& layer);

  /** Draw a layer to the main IGraphics context
   * @param layer The layer to draw
   * @param pBlend Optional blend method */
  void DrawLayer(const ILayerPtr& layer, const IBlend* pBlend = nullptr);

  /** Draw a layer to the main IGraphics context, fitting it to a rectangle that is different to the layer's bounds
   * @param layer The layer to draw
   * @param bounds The bounds in which to draw the layer
   * @param pBlend Optional blend method */
  void DrawFittedLayer(const ILayerPtr& layer, const IRECT& bounds, const IBlend* pBlend);

  /** Draw a layer to the main IGraphics context, with rotation
   * @param layer The layer to draw
   * @param angle The angle of rotation in degrees clockwise */
  void DrawRotatedLayer(const ILayerPtr& layer, double angle);
    
  /** Applies a drop shadow directly onto a layer
  * @param layer - the layer to add the shadow to 
  * @param shadow - the shadow to add */
  void ApplyLayerDropShadow(ILayerPtr& layer, const IShadow& shadow);

protected:
  /** Get the contents of a layers pixels as bitmap data
   * @param layer The layer to get the data from
   * @param data The pixel data extracted from the layer */
  virtual void GetLayerBitmapData(const ILayerPtr& layer, RawBitmapData& data) = 0;
  
  /** Implemented by a graphics backend to apply a calculated shadow mask to a layer, according to the shadow settings specified
   * @param layer The layer to apply the shadow to
   * @param mask The mask of the shadow as raw bitmap data
   * @param shadow The shadow specification */
  virtual void ApplyShadowMask(ILayerPtr& layer, RawBitmapData& mask, const IShadow& shadow) = 0;
  
  /** Implemented by a graphics backend to prepare for drawing to the layer at the top of the stack */
  virtual void UpdateLayer() {}

  /** Push a layer on to the stack.
   * @param pLayer The new layer */
  void PushLayer(ILayer* pLayer);
  
  /** Pop a layer off the stack.
   * @return ILayer* The layer that came off the stack */
  ILayer* PopLayer();
  
#pragma mark - Drawing API path support
public:
  /** Clear the stack of path drawing commands */
  virtual void PathClear() = 0;

  /** Close the path that is being specified. */
  virtual void PathClose() = 0;

  /** Add a line to the current path
   * @param x1 The X coordinate of the start of the line
   * @param y1 The Y coordinate of the start of the line
   * @param x2 The X coordinate of the end of the line
   * @param y2 The Y coordinate of the end of the line */
  void PathLine(float x1, float y1, float x2, float y2)
  {
    PathMoveTo(x1, y1);
    PathLineTo(x2, y2);
  }

  /** Add a radial line to the current path
   * @param cx centre point x coordinate
   * @param cy centre point y coordinate
   * @param angle The angle to draw at in degrees clockwise where 0 is up
   * @param rMin minima of the radial line (distance from cx,cy)
   * @param rMax maxima of the radial line (distance from cx,cy) */
  void PathRadialLine(float cx, float cy, float angle, float rMin, float rMax);

  /** Add a triangle to the current path
   * @param x1 The X coordinate of the first vertex
   * @param y1 The Y coordinate of the first vertex
   * @param x2 The X coordinate of the second vertex
   * @param y2 The Y coordinate of the second vertex
   * @param x3 The X coordinate of the third vertex
   * @param y3 The Y coordinate of the third vertex */
  void PathTriangle(float x1, float y1, float x2, float y2, float x3, float y3);

  /** Add a rectangle to the current path
   * @param bounds The bounds of the rectangle to add */
  void PathRect(const IRECT& bounds);

  /** Add a rounded rectangle to the current path, with independent corner roundness
   * @param bounds The rectangular region to draw the shape in
   * @param cRTL The top left corner radius in pixels
   * @param cRTR The top right corner radius in pixels
   * @param cRBR The bottom right corner radius in pixels
   * @param cRBL The bottom left corner radius in pixels */
  void PathRoundRect(const IRECT& bounds, float ctl, float ctr, float cbl, float cbr);

  /** Add a rounded rectangle to the current path
   * @param bounds The rectangular region to draw the shape in
   * @param cornerRadius The corner radius in pixels */
  void PathRoundRect(const IRECT& bounds, float cornerRadius = 5.f);

  /** Add an arc to the current path
   * @param cx The X coordinate of the centre of the circle on which the arc lies
   * @param cy The Y coordinate of the centre of the circle on which the arc lies
   * @param r The radius of the circle on which the arc lies
   * @param a1 the start angle of the arc at in degrees clockwise where 0 is up
   * @param a2 the end angle of the arc at in degrees clockwise where 0 is up */
  virtual void PathArc(float cx, float cy, float r, float a1, float a2, EWinding winding = EWinding::CW) = 0;

  /** Add a circle to the current path
   * @param cx The X coordinate of the centre of the circle
   * @param cy The Y coordinate of the centre of the circle
   * @param r The radius of the circle */
  void PathCircle(float cx, float cy, float r);

  /** Add an ellipse to the current path, specifying the rectangular region
   * @param bounds The rectangular region to draw the shape in */
   void PathEllipse(const IRECT& bounds);
  
  /** Add an ellipse to the current path
   * @param x The X coordinate of the centre of the ellipse
   * @param y The Y coordinate of the centre of the ellipse
   * @param r1 The radius of the ellipse along the line found by rotating the x-axis by the angle
   * @param r2 The radius of the ellipse along the line found by rotating the y-axis by the angle
   * @param angle The angle rotates the radii r1 and r2 clockwise in degrees to adjust the orientation */
  void PathEllipse(float x, float y, float r1, float r2, float angle = 0.0);

  /** Add a convex polygon to the current path
   * @param x Pointer to the first element in an array of X coordinates for the vertices of the polygon
   * @param y Pointer to the first element in an array of Y coordinates for the vertices of the polygon
   * @param nPoints The number of points in the coordinate arrays */
  void PathConvexPolygon(float* x, float* y, int nPoints);

  /** Move the current point in the current path
   * @param x The X coordinate
   * @param y The Y coordinate */
  virtual void PathMoveTo(float x, float y) = 0;

  /** Add a line to the current path from the current point to the specified location
   * @param x The X coordinate of the end of the line
   * @param y The Y coordinate of the end of the line */
  virtual void PathLineTo(float x, float y) = 0;

  /** NanoVG only. https://github.com/memononen/nanovg/blob/master/src/nanovg.h#L454
  * @param clockwise Should the path be wound clockwise */
  virtual void PathSetWinding(bool clockwise) {};

  /** Add a cubic bezier to the current path from the current point to the specified location
   * @param c1x Control point 1 X coordinate
   * @param c1y Control point 1 Y coordinate
   * @param c2x  Control point 2 X coordinate
   * @param c2y  Control point 2 Y coordinate
   * @param x2 The X coordinate of the end of the line
   * @param y2 The Y coordinate of the end of the line */
  virtual void PathCubicBezierTo(float c1x, float c1y, float c2x, float c2y, float x2, float y2) = 0;

  /** Add a quadratic bezier to the current path from the current point to the specified location
   * @param cx Control point X coordinate
   * @param cy Control point Y coordinate
   * @param x2 The X coordinate of the end of the line
   * @param y2 The Y coordinate of the end of the line */
  virtual void PathQuadraticBezierTo(float cx, float cy, float x2, float y2) = 0;
  
  /** Stroke the current current path
   * @param pattern The IPattern to use, for e.g. color or gradient
   * @param thickness The line thickness
   * @param options Optional IStrokeOptions to specify dash, join and path preserve options
   * @param pBlend Optional blend method */
  virtual void PathStroke(const IPattern& pattern, float thickness, const IStrokeOptions& options = IStrokeOptions(), const IBlend* pBlend = 0) = 0;

  /** Fill the current current path
   * @param pattern The IPattern to use, for e.g. color or gradient
   * @param options Optional IFillOptions to specify fill rule and preserve options
   * @param pBlend Optional blend method */
  virtual void PathFill(const IPattern& pattern, const IFillOptions& options = IFillOptions(), const IBlend* pBlend = 0) = 0;

  /** Save the current affine transform of the current path */
  void PathTransformSave();

  /** Restore the affine transform of the current path, to the previously saved state */
  void PathTransformRestore();

  /** Reset the affine transform of the current path, to the default state
   * @param clearStates Selects whether the call also empties the transform stack */
  void PathTransformReset(bool clearStates = false);

  /** Apply a translation transform to the current path
   * @param x Horizontal translation amount
   * @param y Vertical translation amount */
  void PathTransformTranslate(float x, float y);

  /** Apply a scale transform to the current path, with independant x, y scales
   * @param x Horizontal scale amount
   * @param y Horizontal scale amount */
  void PathTransformScale(float x, float y);

  /** Apply a scale transform to the current path, with independant x, y scales
   * @param scale Scale amount */
  void PathTransformScale(float scale);

  /** Apply a rotation transform to the current path
   * @param angle Angle to rotate in degrees clockwise */
  void PathTransformRotate(float angle);

  /** Apply a skew transform to the current path
   * @param xAngle Angle to skew horizontal in degrees clockwise
   * @param yAngle Angle to skew vertical in degrees clockwise */
  void PathTransformSkew(float xAngle, float yAngle);

  /** Apply an arbitary affine transform matrix to the current path
   * @param matrix The transfomation matrix */
  void PathTransformMatrix(const IMatrix& matrix);

  /** Clip the current path to a particular region
   * @param r The rectangular region to clip */
  void PathClipRegion(const IRECT r = IRECT());
  
  virtual void PathTransformSetMatrix(const IMatrix& matrix) = 0;

  void DoTextRotation(const IText& text, const IRECT& bounds, const IRECT& rect)
  {
    if (!text.mAngle)
      return;
    
    IRECT rotated = rect;
    double tx, ty;
    
    CalculateTextRotation(text, bounds, rotated, tx, ty);
    PathTransformTranslate(static_cast<float>(tx), static_cast<float>(ty));
    PathTransformRotate(text.mAngle);
  }
  
private:
  IPattern GetSVGPattern(const NSVGpaint& paint, float opacity);

  void DoDrawSVG(const ISVG& svg, const IBlend* pBlend = nullptr);
  
  /** Prepare a particular area of the display for drawing, normally resulting in clipping of the region.
   * @param bounds The rectangular region to prepare  */
  void PrepareRegion(const IRECT& bounds)
  {
    PathTransformReset(true);
    PathClear();
    SetClipRegion(bounds);
    mClipRECT = bounds;
  }

  /** Indicate that a particular area of the display has been drawn (for instance to transfer a temporary backing) Always called after a matching call to PrepareRegion.
  * @param bounds The rectangular region that is complete  */
  virtual void CompleteRegion(const IRECT& bounds) {}

  virtual void SetClipRegion(const IRECT& r) = 0;

public:
#pragma mark - Platform implementation
  
  /** Add an OS view as a sub-view, on top of the IGraphics view
   * @param r The bounds where the view should be attached
   * @param pView the platform view, which would be a HWND on Windows, NSView* on macOS or UIView* on iOS */
  virtual void AttachPlatformView(const IRECT& r, void* pView) {};
  
  /** Remove a previously attached platform view from the IGraphics view
   * @param pView the platform view to remove, which would be a HWND on Windows, NSView* on macOS or UIView* on iOS */
  virtual void RemovePlatformView(void* pView) {};

  /** Get the x, y position of the mouse cursor
   * @param x Where the X position will be stored
   * @param y Where the Y position will be stored */
  virtual void GetMouseLocation(float& x, float&y) const = 0;
  
  /** Call to hide/show the mouse cursor
   * @param hide Should the cursor be hidden or shown
   * @param lock Set \c true to hold the cursor in place while hidden */
  virtual void HideMouseCursor(bool hide = true, bool lock = true) = 0;

  /** Force move the mouse cursor to a specific position
   * @param x New X position in pixels
   * @param y New Y position in pixels */
  virtual void MoveMouseCursor(float x, float y) = 0;
  
  /** Sets the mouse cursor to one of ECursor (implementations should return the result of the base implementation)
   * @param cursorType The cursor type
   * @return The previous cursor type so it can be restored later */
  virtual ECursor SetMouseCursor(ECursor cursorType = ECursor::ARROW)
  {
    ECursor oldCursorType = mCursorType;
    mCursorType = cursorType;
    return oldCursorType;
  }

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

  /** @return /c true if the platform window/view is open */
  virtual bool WindowIsOpen() { return GetWindow(); }

  /** Get text from the clipboard
   * @param str A WDL_String that will be filled with the text that is currently on the clipboard
   * @return /c true on success */
  virtual bool GetTextFromClipboard(WDL_String& str) = 0;

  /** Set text in the clipboard
   * @param str A CString that will be used to set the current text in the clipboard
   * @return /c true on success */
  virtual bool SetTextInClipboard(const char* str) = 0;

  /** Call this if you modify control tool tips at runtime. \todo explain */
  virtual void UpdateTooltips() = 0;

  /** Pop up a modal platform message box dialog. NOTE: this method will block the main thread
   * @param str The text message to display in the dialogue
   * @param caption The title of the message box window \todo check
   * @param type EMsgBoxType describing the button options available \see EMsgBoxType
   * @return \todo check */
  virtual EMsgBoxResult ShowMessageBox(const char* str, const char* caption, EMsgBoxType type, IMsgBoxCompletionHanderFunc completionHandler = nullptr) = 0;

  /** Create a platform file prompt dialog to choose a file/directory path for opening/saving a file/directory. NOTE: this method will block the main thread
   * @param fileName Non const WDL_String reference specifying the file name. Set this prior to calling the method for save dialogs, to provide a default file name. For load dialogs, on successful selection of a file this will get set to the file’s name.
   * @param path WDL_String reference where the path will be put on success or empty string on failure/user cancelled
   * @param action Determines whether this is an open dialog or a save dialog
   * @param extensions A comma separated CString list of file extensions to filter in the dialog (e.g. “.wav, .aif” \todo check */
  virtual void PromptForFile(WDL_String& fileName, WDL_String& path, EFileAction action = EFileAction::Open, const char* extensions = 0) = 0;

  /** Create a platform file prompt dialog to choose a directory path for opening/saving a directory. NOTE: this method will block the main thread
   * @param dir Non const WDL_String reference specifying the directory path. Set this prior to calling the method for save dialogs, to provide a default path. For load dialogs, on successful selection of a directory this will get set to the full path. */
  virtual void PromptForDirectory(WDL_String& dir) = 0;

  /** Create a platform color chooser dialog. NOTE: this method will block the main thread
   * @param color When a color is chosen the IColor referenced will be updated with the new color
   * @param str The text to display in the dialog box e.g. "Please choose a color... (Windows only)"
   * @param IColorPickerHandlerFunc func callback for asynchronous color pickers
   * @return /c true if prompt completed successfully */
  virtual bool PromptForColor(IColor& color, const char* str = "", IColorPickerHandlerFunc func = nullptr) = 0;

  /** Open a URL in the platform’s default browser
   * @param url CString specifying the URL to open
   * @param msgWindowTitle \todo ?
   * @param confirmMsg \todo ?
   * @param errMsgOnFailure \todo ?
   * @return /c true on success */
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
  void SetPlatformContext(void* pContext) { mPlatformContext = pContext; }

  /** Get the platform level draw context - an HDC or CGContextRef
   * @return void pointer to an HDC or CGContext */
  void* GetPlatformContext() { return mPlatformContext; }
  
  /** Convert an x, y position in the view to screen coordinates
   * @param x the x position to convert
   * @param y the y position to convert */
  virtual void ClientToScreen(float& x, float& y) {};

  /** Load a font from disk or resource in a platform format.
   * @param fontID A string that is used to reference the font
   * @param fileNameOrResID A resource or file name/path
   * @return PlatformFontPtr from which the platform font can be retrieved */
  virtual PlatformFontPtr LoadPlatformFont(const char* fontID, const char* fileNameOrResID) = 0;

  /** Load a font from data in memory.
   * @param fontID A CString that is used to reference the font
   * @param pData Pointer to font data in memory
   * @param dataSize Size (in bytes) of data at \c pData
   * @return PlatformFontPtr from which the platform font may be retrieved */
  virtual PlatformFontPtr LoadPlatformFont(const char* fontID, void* pData, int dataSize) = 0;
  
  /** Load a system font in a platform format.
   * @param fontID  A string that is used to reference the font
   * @param fontName A string defining the font name
   * @param style An ETextStyle defining the font style
   * @return PlatformFontPtr from which the platform font can be retrieved */
  virtual PlatformFontPtr LoadPlatformFont(const char* fontID, const char* fontName, ETextStyle style) = 0;

  /** Called to indicate that the platform should cache data about the platform font if needed.
   * @param fontID  A string that is used to reference the font
   * @param font A const PlatformFontPtr reference to the relevant font */
  virtual void CachePlatformFont(const char* fontID, const PlatformFontPtr& font) = 0;

  /** Get the bundle ID on macOS and iOS, returns emtpy string on other OSs */
  virtual const char* GetBundleID() { return ""; }

protected:
  /* Implemented on Windows to store previously active GLContext and HDC for restoring, calls GetDC */
  virtual void ActivateGLContext() {}; 

  /* Implemented on Windows to restore previous GL context calls ReleaseDC */
  virtual void DeactivateGLContext() {};

  /** \todo
   * @param control \todo
   * @param text \todo
   * @param bounds \todo
   * @param str \todo */
  virtual void CreatePlatformTextEntry(int paramIdx, const IText& text, const IRECT& bounds, int length, const char* str) = 0;
  
  /** Calls the platform backend to create the platform popup menu
   * @param menu The source IPopupMenu
   * @param bounds \todo
   * @param isAsync This gets set true on platforms where popupmenu creation is asyncronous
   * @return A ptr to the chosen IPopupMenu or nullptr in the case of async or dismissed menu */
  virtual IPopupMenu* CreatePlatformPopupMenu(IPopupMenu& menu, const IRECT& bounds, bool& isAsync) = 0;

#pragma mark - Base implementation
public:
  IGraphics(IGEditorDelegate& dlg, int w, int h, int fps = 0, float scale = 1.);

  virtual ~IGraphics();
    
  IGraphics(const IGraphics&) = delete;
  IGraphics& operator=(const IGraphics&) = delete;
    
  /** Called by the platform IGraphics class when moving to a new screen to set DPI
   * @param scale The scale of the display, typically 2 on a macOS retina screen */
  void SetScreenScale(int scale);

  /** Called by some platform IGraphics classes in order to translate the graphics context, in response to e.g. iOS onscreen keyboard appearing */
  void SetTranslation(float x, float y) { mXTranslation = x; mYTranslation = y; }
  
  /** Called repeatedly at frame rate by the platform class to check what the graphics context says is dirty.
   * @param rects The rectangular regions which will be added to to mark what is dirty in the context
   * @return /c true if a control is dirty */
  bool IsDirty(IRECTList& rects);

  /** Called by the platform class indicating a number of rectangles in the UI that need to redraw
   * @param rects A set of rectangular regions to draw */
  void Draw(IRECTList& rects);

  /** Prompt for user input either using a text entry or pop up menu
   * @param control Reference to the control which the prompt relates to
   * @param bounds Rectangular region of the graphics context that the prompt (e.g. text entry box) should occupy
   * @param valIdx The value index for the control value that the prompt relates to */
  void PromptUserInput(IControl& control, const IRECT& bounds, int valIdx = 0);

  /** Shows a pop up/contextual menu in relation to a rectangular region of the graphics context
   * @param control A reference to the IControl creating this pop-up menu. If it exists IControl::OnPopupMenuSelection() will be called on successful selection
   * @param menu Reference to an IPopupMenu class populated with the items for the platform menu
   * @param bounds The platform menu will popup at the bottom left hand corner of this rectangular region
   * @param valIdx The value index for the control value that the menu relates to */
  void CreatePopupMenu(IControl& control, IPopupMenu& menu, const IRECT& bounds, int valIdx = 0);

  /** Shows a pop up/contextual menu at point
   * @param control A reference to the IControl creating this pop-up menu. If it exists IControl::OnPopupMenuSelection() will be called on successful selection
   * @param x The X coordinate at which to pop up the menu
   * @param y The Y coordinate at which to pop up the menu
   * @param valIdx The value index for the control value that the menu relates to */
  void CreatePopupMenu(IControl& control, IPopupMenu& menu, float x, float y, int valIdx = 0)
  {
    return CreatePopupMenu(control, menu, IRECT(x, y, x, y), valIdx);
  }
    
  /** Create a text entry box
   * @param control The control that the text entry belongs to. If this control is linked to a parameter, the text entry will be configured with initial text matching the parameter value
   * @param text An IText struct to set the formatting of the text entry box
   * @param bounds The rectangular region that the text entry will occupy.
   * @param str A CString to specify the default text to display when the text entry box is opened (unless the control specified by the first argument is linked to a parameter)
   * @param valIdx The value index for the control value that the text entry relates to */
  void CreateTextEntry(IControl& control, const IText& text, const IRECT& bounds, const char* str = "", int valIdx = 0);

   /** Called by the platform class after returning from a text entry in order to update a control with a new value. The base class has a record of the control, so it is not needed here.
    * @param str The new value as a CString */
  void SetControlValueAfterTextEdit(const char* str);
    
  /** Called by PopupMenuControl in order to update a control with a new value after returning from the non-blocking menu. The base class has a record of the control, so it is not needed here.
   * @param pMenu The menu that was clicked */
  void SetControlValueAfterPopupMenu(IPopupMenu* pMenu);
    
  /** \todo 
   * @param lo \todo
   * @param hi \todo */
  void SetScaleConstraints(float lo, float hi)
  {
    mMinScale = std::min(lo, hi);
    mMaxScale = std::max(lo, hi);
  }
  
  /** \todo detailed description of how this works
   * @param w New width in pixels
   * @param h New height in pixels
   * @param scale New scale ratio
   * @param needsPlatformResize This should be true for a "manual" resize from the plug-in UI and false
   * if being called from IEditorDelegate::OnParentWindowResize(), in order to avoid feedback */
  void Resize(int w, int h, float scale, bool needsPlatformResize = true);
  
  /** Enables strict drawing mode. \todo explain strict drawing
   * @param strict Set /c true to enable strict drawing mode */
  void SetStrictDrawing(bool strict);

  /* Enables layout on resize. This means IGEditorDelegate:LayoutUI() will be called when the GUI is resized */
  void SetLayoutOnResize(bool layoutOnResize);

  /** Gets the width of the graphics context
   * @return A whole number representing the width of the graphics context in pixels on a 1:1 screen */
  int Width() const { return mWidth; }

  /** Gets the height of the graphics context
   * @return A whole number representing the height of the graphics context in pixels on a 1:1 screen */
  int Height() const { return mHeight; }

  /** Gets the width of the graphics context including scaling (not display scaling!)
   * @return A whole number representing the width of the graphics context with scaling in pixels on a 1:1 screen */
  int WindowWidth() const { return static_cast<int>(static_cast<float>(mWidth) * mDrawScale); }

  /** Gets the height of the graphics context including scaling (not display scaling!)
   * @return A whole number representing the height of the graphics context with scaling in pixels on a 1:1 screen */
  int WindowHeight() const { return static_cast<int>(static_cast<float>(mHeight) * mDrawScale); }

  /** Gets the drawing frame rate
   * @return A whole number representing the desired frame rate at which the graphics context is redrawn. NOTE: the actual frame rate might be different */
  int FPS() const { return mFPS; }

  /** Gets the graphics context scaling factor.
   * @return The scaling applied to the graphics context */
  float GetDrawScale() const { return mDrawScale; }

  /** Gets the display scaling factor
    * @return The scale factor of the display on which this graphics context is currently located */
  int GetScreenScale() const { return mScreenScale; }

  /** Gets the combined screen and display scaling factor
  * @return The draw scale * screen scale */
  float GetTotalScale() const { return static_cast<float>(mDrawScale * static_cast<float>(mScreenScale)); }

  /** Gets the nearest backing pixel aligned rect to the input IRECT
    * @param r The IRECT to snap
    * @return The IRECT nearest to the input IRECT that is aligned exactly to backing pixels */
  IRECT GetPixelSnapped(IRECT &r) const { return r.GetPixelSnapped(GetBackingPixelScale()); }
    
  /** Gets a pointer to the delegate class that handles communication to and from this graphics context.
   * @return pointer to the delegate */
  IGEditorDelegate* GetDelegate() { return mDelegate; }

  /** @return Get a persistant IPopupMenu (remember to clear it before use) */
  IPopupMenu& GetPromptMenu() { return mPromptPopupMenu; }
  
  /** @return True if a platform text entry in is progress */
  bool IsInPlatformTextEntry() { return mInTextEntry != nullptr && !mTextEntryControl; }
  
  /** @return Ptr to the control that launched the text entry */
  IControl* GetControlInTextEntry() { return mInTextEntry; }
  
  /** @return \c true if tool tips are enabled */
  inline bool TooltipsEnabled() const { return mEnableTooltips; }
  
  /** @return An EUIResizerMode Representing whether the graphics context should scale or be resized, e.g. when dragging a corner resizer */
  EUIResizerMode GetResizerMode() const { return mGUISizeMode; }

  /** @return true if resizing is in process */
  bool GetResizingInProcess() const { return mResizingInProcess; }

  /** Enable/disable multi touch, if platform supports it
    * @return \c true if platform supports it */
  bool EnableMultiTouch(bool enable)
  {
    if (PlatformSupportsMultiTouch())
    {
      mEnableMultiTouch = enable;
      return true;
    }
    else
      mEnableMultiTouch = false;

    return false;
  }
  
  /** @return /c true if multi touch is enabled */
  bool MultiTouchEnabled() const { return mEnableMultiTouch; }

  /** @return /c true if the platform supports multi touch */
  virtual bool PlatformSupportsMultiTouch() const { return false; }
  
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
  
  /** Live edit mode allows you to relocate controls at runtime in debug builds
   * @param enable Set \c true if you wish to enable live editing mode */
  void EnableLiveEdit(bool enable);

  /**@return \c true if live edit mode is enabled */
  bool LiveEditEnabled() const { return mLiveEdit != nullptr; }
  
  /** Returns an IRECT that represents the entire UI bounds
   * This is useful for programatically arranging UI elements by slicing up the IRECT using the various IRECT methods
   * @return An IRECT that corresponds to the entire UI area, with, L = 0, T = 0, R = Width() and B  = Height() */
  IRECT GetBounds() const { return IRECT(0.f, 0.f, (float) Width(), (float) Height()); }

  /** Sets a function that is called at the frame rate, prior to checking for dirty controls 
 * @param func The function to call */
  void SetDisplayTickFunc(IDisplayTickFunc func) { mDisplayTickFunc = func; }

  /** Set a function that is called when key presses are not intercepted by any controls
   * @param keyHandlerFunc A std::function conforming to IKeyHandlerFunc  */
  void SetKeyHandlerFunc(IKeyHandlerFunc func) { mKeyHandlerFunc = func; }

  /** A helper to set the IGraphics KeyHandlerFunc in order to make an instrument playable via QWERTY keys
   * @param func A function to do something when a MIDI message is triggered */
  void SetQwertyMidiKeyHandlerFunc(std::function<void(const IMidiMsg& msg)> func = nullptr);
  
  /** Set functions to draw DearImGui widgets on top of the IGraphics context (only relevant when IGRAPHICS_IMGUI is defined) 
   * @param drawFunc Called at the framerate, where you do the main ImGui
   * @param setupFunc Called once after ImGui context is created */
  void AttachImGui(std::function<void(IGraphics*)> drawFunc, std::function<void()> setupFunc = nullptr);
  
  /** Called by platform class to see if the point at x, y is linked to a gesture recognizer */
  bool RespondsToGesture(float x, float y);
  
  /** Called by platform class when a gesture is recognized */
  void OnGestureRecognized(const IGestureInfo& info);

  /** Returns a scaling factor for resizing parent windows via the host/plugin API
   * @return A scaling factor for resizing parent windows */
  virtual int GetPlatformWindowScale() const { return 1; }

private:
  /* NO-OP to create ImGui when IGRAPHICS_IMGUI is defined */
  virtual void CreatePlatformImGui() {}
  
  /** \todo */
  virtual void PlatformResize(bool parentHasResized) {}
  
  /** \todo */
  virtual void DrawResize() {}
  
  /** Draw a region of the graphics (redrawing all contained items)
   * @param bounds \todo
   * @param scale \todo */
  void Draw(const IRECT& bounds, float scale);
  
  /** \todo
   * @param pControl \todo
   * @param bounds \todo
   * @param scale \todo */
  void DrawControl(IControl* pControl, const IRECT& bounds, float scale);
  
  /** Shows a pop up/contextual menu in relation to a rectangular region of the graphics context
   * @param control A reference to the IControl creating this pop-up menu. If it exists IControl::OnPopupMenuSelection() will be called on successful selection
   * @param menu Reference to an IPopupMenu class populated with the items for the platform menu
   * @param bounds The platform menu will popup at the bottom left hand corner of this rectangular region
   * @param isContext Determines if the menu is a contextual menu or not
   * @param valIdx The value index for the control value that the prompt relates to */
  void DoCreatePopupMenu(IControl& control, IPopupMenu& menu, const IRECT& bounds, int valIdx, bool isContext);
  
  /** Called by ICornerResizer when drag resize commences */
  void StartDragResize() { mResizingInProcess = true; }
  
  /** Called when drag resize ends */
  void EndDragResize();

#pragma mark - Control management
public:
  /** For all controls, including the "special controls" call a method
   * @param func A std::function to perform on each control */
  void ForAllControlsFunc(std::function<void(IControl& control)> func);
  
  /** For all controls, including the "special controls" call a method
   * @param method The method to call
   * @param args The method arguments */
  template<typename T, typename... Args>
  void ForAllControls(T method, Args... args);
  
  /** For all standard controls in the main control stack perform a function
   * @param func A std::function to perform on each control */
  void ForStandardControlsFunc(std::function<void(IControl& control)> func);
  
  /** For all standard controls in the main control stack that are linked to a specific parameter, call a method
   * @param method The method to call
   * @param paramIdx The parameter index to match
   * @param args The method arguments */
  template<typename T, typename... Args>
  void ForMatchingControls(T method, int paramIdx, Args... args);

  /** For all standard controls in the main control stack that are linked to a specific parameter, execute a function
   * @param paramIdx The parameter index to match
   * @param func A std::function to perform on each control */
  void ForControlWithParam(int paramIdx, std::function<void(IControl& control)> func);
  
  /** For all standard controls in the main control stack that are linked to a group, execute a function
   * @param group CString specificying the goupd name
   * @param func A std::function to perform on each control */
  void ForControlInGroup(const char* group, std::function<void(IControl& control)> func);
  
  /** Attach an IBitmapControl as the lowest IControl in the control stack to be the background for the graphics context
   * @param fileName CString fileName resource id for the bitmap image */
  void AttachBackground(const char* fileName);

  /** Attach an ISVGControl as the lowest IControl in the control stack to be the background for the graphics context
   * @param fileName CString fileName resource id for the SVG image */
  void AttachSVGBackground(const char* fileName);
  
  /** Attach an IPanelControl as the lowest IControl in the control stack to fill the background with a solid color
   * @param color The color to fill the panel with */
  void AttachPanelBackground(const IPattern& color);
  
  /** Attach the default control to scale or increase the UI size by dragging the plug-in bottom right-hand corner
   * @param sizeMode Choose whether to scale or size the UI */
  void AttachCornerResizer(EUIResizerMode sizeMode = EUIResizerMode::Scale, bool layoutOnResize = false, const IColor& color = COLOR_TRANSLUCENT, const IColor& mouseOverColor = COLOR_BLACK, const IColor& dragColor = COLOR_BLACK, float size = 20.f);

  /** Attach your own control to scale or increase the UI size by dragging the plug-in bottom right-hand corner
   * @param pControl control a control that inherits from ICornerResizerControl
   * @param sizeMode Choose whether to scale or size the UI */
  void AttachCornerResizer(ICornerResizerControl* pControl, EUIResizerMode sizeMode = EUIResizerMode::Scale, bool layoutOnResize = false);

  /** Attach a control for pop-up menus, to override platform style menus
   @param text The text style to use for the menu
   @param bounds The area that the menu should occupy. An empty IRECT will mean the bounds are calculated based on the menu items */
  void AttachPopupMenuControl(const IText& text = DEFAULT_TEXT, const IRECT& bounds = IRECT());
  
  /** Remove the IGraphics popup menu, use platform popup menu if available */
  void RemovePopupMenuControl();
  
  /** Attach a control for text entry, to override platform text entry */
  void AttachTextEntryControl();
  
  /** Remove the IGraphics text entry, use platform text entry if available */
  void RemoveTextEntryControl();
  
  /** Attach the default control to show text as a control changes*/
  void AttachBubbleControl(const IText& text = DEFAULT_TEXT);

  /** Attach a custom control to show text as a control changes*/
  void AttachBubbleControl(IBubbleControl* pControl);
  
  /* Called by controls to display text in the bubble control */
  void ShowBubbleControl(IControl* pCaller, float x, float y, const char* str, EDirection dir = EDirection::Horizontal, IRECT minimumContentBounds = IRECT());

  /** Shows a control to display the frame rate of drawing
   * @param enable \c true to show */
  void ShowFPSDisplay(bool enable);
  
  /** @return \c true if performance display is shown */
  bool ShowingFPSDisplay() { return mPerfDisplay != nullptr; }
  
  /** Attach an IControl to the graphics context and add it to the top of the control stack. The control is owned by the graphics context and will be deleted when the context is deleted.
   * @param pControl A pointer to an IControl to attach.
   * @param ctrlTag An integer tag that you can use to identify the control
   * @param group A CString that you can use to address controlled by group
   * @return The index of the control (and the number of controls in the stack) */
  IControl* AttachControl(IControl* pControl, int ctrlTag = kNoTag, const char* group = "");

  /** @param idx The index of the control to get
   * @return A pointer to the IControl object at idx or nullptr if not found */
  IControl* GetControl(int idx) { return mControls.Get(idx); }

  /** @param pControl Pointer to the control to get
   * @return integer index of the control in mControls array or -1 if not found */
  int GetControlIdx(IControl* pControl) const { return mControls.Find(pControl); }
  
  /** Gets the index of a tagged control
   * @param ctrlTag The tag to look for
   * @return int index or -1 if not found */
  int GetIdxOfTaggedControl(int ctrlTag) const
  {
    IControl* pControl = GetControlWithTag(ctrlTag);
    return pControl ? GetControlIdx(pControl) : -1;
  }
  
  /** @param ctrlTag The tag to look for
   * @return A pointer to the IControl object with the tag of nullptr if not found */
  IControl* GetControlWithTag(int ctrlTag) const;
  
  /** Get the tag given to a control
   * @param pControl Pointer to the control to get the tag for
   * @return The tag assigned to the control when it was attached, or kNoTag (-1) */
  int GetControlTag(const IControl* pControl) const
  {
    for (auto itr = mCtrlTags.begin(); itr != mCtrlTags.end(); ++itr)
    {
      if (itr->second == pControl)
        return itr->first;
    }
    
    return kNoTag;
  }
  
  /** Check to see if any control is captured */
  bool ControlIsCaptured() const { return mCapturedMap.size() > 0; }
  
  /** Check to see if the control is already captured
   * @return \c true is the control is already captured */
  bool ControlIsCaptured(IControl* pControl) const
  {
    return std::find_if(std::begin(mCapturedMap), std::end(mCapturedMap), [pControl](auto&& press) { return press.second == pControl; }) != mCapturedMap.end();
  }

  /** Populate a vector with the touchIDs active on pControl */
  void GetTouches(IControl* pControl, std::vector<ITouchID>& touchesOnThisControl) const
  {
    for (auto i = mCapturedMap.begin(), j = mCapturedMap.end(); i != j; ++i)
      if (i->second == pControl)
        touchesOnThisControl.push_back(i->first);
  }
  
  /* Get the first control in the control list, the background */
  IControl* GetBackgroundControl() { return GetControl(0);  }
  
  /** @return Pointer to the special pop-up menu control, if one has been attached */
  IPopupMenuControl* GetPopupMenuControl() { return mPopupControl.get(); }
  
  /** @return Pointer to the special text entry control, if one has been attached */
  ITextEntryControl* GetTextEntryControl() { return mTextEntryControl.get(); }
  
  /** @return Pointer to the special bubble control at index i, if one has been attached */
  IBubbleControl* GetBubbleControl(int i = 0) { return mBubbleControls.Get(i); }
  
  /** @return Number of attached bubble controls */
  int NBubbleControls() const { return mBubbleControls.GetSize(); }
  
  /** Helper method to style all of the controls which inherit IVectorBase
   * @param IVStyle Style for the controls */
  void StyleAllVectorControls(const IVStyle& style);
  
   /** This method is called after interacting with a control, so that any other controls linked to the same parameter index, will also be set dirty, and have their values updated.
    * @param pCaller The control that triggered the parameter change.
    * @param callerValIdx The index of the value in the control that triggered the parameter change. */
  void UpdatePeers(IControl* pCaller, int callerValIdx);
  
  /** @return The number of controls that have been added to this graphics context */
  int NControls() const { return mControls.GetSize(); }

  /** Remove controls from the control list with a particular tag.  */
  void RemoveControlWithTag(int ctrlTag);
  
  /** Remove controls from the control list above a particular index, (frees memory).  */
  void RemoveControls(int fromIdx);

  /** Remove a control at a particular index, (frees memory). */
  void RemoveControl(int idx);
  
  /** Remove a control at using ptr, (frees memory). */
  void RemoveControl(IControl* pControl);
  
  /** Removes all regular IControls from the control list, as well as special controls (frees memory). */
  void RemoveAllControls();
  
  /** Hide controls linked to a specific parameter
   * @param paramIdx The parameter index
   * @param hide /c true to hide */
  void HideControl(int paramIdx, bool hide);

  /** Disable or enable controls linked to a specific parameter
   * @param paramIdx The parameter index
   * @param disable /c true to disable */
  void DisableControl(int paramIdx, bool diable);

  /** Calls SetDirty() on every control */
  void SetAllControlsDirty();
  
  /** Calls SetClean() on every control */
  void SetAllControlsClean();
    
  /** Reposition a control, redrawing the interface correctly
   @param idx The index of the control
   @param x The new x position
   @param y The new y position */
  void SetControlPosition(int idx, float x, float y);
  
  /** Resize a control, redrawing the interface correctly
   @param idx The index of the control
   @param w The new width
   @param h The new height */
  void SetControlSize(int idx, float w, float h);
  
  /** Set a controls target and draw rect to r, redrawing the interface correctly
   @param idx The index of the control 
   @param r The new bounds for the control's target and draw rect */
  void SetControlBounds(int idx, const IRECT& r);
  
private:
  /** Get the index of the control at x and y coordinates on mouse event
   * @param x The X coordinate to test
   * @param y The Y coordinate to test
   * @param mouseOver Is this initiated from mouse over event
   * @return int the index of the hit control in the control stack */
  int GetMouseControlIdx(float x, float y, bool mouseOver = false);
  
  /** Get the control at x and y coordinates on mouse event
   * @param x The X coordinate to test
   * @param y The Y coordinate to test
   * @param capture should the control be captured
   * @param mouseOver Is this initiated from mouse over event
   * @param touchID The ITouchID relating to the event (multi-touch only)
   * @return IControl* The hit control in the control stack */
  IControl* GetMouseControl(float x, float y, bool capture, bool mouseOver = false, ITouchID touchID = 0);
  
#pragma mark - Event handling
public:
  /** Called when the platform class sends mouse down events */
  void OnMouseDown(const std::vector<IMouseInfo>& points);

  /** Called when the platform class sends mouse up events */
  void OnMouseUp(const std::vector<IMouseInfo>& points);

  /** Called when the platform class sends drag events */
  void OnMouseDrag(const std::vector<IMouseInfo>& points);
  
  /** Called when the platform class sends touch cancel events */
  void OnTouchCancelled(const std::vector<IMouseInfo>& points);

  /** @param x The X coordinate at which the mouse event occurred
   * @param y The Y coordinate at which the mouse event occurred
   * @param mod IMouseMod struct contain information about the modifiers held
   * @return /c true on handled */
  bool OnMouseDblClick(float x, float y, const IMouseMod& mod);

  /** @param x The X coordinate at which the mouse event occurred
   * @param y The Y coordinate at which the mouse event occurred
   * @param mod IMouseMod struct contain information about the modifiers held
   * @param delta Delta value \todo explain */
  void OnMouseWheel(float x, float y, const IMouseMod& mod, float delta);

  /** @param x The X coordinate of the mouse cursor at the time of the key press
   * @param y The Y coordinate of the mouse cursor at the time of the key press
   * @param key Info about the keypress
   * @return \c true if handled */
  bool OnKeyDown(float x, float y, const IKeyPress& key);

  /** @param x The X coordinate of the mouse cursor at the time of the key press
   * @param y The Y coordinate of the mouse cursor at the time of the key press
   * @param key Info about the keypress
   * @return \c true if handled */
  bool OnKeyUp(float x, float y, const IKeyPress& key);
  
  /** @param x The X coordinate at which to draw
   * @param y The Y coordinate at which to draw
   * @param mod IMouseMod struct contain information about the modifiers held
   * @return \c true if handled*/
  bool OnMouseOver(float x, float y, const IMouseMod& mod);

  /** Called when the mouse leaves the graphics context */
  void OnMouseOut();
  
  /** Called when the mouse enters the graphics context, to update the cursor to mCursorType */
  void OnSetCursor() { SetMouseCursor(mCursorType); }

  /** @param str A CString with the absolute path of the dropped item
   * @param x The X coordinate where the drag and drop occurred
   * @param y The Y coordinate where the drag and drop occurred */
  void OnDrop(const char* str, float x, float y);

  /** This is an idle timer tick call on the GUI thread, only active if USE_IDLE_CALLS is defined */
  void OnGUIIdle();
  
  /** Called by ICornerResizerControl as the corner is dragged to resize */
  void OnDragResize(float x, float y);

  /** @param enable Set \c true if you want to handle mouse over messages. Note: this may increase the amount CPU usage if you redraw on mouse overs etc */
  void EnableMouseOver(bool enable) { mEnableMouseOver = enable; }

  /** Used to tell the graphics context to stop tracking mouse interaction with a control */
  void ReleaseMouseCapture();

  /** @return \c true if the context can handle mouse overs */
  bool CanEnableMouseOver() const { return mEnableMouseOver; }

  /** @return An integer representing the control index in IGraphics::mControls which the mouse is over, or -1 if it is not */
  inline int GetMouseOver() const { return mMouseOverIdx; }

  /** Get the x, y position of the last mouse down message. Does not get cleared on mouse up etc.
   * @param x Where the X position will be stored
   * @param y Where the Y position will be stored */
  void GetMouseDownPoint(float& x, float&y) const { x = mMouseDownX; y = mMouseDownY; }
  
  /**  Set by the platform class if the mouse input is coming from a tablet/stylus
   * @param tablet \c true means input is from a tablet */
  void SetTabletInput(bool tablet) { mTabletInput = tablet; }
#pragma mark - Plug-in API Specific

  /** [AAX only] This can be called by the ProTools API class (e.g. IPlugAAX) in order to ascertain the parameter linked to the control under the mouse.
   * The purpose is to facillitate ProTool's special contextual menus (for configuring parameter automation)
   * @param x The X coordinate to check
   * @param y The Y coordinateto check
   * @return An integer representing the parameter index that was found (or -1 if not found) */
  int GetParamIdxForPTAutomation(float x, float y);

  /** [AAX only]
   * @return An integer representing the last clicked parameter index (or -1 if none) */
  int GetLastClickedParamForPTAutomation();

  /** [AAX only] See AAX_CEffectGUI::SetControlHighlightInfo()
   * @param paramIdx The index of the parameter to highlight
   * @param isHighlighted /c true if the parameter should be highlighted
   * @param color An integer corresponding to AAX_EHighlightColor */
  void SetPTParameterHighlight(int paramIdx, bool isHighlighted, int color);

  /** [VST3 primarily] In VST3 plug-ins this enable support for the IContextMenu interface,
   * which allows the host to add contextual options to e.g. automate a parameter associated with a control
   * @param controlIdx The index of the control in the control stack
   * @param paramIdx The parameter index associated with the control
   * @param x The X coordinate at which to popup the context menu
   * @param y The Y coordinate at which to popup the context menu */
  void PopupHostContextMenuForParam(int controlIdx, int paramIdx, float x, float y);

  /** [VST3 primarily] In VST3 plug-ins this enable support for the IContextMenu interface,
   * which allows the host to add contextual options to e.g. automate a parameter associated with a control
   * @param pControl Ptr to the control in the control stack
   * @param paramIdx The parameter index associated with the control
   * @param x The X coordinate at which to popup the context menu
   * @param y The Y coordinate at which to popup the context menu */
  void PopupHostContextMenuForParam(IControl* pControl, int paramIdx, float x, float y);
  
#pragma mark - Resource/File Loading
  
  /** Gets the name of the shared resources subpath. */
  const char* GetSharedResourcesSubPath() const { return mSharedResourcesSubPath.Get(); }
  
  /** Sets the name of the shared resources subpath. */
  void SetSharedResourcesSubPath(const char* sharedResourcesSubPath) { mSharedResourcesSubPath.Set(sharedResourcesSubPath); }
  
  /** Load a bitmap image from disk or from windows resource
   * @param fileNameOrResID CString file name or resource ID
   * @param nStates The number of states/frames in a multi-frame stacked bitmap
   * @param framesAreHorizontal Set \c true if the frames in a bitmap are stacked horizontally
   * @param targetScale Set \c to a number > 0 to explicity load e.g. an @2x.png
   * @return An IBitmap representing the image */
  virtual IBitmap LoadBitmap(const char* fileNameOrResID, int nStates = 1, bool framesAreHorizontal = false, int targetScale = 0);

  /** Load a bitmap image from memory
   * @param name CString name to associate with the bitmap, must include a file extension
   * @param pData pointer to the bitmap file data
   * @param dataSize size of the data at \c pData
   * @param nStates The number of states/frames in a multi-frame stacked bitmap
   * @param framesAreHorizontal Set \c true if the frames in a bitmap are stacked horizontally
   * @param targetScale Set \c to a number > 0 to explicity load e.g. an @2x.png
   * @return An IBitmap representing the image */
  virtual IBitmap LoadBitmap(const char *name, const void* pData, int dataSize, int nStates = 1, bool framesAreHorizontal = false, int targetScale = 0);

  /** Load an SVG from disk or from windows resource
   * @param fileNameOrResID A CString absolute path or resource ID
   * @return An ISVG representing the image */
  virtual ISVG LoadSVG(const char* fileNameOrResID, const char* units = "px", float dpi = 72.f);

  /** Load an SVG image from memory
   * @param name CString name to associate with the SVG
   * @param pData Pointer to the SVG file data
   * @param dataSize Size (in bytes) of the data at \c pData
   * @param units \todo
   * @param dpi The dots per inch of the SVG file
   * @return An ISVG representing the image */
  virtual ISVG LoadSVG(const char* name, const void* pData, int dataSize, const char* units = "px", float dpi = 72.f);

  /** Load a resource from the file system, the bundle, or a Windows resource, and returns its data
   * @param fileNameOrResID CString file name or resource ID
   * @param fileType Type of the file (e.g "png", "svg", "ttf")
   * @return A WDL_TypedBuf containing the data, or with a length of 0 if the resource was not found */
  virtual WDL_TypedBuf<uint8_t> LoadResource(const char* fileNameOrResID, const char* fileType);

  /** Registers a gesture recognizer with the graphics context
   * @param type The type of gesture recognizer */
  virtual void AttachGestureRecognizer(EGestureType type); //TODO: should be protected?
  
  /** Attach a gesture recognizer to a rectangular region of the GUI, i.e. not linked to an IControl
   * @param bounds The area that should recognize the gesture
   * @param type The type of gesture to recognize
   * @param func The function to call when the gesture is recognized */
  void AttachGestureRecognizerToRegion(const IRECT& bounds, EGestureType type, IGestureFunc func);
  
  /** Remove all gesture recognizers linked to regions */
  void ClearGestureRegions();

protected:
  /** Drawing API method to load a bitmap, called internally
   * @param fileNameOrResID A CString absolute path or resource ID
   * @param scale Integer to identify the scale of the resource, for multi-scale bitmaps
   * @param location Identifies the kind of resource location
   * @param ext CString for the file extension
   * @return APIBitmap* Drawing API bitmap abstraction */
  virtual APIBitmap* LoadAPIBitmap(const char* fileNameOrResID, int scale, EResourceLocation location, const char* ext) = 0;

  /** Drawing API method to load a bitmap from binary data, called internally
   * @param name CString for the name of the resource
   * @param pData Raw pointer to the binary data
   * @param dataSize Size of the data in bytes
   * @param scale Integer to identify the scale of the resource, for multi-scale bitmaps
   * @return APIBitmap* Drawing API bitmap abstraction */
  virtual APIBitmap* LoadAPIBitmap(const char* name, const void* pData, int dataSize, int scale) = 0;

  /** Creates a new API bitmap, either in memory or as a GPU texture
   * @param width The desired width
   * @param height The desired height
   * @param scale The scale in relation to 1:1 pixels
   * @param drawScale \todo
   * @param cacheable Used to make sure the underlying bitmap can be shared between plug-in instances
   * @return APIBitmap* The new API Bitmap */
  virtual APIBitmap* CreateAPIBitmap(int width, int height, int scale, double drawScale, bool cacheable = false) = 0;

  /** Drawing API method to load a font from a PlatformFontPtr, called internally
   * @param fontID A CString that will be used to reference the font
   * @param font Valid PlatformFontPtr, loaded via LoadPlatformFont
   * @return bool \c true if the font was loaded successfully */
  virtual bool LoadAPIFont(const char* fontID, const PlatformFontPtr& font) = 0;

  /** Specialized in IGraphicsCanvas drawing backend */
  virtual bool AssetsLoaded() { return true; }
    
  /** @return int The index of the alpha component in a drawing backend's pixel (RGBA or ARGB) */
  virtual int AlphaChannel() const = 0;

  /** @return bool \c true if the drawing backend flips images (e.g. OpenGL) */
  virtual bool FlippedBitmap() const = 0;

  /** Utility used by SearchImageResource/SearchBitmapInCache
   * @param sourceScale \todo
   * @param targetScale \todo */
  inline void SearchNextScale(int& sourceScale, int targetScale);

  /** Search for a bitmap image resource matching the target scale 
   * @param fileName \todo
   * @param type \todo 
   * @param result \todo
   * @param targetScale \todo
   * @param sourceScale \todo
   * @return EResourceLocation \todo */
  EResourceLocation SearchImageResource(const char* fileName, const char* type, WDL_String& result, int targetScale, int& sourceScale);

  /** Search the static storage cache for a bitmap image resource matching the target scale
   * @param fileName \todo
   * @param targetScale \todo
   * @param sourceScale \todo
   * @return  pointer to the bitmap in the cache,  or null pointer if not found */
  APIBitmap* SearchBitmapInCache(const char* fileName, int targetScale, int& sourceScale);

  /** \todo
   * @param text \todo
   * @param str \todo
   * @param bounds \todo
   * @return The width of the text */
  virtual float DoMeasureText(const IText& text, const char* str, IRECT& bounds) const = 0;
    
  /** \todo
   * @param text \todo
   * @param str \todo
   * @param bounds \todo
   * @param pBlend \todo */
  virtual void DoDrawText(const IText& text, const char* str, const IRECT& bounds, const IBlend* pBlend = nullptr) = 0;

  /** \todo
   * @param text \todo
   * @param bounds \todo
   * @param rect \todo */
  void DoMeasureTextRotation(const IText& text, const IRECT& bounds, IRECT& rect) const;
  
  /** \todo
   * @param text \todo
   * @param bounds \todo
   * @param rect \todo
   * @param tx \todo
   * @param ty \todo */
  void CalculateTextRotation(const IText& text, const IRECT& bounds, IRECT& rect, double& tx, double& ty) const;
  
  /** @return float \todo */
  virtual float GetBackingPixelScale() const { return GetScreenScale() * GetDrawScale(); };

  IMatrix GetTransformMatrix() const { return mTransform; }
#pragma mark -

private:
  void ClearMouseOver()
  {
    mMouseOver = nullptr;
    mMouseOverIdx = -1;
  }
  
  WDL_PtrList<IControl> mControls;
  std::unordered_map<int, IControl*> mCtrlTags;

  // Order (front-to-back) ToolTip / PopUp / TextEntry / LiveEdit / Corner / PerfDisplay
  std::unique_ptr<ICornerResizerControl> mCornerResizer;
  WDL_PtrList<IBubbleControl> mBubbleControls;
  std::unique_ptr<IPopupMenuControl> mPopupControl;
  std::unique_ptr<IFPSDisplayControl> mPerfDisplay;
  std::unique_ptr<ITextEntryControl> mTextEntryControl;
  std::unique_ptr<IControl> mLiveEdit;
  
  IPopupMenu mPromptPopupMenu;
  
  WDL_String mSharedResourcesSubPath;
  
  ECursor mCursorType = ECursor::ARROW;
  int mWidth;
  int mHeight;
  int mFPS;
  int mScreenScale = 1; // the scaling of the display that the UI is currently on e.g. 2 for retina
  float mDrawScale = 1.f; // scale deviation from  default width and height i.e stretching the UI by dragging bottom right hand corner

  int mIdleTicks = 0;
  
  std::vector<EGestureType> mRegisteredGestures; // All the types of gesture registered with the graphics context
  IRECTList mGestureRegions; // Rectangular regions linked to gestures (excluding IControls)
  std::unordered_map<int, IGestureFunc> mGestureRegionFuncs; // Map of gesture region index to gesture function
  std::unordered_map<ITouchID, IControl*> mCapturedMap; // associative array of touch ids to control pointers, the same control can be touched multiple times
  IControl* mMouseOver = nullptr;
  IControl* mInTextEntry = nullptr;
  IControl* mInPopupMenu = nullptr;
  void* mPlatformContext = nullptr;
  bool mIsContextMenu = false;
  int mTextEntryValIdx = kNoValIdx;
  int mPopupMenuValIdx = kNoValIdx;
  int mMouseOverIdx = -1;
  float mMouseDownX = -1.f;
  float mMouseDownY = -1.f;
  float mMinScale;
  float mMaxScale;
  int mLastClickedParam = kNoParameter;
  bool mEnableMouseOver = false;
  bool mStrict = false;
  bool mEnableTooltips = false;
  bool mShowControlBounds = false;
  bool mShowAreaDrawn = false;
  bool mResizingInProcess = false;
  bool mLayoutOnResize = false;
  bool mEnableMultiTouch = false;
  EUIResizerMode mGUISizeMode = EUIResizerMode::Scale;
  double mPrevTimestamp = 0.;
  IKeyHandlerFunc mKeyHandlerFunc = nullptr;
  IDisplayTickFunc mDisplayTickFunc = nullptr;

protected:
  IGEditorDelegate* mDelegate;
  bool mCursorHidden = false;
  bool mCursorLock = false;
  bool mTabletInput = false;
  float mCursorX = -1.f;
  float mCursorY = -1.f;
  float mXTranslation = 0.f;
  float mYTranslation = 0.f;
  
  friend class IGraphicsLiveEdit;
  friend class ICornerResizerControl;
  friend class ITextEntryControl;
  
  std::stack<ILayer*> mLayers;

  IRECT mClipRECT;
  IMatrix mTransform;
  std::stack<IMatrix> mTransformStates;
  
#ifdef IGRAPHICS_IMGUI
public:
  std::unique_ptr<ImGuiRenderer> mImGuiRenderer;
#endif
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
