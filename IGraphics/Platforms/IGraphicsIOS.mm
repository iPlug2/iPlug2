/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#import <QuartzCore/QuartzCore.h>
#import <MetalKit/MetalKit.h>

#include "IGraphicsIOS.h"
#include "IGraphicsCoreText.h"

#import "IGraphicsIOS_view.h"

#include <map>
#include <string>

#pragma clang diagnostic ignored "-Wdeprecated-declarations"

using namespace iplug;
using namespace igraphics;

StaticStorage<CoreTextFontDescriptor> sFontDescriptorCache;

#pragma mark -

std::map<std::string, MTLTexturePtr> gTextureMap;

IGraphicsIOS::IGraphicsIOS(IGEditorDelegate& dlg, int w, int h, int fps, float scale)
: IGRAPHICS_DRAW_CLASS(dlg, w, h, fps, scale)
{
 
  if(!gTextureMap.size())
  {
    MTKTextureLoader* textureLoader = [[MTKTextureLoader alloc] initWithDevice:MTLCreateSystemDefaultDevice()];

    NSBundle* pBundle = [NSBundle mainBundle];
   
    if(IsAuv3AppExtension())
      pBundle = [NSBundle bundleWithPath: [[[pBundle bundlePath] stringByDeletingLastPathComponent] stringByDeletingLastPathComponent]];
    
    NSArray<NSURL*>* textureFiles = [pBundle URLsForResourcesWithExtension:@"ktx" subdirectory:@""];

    NSError* pError = nil;
    NSDictionary* textureOptions = @{ MTKTextureLoaderOptionSRGB: [NSNumber numberWithBool:NO] };

    NSArray<id<MTLTexture>>* textures = [textureLoader newTexturesWithContentsOfURLs:textureFiles options:textureOptions error:&pError];

    for(int i=0; i < textures.count; i++)
    {
      gTextureMap.insert(std::make_pair([[[textureFiles[i] lastPathComponent] stringByDeletingPathExtension] cStringUsingEncoding:NSUTF8StringEncoding], textures[i]));
    }
    
    DBGMSG("Loaded %i textures\n", (int) textures.count);
    
    [textureLoader release];
  }
}

IGraphicsIOS::~IGraphicsIOS()
{
  CloseWindow();
}

void* IGraphicsIOS::OpenWindow(void* pParent)
{
  TRACE
  CloseWindow();
  IGraphicsIOS_View* view = (IGraphicsIOS_View*) [[IGraphicsIOS_View alloc] initWithIGraphics: this];
  mView = view;
  
  OnViewInitialized([view layer]);
  
  SetScreenScale([UIScreen mainScreen].scale);
  
  GetDelegate()->LayoutUI(this);
  GetDelegate()->OnUIOpen();

  if (pParent)
  {
    [(UIView*) pParent addSubview: view];
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
    [view removeFromSuperview];
    [view release];
    mView = nullptr;

    OnViewDestroyed();
  }
}

bool IGraphicsIOS::WindowIsOpen()
{
  return mView;
}

void IGraphicsIOS::PlatformResize(bool parentHasResized)
{
  if (mView)
  {
    //TODO
  }
}

EMsgBoxResult IGraphicsIOS::ShowMessageBox(const char* str, const char* caption, EMsgBoxType type, IMsgBoxCompletionHanderFunc completionHandler)
{
  ReleaseMouseCapture();
  [(IGraphicsIOS_View*) mView showMessageBox:str :caption :type :completionHandler];
  return EMsgBoxResult::kNoResult; // we need to rely on completionHandler
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

bool IGraphicsIOS::PromptForColor(IColor& color, const char* str, IColorPickerHandlerFunc func)
{
  return false;
}

IPopupMenu* IGraphicsIOS::CreatePlatformPopupMenu(IPopupMenu& menu, const IRECT& bounds)
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
  
  return pReturnMenu;
}

void IGraphicsIOS::CreatePlatformTextEntry(int paramIdx, const IText& text, const IRECT& bounds, int length, const char* str)
{
  ReleaseMouseCapture();
  CGRect areaRect = ToCGRect(this, bounds);
  [(IGraphicsIOS_View*) mView createTextEntry: paramIdx : text: str: length: areaRect];
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

bool IGraphicsIOS::SetTextInClipboard(const WDL_String& str)
{
  return false;
}

void IGraphicsIOS::CreatePlatformImGui()
{
#ifdef IGRAPHICS_IMGUI
  if(mView)
  {
    IGRAPHICS_VIEW* pView = (IGRAPHICS_VIEW*) mView;
    
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

void IGraphicsIOS::LaunchBluetoothMidiDialog(float x, float y)
{
  ReleaseMouseCapture();
  NSDictionary* dic = @{@"x": @(x), @"y": @(y)};
  [[NSNotificationCenter defaultCenter] postNotificationName:@"LaunchBTMidiDialog" object:nil userInfo:dic];
}

#if defined IGRAPHICS_NANOVG
  #include "IGraphicsNanoVG.cpp"
#elif defined IGRAPHICS_SKIA
  #include "IGraphicsSkia.cpp"
#else
  #error Either NO_IGRAPHICS or one and only one choice of graphics library must be defined!
#endif
