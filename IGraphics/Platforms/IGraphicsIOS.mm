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

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

void GetScreenDimensions(int& width, int& height)
{
  CGRect bounds = [[UIScreen mainScreen] bounds];
  width = bounds.size.width;
  height = bounds.size.height;
}

float GetScaleForScreen(int plugWidth, int plugHeight)
{
  int width, height;
  GetScreenDimensions(width, height);
  return std::min((float) width / (float) plugWidth, (float) height / (float) plugHeight);
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
 
#if defined IGRAPHICS_METAL && !defined IGRAPHICS_SKIA
  if(!gTextureMap.size())
  {
    NSBundle* pBundle = [NSBundle mainBundle];

    if(IsOOPAuv3AppExtension())
      pBundle = [NSBundle bundleWithPath: [[[pBundle bundlePath] stringByDeletingLastPathComponent] stringByDeletingLastPathComponent]];
    
    NSArray<NSURL*>* pTextureFiles = [pBundle URLsForResourcesWithExtension:@"ktx" subdirectory:@""];
    
    if ([pTextureFiles count])
    {
      MTKTextureLoader* textureLoader = [[MTKTextureLoader alloc] initWithDevice:MTLCreateSystemDefaultDevice()];
      
      NSError* pError = nil;
      NSDictionary* textureOptions = @{ MTKTextureLoaderOptionSRGB: [NSNumber numberWithBool:NO] };

      gTextures = [textureLoader newTexturesWithContentsOfURLs:pTextureFiles options:textureOptions error:&pError];
    
      for(int i=0; i < gTextures.count; i++)
      {
        gTextureMap.insert(std::make_pair([[[pTextureFiles[i] lastPathComponent] stringByDeletingPathExtension] cStringUsingEncoding:NSUTF8StringEncoding], (MTLTexturePtr) gTextures[i]));
      }
    
      DBGMSG("Preloaded %i textures\n", (int) [pTextureFiles count]);
    
      [textureLoader release];
      textureLoader = nil;
    }
  }
#endif
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
  mView = (void*) view;
  
  OnViewInitialized((void*) [view metalLayer]);
  
  SetScreenScale([UIScreen mainScreen].scale);
  
  GetDelegate()->LayoutUI(this);
  GetDelegate()->OnUIOpen();
  
  [view setMultipleTouchEnabled:MultiTouchEnabled()];

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
    
    IGRAPHICS_VIEW* pView = (IGRAPHICS_VIEW*) mView;
    [pView removeFromSuperview];
    [pView release];
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
    CGRect r = CGRectMake(0., 0., static_cast<CGFloat>(WindowWidth()), static_cast<CGFloat>(WindowHeight()));
    [(IGRAPHICS_VIEW*) mView setFrame: r ];
  }
}

void IGraphicsIOS::AttachPlatformView(const IRECT& r, void* pView)
{
  IGRAPHICS_VIEW* pMainView = (IGRAPHICS_VIEW*) mView;
  
  UIView* pNewSubView = (UIView*) pView;
  [pNewSubView setFrame:ToCGRect(this, r)];

  [pMainView addSubview:pNewSubView];
}

void IGraphicsIOS::RemovePlatformView(void* pView)
{
  [(UIView*) pView removeFromSuperview];
}

EMsgBoxResult IGraphicsIOS::ShowMessageBox(const char* str, const char* caption, EMsgBoxType type, IMsgBoxCompletionHanderFunc completionHandler)
{
  ReleaseMouseCapture();
  [(IGRAPHICS_VIEW*) mView showMessageBox:str :caption :type :completionHandler];
  return EMsgBoxResult::kNoResult; // we need to rely on completionHandler
}

void IGraphicsIOS::AttachGestureRecognizer(EGestureType type)
{
  IGraphics::AttachGestureRecognizer(type);
  [(IGRAPHICS_VIEW*) mView attachGestureRecognizer:type];
}

void IGraphicsIOS::ForceEndUserEdit()
{
  if (mView)
  {
    [(IGRAPHICS_VIEW*) mView endUserInput];
  }
}

const char* IGraphicsIOS::GetPlatformAPIStr()
{
  return "iOS";
}

void IGraphicsIOS::GetMouseLocation(float& x, float&y) const
{
  [(IGRAPHICS_VIEW*) mView getLastTouchLocation: x : y];
}

void IGraphicsIOS::PromptForFile(WDL_String& fileName, WDL_String& path, EFileAction action, const char* ext)
{
}

void IGraphicsIOS::PromptForDirectory(WDL_String& dir)
{
}

bool IGraphicsIOS::PromptForColor(IColor& color, const char* str, IColorPickerHandlerFunc func)
{
  [(IGRAPHICS_VIEW*) mView promptForColor: color: str: func];
  return false;
}

IPopupMenu* IGraphicsIOS::CreatePlatformPopupMenu(IPopupMenu& menu, const IRECT& bounds, bool& isAsync)
{
  IPopupMenu* pReturnMenu = nullptr;
  isAsync = true;
  if (mView)
  {
    CGRect areaRect = ToCGRect(this, bounds);
    pReturnMenu = [(IGRAPHICS_VIEW*) mView createPopupMenu: menu: areaRect];
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
  [(IGRAPHICS_VIEW*) mView createTextEntry: paramIdx : text: str: length: areaRect];
}

bool IGraphicsIOS::OpenURL(const char* url, const char* msgWindowTitle, const char* confirmMsg, const char* errMsgOnFailure)
{
  NSURL* pNSURL = nullptr;
  if (strstr(url, "http"))
    pNSURL = [NSURL URLWithString:[NSString stringWithCString:url encoding:NSUTF8StringEncoding]];
  else
    pNSURL = [NSURL fileURLWithPath:[NSString stringWithCString:url encoding:NSUTF8StringEncoding]];

  if (pNSURL)
  {
    UIResponder* pResponder = (UIResponder*) mView;
    while(pResponder) {
      if ([pResponder respondsToSelector: @selector(openURL:)])
        [pResponder performSelector: @selector(openURL:) withObject: pNSURL];

      pResponder = [pResponder nextResponder];
    }
    return true;
  }
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

bool IGraphicsIOS::SetTextInClipboard(const char* str)
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

PlatformFontPtr IGraphicsIOS::LoadPlatformFont(const char* fontID, void* pData, int dataSize)
{
  return CoreTextHelpers::LoadPlatformFont(fontID, pData, dataSize);
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

EUIAppearance IGraphicsIOS::GetUIAppearance() const
{
  IGRAPHICS_VIEW* pView = (IGRAPHICS_VIEW*) mView;
  
  if (pView)
  {
    return [[pView traitCollection] userInterfaceStyle] == UIUserInterfaceStyleDark ? EUIAppearance::Dark
                                                                                    : EUIAppearance::Light;
  }
  else
  {
    return EUIAppearance::Light;
  }
}

#if defined IGRAPHICS_NANOVG
  #include "IGraphicsNanoVG.cpp"
#elif defined IGRAPHICS_SKIA
  #include "IGraphicsSkia.cpp"
#else
  #error Either NO_IGRAPHICS or one and only one choice of graphics library must be defined!
#endif
