/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#if !__has_feature(objc_arc)
#error This file must be compiled with Arc. Use -fobjc-arc flag
#endif

#import <QuartzCore/QuartzCore.h>
#import <MetalKit/MetalKit.h>

#include "IGraphicsIOS.h"
#include "IGraphicsCoreText.h"

#import "IGraphicsIOS_view.h"

#include <map>
#include <string>

#pragma clang diagnostic ignored "-Wdeprecated-declarations"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

void GetScreenDimensions(int& width, int& height)
{
  CGRect bounds = [[UIScreen mainScreen] bounds];
  width = bounds.size.width;
  height = bounds.size.height;
}

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE

using namespace iplug;
using namespace igraphics;

StaticStorage<CoreTextFontDescriptor> sFontDescriptorCache;

#pragma mark -

std::map<std::string, MTLTexturePtr> gTextureMap;
NSArray<id<MTLTexture>>* gTextures;

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

    gTextures = [textureLoader newTexturesWithContentsOfURLs:textureFiles options:textureOptions error:&pError];
  
    for(int i=0; i < gTextures.count; i++)
    {
      gTextureMap.insert(std::make_pair([[[textureFiles[i] lastPathComponent] stringByDeletingPathExtension] cStringUsingEncoding:NSUTF8StringEncoding], (__bridge void*) gTextures[i]));
    }
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
  IGRAPHICS_VIEW* view = [[IGRAPHICS_VIEW alloc] initWithIGraphics: this];
  mView = (__bridge void*) view;
  
  OnViewInitialized((__bridge void*) [view layer]);
  
  SetScreenScale([UIScreen mainScreen].scale);
  
  GetDelegate()->LayoutUI(this);
  GetDelegate()->OnUIOpen();

  if (pParent)
  {
    [(__bridge UIView*) pParent addSubview: view];
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
      mImGuiView = nullptr;
    }
#endif
    
    IGRAPHICS_VIEW* view = (__bridge IGRAPHICS_VIEW*)mView;
    [view removeFromSuperview];
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
  [(__bridge IGRAPHICS_VIEW*)mView showMessageBox:str :caption :type :completionHandler];
  return EMsgBoxResult::kNoResult; // we need to rely on completionHandler
}

void IGraphicsIOS::AttachGestureRecognizer(EGestureType type)
{
  IGraphics::AttachGestureRecognizer(type);
  [(__bridge IGRAPHICS_VIEW*)mView attachGestureRecognizer:type];
}

void IGraphicsIOS::ForceEndUserEdit()
{
  if (mView)
  {
    [(__bridge IGRAPHICS_VIEW*)mView endUserInput];
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

IPopupMenu* IGraphicsIOS::CreatePlatformPopupMenu(IPopupMenu& menu, const IRECT& bounds, bool& isAsync)
{
  IPopupMenu* pReturnMenu = nullptr;
  isAsync = true;
  if (mView)
  {
    CGRect areaRect = ToCGRect(this, bounds);
    pReturnMenu = [(__bridge IGRAPHICS_VIEW*) mView createPopupMenu: menu: areaRect];
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
  [(__bridge IGRAPHICS_VIEW*)mView createTextEntry: paramIdx : text: str: length: areaRect];
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
    IGRAPHICS_VIEW* pView = (__bridge IGRAPHICS_VIEW*)mView;
    
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
