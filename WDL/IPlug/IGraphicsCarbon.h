#ifndef _IGRAPHICSCARBON_
#define _IGRAPHICSCARBON_

#include <Carbon/Carbon.h>
#include "IGraphicsMac.h"

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

// The text entry can be implemented either using a HIViewControl or the older MLTE
// MLTE is more customisable and doesn't have to have a blue focus rim, but
// text doesn't centre properly and when you select the text and drag it behaves strangely

// MLTE text entry code was adapted from expdigital's infinity api http://www.expdigital.co.uk/legacy2/developers.htm

#ifndef USE_MLTE
  #define USE_MLTE 0
#endif

class IGraphicsCarbon
{
public:
  IGraphicsCarbon(IGraphicsMac* pGraphicsMac, WindowRef pWindow, ControlRef pParentControl, short leftOffset, short topOffset);
  ~IGraphicsCarbon();

  ControlRef GetView() { return mView; }
  CGContextRef GetCGContext() { return mCGC; }
  bool GetIsComposited() {return mIsComposited;}
  short GetLeftOffset() { return mLeftOffset; }
  short GetTopOffset() { return mTopOffset; }

  bool Resize(int w, int h);

  MenuRef CreateMenu(IPopupMenu* pMenu);
  IPopupMenu* CreateIPopupMenu(IPopupMenu* pMenu, IRECT* pAreaRect);
  void CreateTextEntry(IControl* pControl, IText* pText, IRECT* pTextRect, const char* pString, IParam* pParam);

  void EndUserInput(bool commit);
  
protected:
  void ShowTooltip();
  void HideTooltip();

private:
  IGraphicsMac* mGraphicsMac;
  bool mIsComposited;
//  RgnHandle mRgn;
  WindowRef mWindow;
  ControlRef mView;
  EventLoopTimerRef mTimer;
  EventHandlerRef mControlHandler;
  EventHandlerRef mWindowHandler;
  EventHandlerRef mTextEntryHandler;
  CGContextRef mCGC;

  #if USE_MLTE
  TXNObject mTextEntryView;
  IRECT mTextEntryRect;
  #else
  ControlRef mTextEntryView;
  #endif

  IControl* mEdControl;
  IParam* mEdParam;
  int mPrevX, mPrevY;
  short mLeftOffset, mTopOffset; // only for RTAS
  
  bool mShowingTooltip;
  int mTooltipIdx, mTooltipTimer;
  const char* mTooltip;

public:
  static pascal OSStatus MainEventHandler(EventHandlerCallRef pHandlerCall, EventRef pEvent, void* pGraphicsCarbon);
  static pascal void TimerHandler(EventLoopTimerRef pTimer, void* pGraphicsCarbon);
  static pascal OSStatus TextEntryHandler(EventHandlerCallRef pHandlerCall, EventRef pEvent, void* pGraphicsCarbon);
};

#endif // _IGRAPHICSCARBON_
