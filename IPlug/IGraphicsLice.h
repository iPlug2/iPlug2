#pragma once

#include "IControl.h"
#include "lice.h"

class IGraphicsLice : public IGraphics
{
public:
  IGraphicsLice(IPlugBase* pPlug, int w, int h, int refreshFPS = 0);
  ~IGraphicsLice();

  void PrepDraw();

  bool DrawBitmap(IBitmap* pBitmap, IRECT* pDest, int srcX, int srcY, const IChannelBlend* pBlend = 0);
  virtual bool DrawRotatedBitmap(IBitmap* pBitmap, int destCtrX, int destCtrY, double angle, int yOffsetZeroDeg = 0, const IChannelBlend* pBlend = 0);
  virtual bool DrawRotatedMask(IBitmap* pBase, IBitmap* pMask, IBitmap* pTop, int x, int y, double angle, const IChannelBlend* pBlend = 0);
  bool DrawPoint(const IColor* pColor, float x, float y, const IChannelBlend* pBlend = 0, bool antiAlias = false);
  bool ForcePixel(const IColor* pColor, int x, int y);
  bool DrawLine(const IColor* pColor, float x1, float y1, float x2, float y2, const IChannelBlend* pBlend = 0, bool antiAlias = false);
  bool DrawArc(const IColor* pColor, float cx, float cy, float r, float minAngle, float maxAngle,  const IChannelBlend* pBlend = 0, bool antiAlias = false);
  bool DrawCircle(const IColor* pColor, float cx, float cy, float r,const IChannelBlend* pBlend = 0, bool antiAlias = false);
  bool FillCircle(const IColor* pColor, int cx, int cy, float r, const IChannelBlend* pBlend = 0, bool antiAlias = false);
  bool FillIRect(const IColor* pColor, IRECT* pR, const IChannelBlend* pBlend = 0);
  bool RoundRect(const IColor* pColor, IRECT* pR, const IChannelBlend* pBlend, int cornerradius, bool aa);
  bool FillRoundRect(const IColor* pColor, IRECT* pR, const IChannelBlend* pBlend, int cornerradius, bool aa);
  bool FillIConvexPolygon(const IColor* pColor, int* x, int* y, int npoints, const IChannelBlend* pBlend = 0);
  bool FillTriangle(const IColor* pColor, int x1, int y1, int x2, int y2, int x3, int y3, IChannelBlend* pBlend);
  IColor GetPoint(int x, int y);
  void* GetData() { return GetBits(); }
  bool DrawVerticalLine(const IColor* pColor, int xi, int yLo, int yHi);
  bool DrawHorizontalLine(const IColor* pColor, int yi, int xLo, int xHi);
  
  bool DrawIText(IText* pTxt, const char* str, IRECT* pR, bool measure = false);
  IBitmap LoadIBitmap(int ID, const char* name, int nStates = 1, bool framesAreHoriztonal = false);

  // Specialty use...
  IBitmap ScaleBitmap(IBitmap* pBitmap, int destW, int destH);
  IBitmap CropBitmap(IBitmap* pBitmap, IRECT* pR);
  void RetainBitmap(IBitmap* pBitmap);
  void ReleaseBitmap(IBitmap* pBitmap);
  LICE_pixel* GetBits();
  
  inline LICE_SysBitmap* GetDrawBitmap() const { return mDrawBitmap; }

protected:
  virtual LICE_IBitmap* OSLoadBitmap(int ID, const char* name) = 0;
  
  LICE_SysBitmap* mDrawBitmap;
  LICE_IFont* CacheFont(IText* pTxt);
  
private:
  LICE_MemBitmap* mTmpBitmap;
};
