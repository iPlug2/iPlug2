#ifndef _IGRAPHICSCARBON_
#define _IGRAPHICSCARBON_

#include <Carbon/Carbon.h>
#include "IGraphicsMac.h"

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

class IGraphicsCarbon
{
public:
  
  IGraphicsCarbon(IGraphicsMac* pGraphicsMac, WindowRef pWindow, ControlRef pParentControl);
  ~IGraphicsCarbon();
  
  ControlRef GetView() { return mView; }
  CGContextRef GetCGContext() { return mCGC; }
  bool GetIsComposited() {return mIsComposited;}

  bool Resize(int w, int h);

	IPopupMenu* CreateIPopupMenu(IPopupMenu* pMenu, IRECT* pAreaRect);
  void CreateTextEntry(IControl* pControl, IText* pText, IRECT* pTextRect, const char* pString, IParam* pParam);

  void EndUserInput(bool commit);
  
private:
  
  IGraphicsMac* mGraphicsMac;
  bool mIsComposited;
  RgnHandle mRgn;
  WindowRef mWindow;
  ControlRef mView;
  ControlRef mTextEntryView;
  EventLoopTimerRef mTimer;
  EventHandlerRef mControlHandler;
  EventHandlerRef mWindowHandler;
  EventHandlerRef mTextEntryHandler;
  CGContextRef mCGC;

  IControl* mEdControl;
  IParam* mEdParam;
  int mPrevX, mPrevY;
  
public:
  
  static pascal OSStatus MainEventHandler(EventHandlerCallRef pHandlerCall, EventRef pEvent, void* pGraphicsCarbon);
  static pascal void TimerHandler(EventLoopTimerRef pTimer, void* pGraphicsCarbon);
  static pascal OSStatus TextEntryHandler(EventHandlerCallRef pHandlerCall, EventRef pEvent, void* pGraphicsCarbon);
};

#endif // _IGRAPHICSCARBON_
