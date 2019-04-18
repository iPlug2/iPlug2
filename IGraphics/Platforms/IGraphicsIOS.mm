/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#import <QuartzCore/QuartzCore.h>

#include "IGraphicsIOS.h"
#include "IGraphicsCoreText.h"

#import "IGraphicsIOS_view.h"

#include "IControl.h"
#include "IPopupMenuControl.h"

#pragma clang diagnostic ignored "-Wdeprecated-declarations"

StaticStorage<CoreTextFontDescriptor> sFontDescriptorCache;

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
#ifdef IGRAPHICS_IMGUI
    if(mImGuiView)
    {
      IGRAPHICS_IMGUIVIEW* pImGuiView = (IGRAPHICS_IMGUIVIEW*) mImGuiView;
      [pImGuiView removeFromSuperview];
      [pImGuiView release];
      mImGuiView = nullptr;
    }
#endif
    
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

void IGraphicsIOS::CreatePlatformImGui()
{
#ifdef IGRAPHICS_IMGUI
  if(mView)
  {
    IGraphicsIOS_View* pView = (IGraphicsIOS_View*) mView;
    
    IGRAPHICS_IMGUIVIEW* pImGuiView = [[IGRAPHICS_IMGUIVIEW alloc] initWithIGraphicsView:pView];
    [pView addSubview: pImGuiView];
    mImGuiView = pImGuiView;
  }
#endif
}

PlatformFontPtr IGraphicsIOS::LoadPlatformFont(const char* fontID, const char* fileNameOrResID)
{
  return CoreTextHelpers::LoadPlatformFont(fontID, fileNameOrResID, GetBundleID());
}

PlatformFontPtr IGraphicsIOS::LoadPlatformFont(const char* fontID, const char* fontName, ETextStyle style)
{
  return CoreTextHelpers::LoadPlatformFont(fontID, fontName, style);
}

void IGraphicsIOS::CachePlatformFont(const char* fontID, const PlatformFontPtr& font)
{
  CoreTextHelpers::CachePlatformFont(fontID, font, sFontDescriptorCache);
}
