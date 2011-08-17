#ifndef _IGRAPHICSCARBON_
#define _IGRAPHICSCARBON_

#include <Carbon/Carbon.h>
#include "IGraphicsMac.h"

#ifndef IPLUG_NO_CARBON_SUPPORT

class IGraphicsCarbon
{
public:
  
  IGraphicsCarbon(IGraphicsMac* pGraphicsMac, WindowRef pWindow, ControlRef pParentControl);
  ~IGraphicsCarbon();
  
  ControlRef GetView() { return mView; }
  CGContextRef GetCGContext() { return mCGC; }
  bool GetIsComposited() {return mIsComposited;}

  void OffsetContentRect(CGRect* pR);
  bool Resize(int w, int h);

	IPopupMenu* CreateIPopupMenu(IPopupMenu* pMenu, IRECT* pAreaRect);
  void CreateTextEntry(IControl* pControl, IText* pText, IRECT* pTextRect, const char* pString, IParam* pParam);

  void EndUserInput(bool commit);
  
private:
  
  IGraphicsMac* mGraphicsMac;
  bool mIsComposited;
  int mContentXOffset, mContentYOffset;
  RgnHandle mRgn;
  WindowRef mWindow;
  ControlRef mView; // was HIViewRef
  EventLoopTimerRef mTimer;
  EventHandlerRef mControlHandler, mWindowHandler;
  CGContextRef mCGC;

  ControlRef mTextFieldView;
  EventHandlerRef mParamEditHandler;
  // Ed = being edited manually.
  IControl* mEdControl;
  IParam* mEdParam;
  int mPrevX, mPrevY;
  
public:
  
  static pascal OSStatus CarbonEventHandler(EventHandlerCallRef pHandlerCall, EventRef pEvent, void* pGraphicsCarbon);
  static pascal void CarbonTimerHandler(EventLoopTimerRef pTimer, void* pGraphicsCarbon);
  static pascal OSStatus CarbonParamEditHandler(EventHandlerCallRef pHandlerCall, EventRef pEvent, void* pGraphicsCarbon);
};

#endif // IPLUG_NO_CARBON_SUPPORT
#endif // _IGRAPHICSCARBON_
