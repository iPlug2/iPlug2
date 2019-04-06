/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#ifndef NO_IGRAPHICS
#import <QuartzCore/QuartzCore.h>
#import "IGraphicsIOS_view.h"

#include "IGraphicsIOS.h"
#include "IControl.h"
#include "IPopupMenuControl.h"

#include "IPlugPluginBase.h"
#include "IPlugPaths.h"

#pragma clang diagnostic ignored "-Wdeprecated-declarations"

#pragma mark -

IGraphicsIOS::IGraphicsIOS(IGEditorDelegate& dlg, int w, int h, int fps, float scale)
: IGraphicsNanoVG(dlg, w, h, fps, scale)
{
}

IGraphicsIOS::~IGraphicsIOS()
{
  CloseWindow();
}

void* IGraphicsIOS::OpenWindow(void* pParent)
{
  TRACE;
  CloseWindow();
  mView = (IGraphicsIOS_View*) [[IGraphicsIOS_View alloc] initWithIGraphics: this];
  
  IGraphicsIOS_View* view = (IGraphicsIOS_View*) mView;
  
  OnViewInitialized([view layer]);
  
  SetScreenScale([UIScreen mainScreen].scale);
  
  GetDelegate()->LayoutUI(this);

  if (pParent)
  {
    [(UIView*) pParent addSubview: (IGraphicsIOS_View*) mView];
  }

  return mView;
}

void IGraphicsIOS::CloseWindow()
{
  if (mView)
  {
    IGraphicsIOS_View* view = (IGraphicsIOS_View*) mView;

    mView = nullptr;
    
    if (view->mGraphics)
    {
      [view removeFromSuperview];  
    }
    [view release];

    OnViewDestroyed();
  }
}

bool IGraphicsIOS::WindowIsOpen()
{
  return mView;
}

void IGraphicsIOS::PlatformResize()
{
  if (mView)
  {
    //TODO
  }
}

int IGraphicsIOS::ShowMessageBox(const char* str, const char* caption, EMessageBoxType type)
{
  //TODO
  return 0;
}

void IGraphicsIOS::ForceEndUserEdit()
{
  if (mView)
  {
    [(IGraphicsIOS_View*) mView endUserInput];
  }
}

const char* IGraphicsIOS::GetPlatformAPIStr()
{
  return "iOS";
}

void IGraphicsIOS::PromptForFile(WDL_String& fileName, WDL_String& path, EFileAction action, const char* ext)
{
}

void IGraphicsIOS::PromptForDirectory(WDL_String& dir)
{
}

bool IGraphicsIOS::PromptForColor(IColor& color, const char* str)
{
  return false;
}

IPopupMenu* IGraphicsIOS::CreatePlatformPopupMenu(IPopupMenu& menu, const IRECT& bounds, IControl* pCaller)
{
  IPopupMenu* pReturnMenu = nullptr;
  
  if (mView)
  {
    CGRect areaRect = ToCGRect(this, bounds);
    pReturnMenu = [(IGraphicsIOS_View*) mView createPopupMenu: menu: areaRect];
  }
  
  //synchronous
  if(pReturnMenu && pReturnMenu->GetFunction())
    pReturnMenu->ExecFunction();
  
  if(pCaller)
    pCaller->OnPopupMenuSelection(pReturnMenu); // should fire even if pReturnMenu == nullptr

  return pReturnMenu;
}

void IGraphicsIOS::CreatePlatformTextEntry(IControl& control, const IText& text, const IRECT& bounds, const char* str)
{
}

bool IGraphicsIOS::OpenURL(const char* url, const char* msgWindowTitle, const char* confirmMsg, const char* errMsgOnFailure)
{
  return false;
}

void* IGraphicsIOS::GetWindow()
{
  if (mView)
    return mView;
  else
    return 0;
}

// static
int IGraphicsIOS::GetUserOSVersion()
{
  return (int) 0; //TODO
}

bool IGraphicsIOS::GetTextFromClipboard(WDL_String& str)
{
  return false;
}

#endif// NO_IGRAPHICS
