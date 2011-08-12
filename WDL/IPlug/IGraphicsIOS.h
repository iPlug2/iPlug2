#ifndef _IGRAPHICSIOS_
#define _IGRAPHICSIOS_

#include "IGraphics.h"

// Stupid dummy IOS Graphics class, not used at the moment because the whole gui is done in cocoa

class IGraphicsIOS : public IGraphics
{
public:
  IGraphicsIOS(IPlugBase* pPlug, int w, int h, int refreshFPS) : IGraphics(pPlug, w, h, refreshFPS) {}
  ~IGraphicsIOS() {}
  void PrepDraw() {}
	bool DrawBitmap(IBitmap* pBitmap, IRECT* pDest, int srcX, int srcY, const IChannelBlend* pBlend = 0) { return false; }
	bool DrawRotatedBitmap(IBitmap* pBitmap, int destCtrX, int destCtrY, double angle, int yOffsetZeroDeg = 0, const IChannelBlend* pBlend = 0) { return false; } 
	bool DrawRotatedMask(IBitmap* pBase, IBitmap* pMask, IBitmap* pTop, int x, int y, double angle, const IChannelBlend* pBlend = 0) { return false; } 
	bool DrawPoint(const IColor* pColor, float x, float y, const IChannelBlend* pBlend = 0, bool antiAlias = false) { return false; }
  bool ForcePixel(const IColor* pColor, int x, int y) { return false; }
	bool DrawLine(const IColor* pColor, float x1, float y1, float x2, float y2, const IChannelBlend* pBlend = 0, bool antiAlias = false) { return false; }
	bool DrawArc(const IColor* pColor, float cx, float cy, float r, float minAngle, float maxAngle, const IChannelBlend* pBlend = 0, bool antiAlias = false) { return false; }
	bool DrawCircle(const IColor* pColor, float cx, float cy, float r, const IChannelBlend* pBlend = 0, bool antiAlias = false) { return false; }
  bool FillIRect(const IColor* pColor, IRECT* pR, const IChannelBlend* pBlend = 0) { return false; }
	bool RoundRect(const IColor* pColor, IRECT* pR, const IChannelBlend* pBlend, int cornerradius, bool aa) { return false; }
	bool FillCircle(const IColor* pColor, int cx, int cy, float r, const IChannelBlend* pBlend = 0, bool antiAlias = false) { return false; }
	bool FillIConvexPolygon(const IColor* pColor, int* x, int* y, int npoints, const IChannelBlend* pBlend = 0) { return false; }
  bool FillTriangle(const IColor* pColor, int x1, int y1, int x2, int y2, int x3, int y3, IChannelBlend* pBlend) { return false; }
  IBitmap LoadIBitmap(int ID, const char* name, int nStates = 1) { return 0; }
  IBitmap ScaleBitmap(IBitmap* pSrcBitmap, int destW, int destH) { return 0; }
  IBitmap CropBitmap(IBitmap* pSrcBitmap, IRECT* pR) { return 0; }
  void RetainBitmap(IBitmap* pBitmap) {}
  void ReleaseBitmap(IBitmap* pBitmap) {}
  IColor GetPoint(int x, int y) { return 0; }
  void* GetData() { return 0; }
  void SetBundleID(const char* bundleID) { mBundleID.Set(bundleID); }
  bool DrawScreen(IRECT* pR) { return 0; }
  void* OpenWindow(void* pWindow) { return 0; }
  void CloseWindow() {}
  bool WindowIsOpen() { return false; }
  void Resize(int w, int h) {}  
  void ForceEndUserEdit() {}
  void HostPath(WDL_String* pPath) {} 
  void PluginPath(WDL_String* pPath) {}
  void PromptForFile(WDL_String* pFilename, EFileAction action = kFileOpen, char* dir = "", char* extensions = "") {}
  bool PromptForColor(IColor* pColor, char* prompt = "") { return false; }
  IPopupMenu* CreateIPopupMenu(IPopupMenu* pMenu, IRECT* pTextRect) { return 0; }
  void CreateTextEntry(IControl* pControl, IText* pText, IRECT* pTextRect, const char* pString, IParam* pParam ) {}
  bool OpenURL(const char* url, const char* msgWindowTitle = 0, const char* confirmMsg = 0, const char* errMsgOnFailure = 0) { return false; }
  void* GetWindow() { return 0; }
  const char* GetBundleID()  { return mBundleID.Get(); }
  static int GetUserOSVersion() { return 0; }
  bool DrawIText(IText* pTxt, char* str, IRECT* pR) { return false; }
private:
  WDL_String mBundleID;
};

#endif //_IGRAPHICSIOS_