#pragma once

#include "IPlugPlatform.h"

#include "cairo/cairo.h"
#ifdef OS_MAC
#include "cairo/cairo-quartz.h"
#else
#include "cairo/cairo-win32.h"
#endif

#ifdef IGRAPHICS_FREETYPE
#include "cairo/cairo-ft.h"
#endif

#include "IGraphicsPathBase.h"

class CairoBitmap : public APIBitmap
{
public:
  CairoBitmap(cairo_surface_t* s, int scale);
  virtual ~CairoBitmap();
};

/** IGraphics draw class using Cairo
*   @ingroup DrawClasses
*/
class IGraphicsCairo : public IGraphicsPathBase
{
public:
  const char* GetDrawingAPIStr() override { return "CAIRO"; }

  IGraphicsCairo(IDelegate& dlg, int w, int h, int fps);
  ~IGraphicsCairo();

  void DrawBitmap(IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend) override;
      
  void PathClear() override { cairo_new_path(mContext); }
  void PathStart() override { cairo_new_sub_path(mContext); }
  void PathClose() override { cairo_close_path(mContext); }

  void PathArc(float cx, float cy, float r, float aMin, float aMax) override { cairo_arc(mContext, cx, cy, r, DegToRad(aMin), DegToRad(aMax)); }
    
  void PathMoveTo(float x, float y) override { cairo_move_to(mContext, x, y); }
  void PathLineTo(float x, float y) override { cairo_line_to(mContext, x, y); }
  void PathCurveTo(float x1, float y1, float x2, float y2, float x3, float y3) override { cairo_curve_to(mContext, x1, y1, x2, y2, x3, y3); }

  void PathStroke(const IPattern& pattern, float thickness, const IStrokeOptions& options, const IBlend* pBlend) override;
  void PathFill(const IPattern& pattern, const IFillOptions& options, const IBlend* pBlend) override;
  
  void PathStateSave() override { cairo_save(mContext); }
  void PathStateRestore() override { cairo_restore(mContext); }
  
  void PathTransformTranslate(float x, float y) override { cairo_translate(mContext, x, y); }
  void PathTransformScale(float scaleX, float scaleY) override { cairo_scale(mContext, scaleX, scaleY); }
  void PathTransformRotate(float angle) override { cairo_rotate(mContext, DegToRad(angle)); }
    
  IColor GetPoint(int x, int y) override;
  void* GetData() override { return (void*) mContext; }

  bool DrawText(const IText& text, const char* str, IRECT& bounds, bool measure) override;
  bool MeasureText(const IText& text, const char* str, IRECT& bounds) override;

  void RenderDrawBitmap() override;

  void SetPlatformContext(void* pContext) override;

protected:

  APIBitmap* LoadAPIBitmap(const WDL_String& resourcePath, int scale) override;
  APIBitmap* ScaleAPIBitmap(const APIBitmap* pBitmap, int scale) override;

  void SetCairoSourcePattern(const IPattern& pattern, const IBlend* pBlend);
  
private:
  
  void ClipRegion(const IRECT& r) override
  {
    cairo_reset_clip(mContext);
    PathClear();
    PathRect(r);
    cairo_clip(mContext);
  }
  
  void ResetClipRegion() override
  {
    cairo_reset_clip(mContext);
  }
  
  cairo_t* mContext;
  cairo_surface_t* mSurface;
};
