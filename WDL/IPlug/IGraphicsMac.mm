#include <Foundation/NSArchiver.h>
#include "IGraphicsMac.h"
#include "IControl.h"
#include "Log.h"
#import "IGraphicsCocoa.h"
#ifndef IPLUG_NO_CARBON_SUPPORT
  #include "IGraphicsCarbon.h"
#endif
#include "../swell/swell-internal.h"

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

struct CocoaAutoReleasePool
{
  NSAutoreleasePool* mPool;

  CocoaAutoReleasePool()
  {
    mPool = [[NSAutoreleasePool alloc] init];
  }

  ~CocoaAutoReleasePool()
  {
    [mPool release];
  }
};

//#define IGRAPHICS_MAC_BLIT_BENCHMARK
//#define IGRAPHICS_MAC_OLD_IMAGE_DRAWING

#ifdef IGRAPHICS_MAC_BLIT_BENCHMARK
#include <sys/time.h>
static double gettm()
{
  struct timeval tm={0,};
  gettimeofday(&tm,NULL);
  return (double)tm.tv_sec + (double)tm.tv_usec/1000000;
}
#endif

#ifndef __ppc__

#ifdef __SSE__
#include <xmmintrin.h>
#endif

static void SWELL_fastDoubleUpImage(unsigned int *op, const unsigned int *ip, int w, int h, int sw, int newspan)
{
  int y = h;
  while (y-->0)
  {
    const unsigned int *rd = ip;
    unsigned int *wr = op;
    int remaining = w;
    
#ifdef __SSE__
    if (remaining >= 4)
    {
      // with SSE is about 2x faster than without
      if (((INT_PTR)rd & 7))
      {
        // input isn't 8 byte aligned, must use unaligned reads
        int x = remaining/4;
        while (x-->0)
        {
          __m128 m =  _mm_loadu_ps((const float *)rd);
          __m128 p1 = _mm_shuffle_ps(m,m,_MM_SHUFFLE(1,1,0,0));
          __m128 p2 = _mm_shuffle_ps(m,m,_MM_SHUFFLE(3,3,2,2));
          
          unsigned int *wr2 = wr+newspan;
          rd+=4;
          
          _mm_store_ps((float*)wr,p1);
          _mm_store_ps((float*)wr2,p1);
          
          _mm_store_ps((float*)wr + 4,p2);
          _mm_store_ps((float*)wr2 + 4,p2);
          
          wr += 8;
        }
      }
      else
      {
        // if rd is 8 byte aligned, we can do SSE without unaligned reads
        
        // but if it is not 16 byte aligned, we need to preprocess a pair of pixels
        // (advancing rd by 8 bytes, and wr by 16)
        
        if ((INT_PTR)rd & 15)
        {
          unsigned int *nwr = wr+newspan;
          wr[0] = wr[1] = nwr[0] = nwr[1] = rd[0];
          wr[2] = wr[3] = nwr[2] = nwr[3] = rd[1];
          wr+=4;
          rd+=2;
          remaining-=2;
        }
        
        int x = remaining/4;
        while (x-->0)
        {
          __m128 m =  _mm_load_ps((const float *)rd);
          __m128 p1 = _mm_shuffle_ps(m,m,_MM_SHUFFLE(1,1,0,0));
          __m128 p2 = _mm_shuffle_ps(m,m,_MM_SHUFFLE(3,3,2,2));
          
          unsigned int *wr2 = wr+newspan;
          rd+=4;
          
          _mm_store_ps((float*)wr,p1);
          _mm_store_ps((float*)wr2,p1);
          
          _mm_store_ps((float*)wr + 4,p2);
          _mm_store_ps((float*)wr2 + 4,p2);
          
          wr += 8;
        }
      }
      remaining &= 3;
    }
#endif //__SSE__
    
    int x = remaining/2;
    while (x-->0)
    {
      unsigned int *nwr = wr+newspan;
      wr[0] = wr[1] = nwr[0] = nwr[1] = rd[0];
      wr[2] = wr[3] = nwr[2] = nwr[3] = rd[1];
      rd+=2;
      wr+=4;
    }
    if (remaining&1)
    {
      wr[0] = wr[1] = wr[newspan] = wr[newspan+1] = *rd;
    }
    ip += sw;
    op += newspan*2;
  }
}
#endif




@interface CUSTOM_COCOA_WINDOW : NSWindow {}
@end

@implementation CUSTOM_COCOA_WINDOW
- (BOOL)canBecomeKeyWindow {return YES;}
@end

IGraphicsMac::IGraphicsMac(IPlugBase* pPlug, int w, int h, int refreshFPS)
  :	IGraphics(pPlug, w, h, refreshFPS),
    #ifndef IPLUG_NO_CARBON_SUPPORT
    mGraphicsCarbon(0),
    #endif
    mGraphicsCocoa(0),
    mColorSpace(NULL)
{
  NSApplicationLoad();
}

IGraphicsMac::~IGraphicsMac()
{
  CloseWindow();
  if (mColorSpace)
  {
    CFRelease(mColorSpace);
    mColorSpace=0;
  }
}

LICE_IBitmap* LoadImgFromResourceOSX(const char* bundleID, const char* filename)
{
  if (!filename) return 0;
  CocoaAutoReleasePool pool;

  const char* ext = filename+strlen(filename)-1;
  while (ext >= filename && *ext != '.') --ext;
  ++ext;

  bool ispng = !stricmp(ext, "png");
  #ifndef IPLUG_JPEG_SUPPORT
  if (!ispng) return 0;
  #else
  bool isjpg = !stricmp(ext, "jpg");
  if (!isjpg && !ispng) return 0;
  #endif

  NSBundle* pBundle = [NSBundle bundleWithIdentifier:ToNSString(bundleID)];
  NSString* pFile = [[[NSString stringWithCString:filename] lastPathComponent] stringByDeletingPathExtension];
  
  if (pBundle && pFile)
  {
    NSString* pPath = 0;
    if (ispng) pPath = [pBundle pathForResource:pFile ofType:@"png"];
    #ifdef IPLUG_JPEG_SUPPORT
    if (isjpg) pPath = [pBundle pathForResource:pFile ofType:@"jpg"];
    #endif

    if (pPath)
    {
      const char* resourceFileName = [pPath cString];
      if (CSTR_NOT_EMPTY(resourceFileName))
      {
        if (ispng) return LICE_LoadPNG(resourceFileName);
        #ifdef IPLUG_JPEG_SUPPORT
        if (isjpg) return LICE_LoadJPG(resourceFileName);
        #endif
      }
    }
  }
  return 0;
}

LICE_IBitmap* IGraphicsMac::OSLoadBitmap(int ID, const char* name)
{
  return LoadImgFromResourceOSX(GetBundleID(), name);
}

bool IGraphicsMac::DrawScreen(IRECT* pR)
{
  CGContextRef pCGC = 0;
  CGRect r = CGRectMake(0, 0, Width(), Height());

  if (mGraphicsCocoa)
  {
    pCGC = (CGContextRef) [[NSGraphicsContext currentContext] graphicsPort];  // Leak?
    NSGraphicsContext* gc = [NSGraphicsContext graphicsContextWithGraphicsPort: pCGC flipped: YES];
    pCGC = (CGContextRef) [gc graphicsPort];
  }
  #ifndef IPLUG_NO_CARBON_SUPPORT
  else if (mGraphicsCarbon)
  {
    pCGC = mGraphicsCarbon->GetCGContext();
  }
  #endif
  if (!pCGC)
  {
    return false;
  }
  
  if (!mColorSpace)
  {
    SInt32 v=0x1040;
    Gestalt(gestaltSystemVersion,&v);
    if (v >= 0x1070)
    {
#ifdef MAC_OS_X_VERSION_10_11
      mColorSpace = CGDisplayCopyColorSpace(CGMainDisplayID());
#else
      CMProfileRef systemMonitorProfile = NULL;
      CMError getProfileErr = CMGetSystemProfile(&systemMonitorProfile);
      if(noErr == getProfileErr)
      {
        mColorSpace = CGColorSpaceCreateWithPlatformColorSpace(systemMonitorProfile);
        CMCloseProfile(systemMonitorProfile);
      }
#endif
    }
    if (!mColorSpace)
      mColorSpace = CGColorSpaceCreateDeviceRGB();
  }
  
#ifdef IGRAPHICS_MAC_BLIT_BENCHMARK
  double tm=gettm();
#endif
  
  CGImageRef img=NULL;
  
#ifdef IGRAPHICS_MAC_OLD_IMAGE_DRAWING
  HDC__ * srcCtx = (HDC__*) mDrawBitmap->getDC();
  img = CGBitmapContextCreateImage(srcCtx->ctx);
#else
  const unsigned char *p = (const unsigned char *)mDrawBitmap->getBits();
  const unsigned char *retina_buf = NULL;
  
  int sw = mDrawBitmap->getRowSpan();
  int h = mDrawBitmap->getHeight();
  int w = mDrawBitmap->getWidth();
#ifndef __ppc__
  if (CGContextConvertSizeToDeviceSpace(pCGC, CGSizeMake(1,1)).width > 1.9)
  {
    const int newspan = (w*2+3)&~3;
    const int newsz=sizeof(unsigned int) * newspan*h*2 + 32;
    mRetinaUpscaleBuf.Resize(newsz,false);
    if (mRetinaUpscaleBuf.GetSize()==newsz)
    {
      retina_buf = (unsigned char *)mRetinaUpscaleBuf.Get();
      const UINT_PTR align = (UINT_PTR)retina_buf & 31;
      if (align) retina_buf += 32-align;
      
      SWELL_fastDoubleUpImage((unsigned int *)retina_buf,
                              (const unsigned int *)p,w,h,sw,newspan);
      
      sw = newspan;
      w *= 2;
      h *= 2;
    }
  }
#endif
  
  
  CGDataProviderRef provider = CGDataProviderCreateWithData(NULL,retina_buf ? retina_buf : p,4*sw*h,NULL);
  img = CGImageCreate(w,h,8,32,4*sw,(CGColorSpaceRef)mColorSpace,
                                 kCGImageAlphaNoneSkipFirst,
                                 provider,NULL,NO,kCGRenderingIntentDefault);
  CGDataProviderRelease(provider);
#endif
  
  if (img)
  {
    CGContextDrawImage(pCGC, r, img);
    CGImageRelease(img);
  }
#ifdef IGRAPHICS_MAC_BLIT_BENCHMARK
  printf("blit %fms\n",(gettm()-tm)*1000.0);
#endif
  return true;
}

bool IGraphicsMac::MeasureIText(IText* pTxt, char* str, IRECT* pR)
{
  CocoaAutoReleasePool pool;

  return DrawIText(pTxt, str, pR, true);
}

void* IGraphicsMac::OpenWindow(void* pParent)
{
  return OpenCocoaWindow(pParent);
}

#ifndef IPLUG_NO_CARBON_SUPPORT
void* IGraphicsMac::OpenWindow(void* pWindow, void* pControl, short leftOffset, short topOffset)
{
  return OpenCarbonWindow(pWindow, pControl, leftOffset, topOffset);
}
#endif

void* IGraphicsMac::OpenCocoaWindow(void* pParentView)
{
  TRACE;
  CloseWindow();
  mGraphicsCocoa = (IGRAPHICS_COCOA*) [[IGRAPHICS_COCOA alloc] initWithIGraphics: this];
  
  if (pParentView) // Cocoa VST host.
  {
    [(NSView*) pParentView addSubview: (IGRAPHICS_COCOA*) mGraphicsCocoa];
  }
    
  UpdateTooltips();
  
  // Else we are being called by IGraphicsCocoaFactory, which is being called by a Cocoa AU host,
  // and the host will take care of attaching the view to the window.
  return mGraphicsCocoa;
}

#ifndef IPLUG_NO_CARBON_SUPPORT
void* IGraphicsMac::OpenCarbonWindow(void* pParentWnd, void* pParentControl, short leftOffset, short topOffset)
{
  TRACE;
  CloseWindow();
  WindowRef pWnd = (WindowRef) pParentWnd;
  ControlRef pControl = (ControlRef) pParentControl;
  mGraphicsCarbon = new IGraphicsCarbon(this, pWnd, pControl, leftOffset, topOffset);
  return mGraphicsCarbon->GetView();
}
#endif

void IGraphicsMac::AttachSubWindow(void* hostWindowRef)
{
  CocoaAutoReleasePool pool;

  NSWindow* hostWindow = [[NSWindow alloc] initWithWindowRef: hostWindowRef];
  [hostWindow retain];
  [hostWindow setCanHide: YES];
  [hostWindow setReleasedWhenClosed: YES];

  NSRect w = [hostWindow frame];

  int xOffset = 0;

  if (w.size.width > Width())
  {
    xOffset = (int) floor((w.size.width - Width()) / 2.);
  }

  NSRect windowRect = NSMakeRect(w.origin.x + xOffset, w.origin.y, Width(), Height());
  CUSTOM_COCOA_WINDOW *childWindow = [[CUSTOM_COCOA_WINDOW alloc] initWithContentRect:windowRect
                                                                            styleMask:( NSBorderlessWindowMask )
                                                                              backing:NSBackingStoreBuffered defer:NO];
  [childWindow retain];
  [childWindow setOpaque:YES];
  [childWindow setCanHide: YES];
  [childWindow setHasShadow: NO];
  [childWindow setReleasedWhenClosed: YES];

  NSView* childContent = [childWindow contentView];

  OpenWindow(childContent);

  [hostWindow addChildWindow: childWindow ordered: NSWindowAbove];
  [hostWindow orderFront: nil];
  [hostWindow display];
  [childWindow performSelector:@selector(orderFront:) withObject :(id) nil afterDelay :0.05];

  mHostNSWindow = (void*) hostWindow;
}

void IGraphicsMac::RemoveSubWindow()
{
  CocoaAutoReleasePool pool;

  NSWindow* hostWindow = (NSWindow*) mHostNSWindow;
  NSArray* childWindows = [hostWindow childWindows];
  NSWindow* childWindow = [childWindows objectAtIndex:0]; // todo: check it is allways the only child

  CloseWindow();

  [childWindow orderOut:nil];
  [hostWindow orderOut:nil];
  [childWindow close];
  [hostWindow removeChildWindow: childWindow];
  [hostWindow close];
}

void IGraphicsMac::CloseWindow()
{
  #ifndef IPLUG_NO_CARBON_SUPPORT
  if (mGraphicsCarbon)
  {
    DELETE_NULL(mGraphicsCarbon);
  }
  else
  #endif
  if (mGraphicsCocoa)
  {
    IGRAPHICS_COCOA* graphicscocoa = (IGRAPHICS_COCOA*)mGraphicsCocoa;
    [graphicscocoa removeAllToolTips];
    [graphicscocoa killTimer];
    mGraphicsCocoa = 0;

    if (graphicscocoa->mGraphics)
    {
      graphicscocoa->mGraphics = 0;
      [graphicscocoa removeFromSuperview];   // Releases.
    }
  }
}

bool IGraphicsMac::WindowIsOpen()
{
  #ifndef IPLUG_NO_CARBON_SUPPORT
  return (mGraphicsCarbon || mGraphicsCocoa);
  #else
  return mGraphicsCocoa;
  #endif
}

void IGraphicsMac::Resize(int w, int h)
{
  if (w == Width() && h == Height()) return;

  IGraphics::Resize(w, h);

  #ifndef IPLUG_NO_CARBON_SUPPORT
  if (mGraphicsCarbon)
  {
    mGraphicsCarbon->Resize(w, h);
  }
  else
  #endif
  if (mGraphicsCocoa)
  {
    NSSize size = { static_cast<CGFloat>(w), static_cast<CGFloat>(h) };
    [(IGRAPHICS_COCOA*) mGraphicsCocoa setFrameSize: size ];
  }
}

void IGraphicsMac::HideMouseCursor()
{
  if (!mCursorHidden)
  {
    if (CGDisplayHideCursor(CGMainDisplayID()) == CGDisplayNoErr) mCursorHidden = true;
    NSPoint mouse = [NSEvent mouseLocation];
    mHiddenMousePointX = mouse.x;
    mHiddenMousePointY = CGDisplayPixelsHigh(CGMainDisplayID())-mouse.y; //get current mouse position
  }
}

void IGraphicsMac::ShowMouseCursor()
{
  if (mCursorHidden)
  {
    CGPoint point; point.x = mHiddenMousePointX; point.y = mHiddenMousePointY;
    CGDisplayMoveCursorToPoint(CGMainDisplayID(), point);

    if (CGDisplayShowCursor(CGMainDisplayID()) == CGDisplayNoErr) mCursorHidden = false;
  }
}

int IGraphicsMac::ShowMessageBox(const char* pText, const char* pCaption, int type)
{
  int result = 0;

  CFStringRef defaultButtonTitle = NULL;
  CFStringRef alternateButtonTitle = NULL;
  CFStringRef otherButtonTitle = NULL;

  CFStringRef alertMessage = CFStringCreateWithCStringNoCopy(NULL, pText, 0, kCFAllocatorNull);
  CFStringRef alertHeader = CFStringCreateWithCStringNoCopy(NULL, pCaption, 0, kCFAllocatorNull);

  switch (type)
  {
    case MB_OKCANCEL:
      alternateButtonTitle = CFSTR("Cancel");
      break;
    case MB_YESNO:
      defaultButtonTitle = CFSTR("Yes");
      alternateButtonTitle = CFSTR("No");
      break;
    case MB_YESNOCANCEL:
      defaultButtonTitle = CFSTR("Yes");
      alternateButtonTitle = CFSTR("No");
      otherButtonTitle = CFSTR("Cancel");
      break;
  }

  CFOptionFlags response = 0;
  CFUserNotificationDisplayAlert(0, kCFUserNotificationNoteAlertLevel, NULL, NULL, NULL,
                                 alertHeader, alertMessage,
                                 defaultButtonTitle, alternateButtonTitle, otherButtonTitle,
                                 &response);

  CFRelease(alertMessage);
  CFRelease(alertHeader);

  switch (response) // TODO: check the return type, what about IDYES
  {
    case kCFUserNotificationDefaultResponse:
      result = IDOK;
      break;
    case kCFUserNotificationAlternateResponse:
      result = IDNO;
      break;
    case kCFUserNotificationOtherResponse:
      result = IDCANCEL;
      break;
  }

  return result;
}

void IGraphicsMac::ForceEndUserEdit()
{
  #ifndef IPLUG_NO_CARBON_SUPPORT
  if (mGraphicsCarbon)
  {
    mGraphicsCarbon->EndUserInput(false);
  }
  #endif
  if (mGraphicsCocoa)
  {
    [(IGRAPHICS_COCOA*) mGraphicsCocoa endUserInput];
  }
}

void IGraphicsMac::UpdateTooltips()
{
  if (!(mGraphicsCocoa && TooltipsEnabled())) return;

  CocoaAutoReleasePool pool;
  
  [(IGRAPHICS_COCOA*) mGraphicsCocoa removeAllToolTips];
  
  IControl** ppControl = mControls.GetList();
  
  for (int i = 0, n = mControls.GetSize(); i < n; ++i, ++ppControl) 
  {
    IControl* pControl = *ppControl;
    const char* tooltip = pControl->GetTooltip();
    if (tooltip && !pControl->IsHidden()) 
    {
      IRECT* pR = pControl->GetTargetRECT();
      if (!pControl->GetTargetRECT()->Empty()) 
      {
        [(IGRAPHICS_COCOA*) mGraphicsCocoa registerToolTip: pR];
      }
    }
  }
}

const char* IGraphicsMac::GetGUIAPI()
{
  #ifndef IPLUG_NO_CARBON_SUPPORT
  if (mGraphicsCarbon)
  {
    if (mGraphicsCarbon->GetIsComposited())
      return "Carbon Composited GUI";
    else
      return "Carbon Non-Composited GUI";
  }
  else
  #endif
    return "Cocoa GUI";
}

void IGraphicsMac::HostPath(WDL_String* pPath)
{
  CocoaAutoReleasePool pool;
  NSBundle* pBundle = [NSBundle bundleWithIdentifier: ToNSString(GetBundleID())];
  
  if (pBundle)
  {
    NSString* path = [pBundle executablePath];
    if (path)
    {
      pPath->Set([path UTF8String]);
    }
  }
}

void IGraphicsMac::PluginPath(WDL_String* pPath)
{
  CocoaAutoReleasePool pool;
  NSBundle* pBundle = [NSBundle bundleWithIdentifier: ToNSString(GetBundleID())];
  
  if (pBundle)
  {
    NSString* path = [[pBundle bundlePath] stringByDeletingLastPathComponent];
    
    if (path)
    {
      pPath->Set([path UTF8String]);
      pPath->Append("/");
    }
  }
}

void IGraphicsMac::DesktopPath(WDL_String* pPath)
{
  NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDesktopDirectory, NSUserDomainMask, YES);
  NSString *desktopDirectory = [paths objectAtIndex:0];
  pPath->Set([desktopDirectory UTF8String]);
}

//void IGraphicsMac::VST3PresetsPath(WDL_String* pPath, bool isSystem)
//{
//  NSArray *paths;
//  if (isSystem)
//    paths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSLocalDomainMask, YES);
//  else
//    paths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES);
//  
//  NSString *applicationSupportDirectory = [paths objectAtIndex:0];
//  pPath->SetFormatted(MAX_PATH, "%s/Audio/Presets/%s/%s/",
//                      [applicationSupportDirectory UTF8String],
//                      GetController()->GetMfrNameStr(),
//                      GetController()->GetPluginNameStr());
//}

void IGraphicsMac::AppSupportPath(WDL_String* pPath, bool isSystem)
{
  NSArray *paths;
  
  if (isSystem)
    paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSSystemDomainMask, YES);
  else
    paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
  
  NSString *applicationSupportDirectory = [paths objectAtIndex:0];
  pPath->Set([applicationSupportDirectory UTF8String]);
}

void IGraphicsMac::SandboxSafeAppSupportPath(WDL_String* pPath)
{
#if MAC_OS_X_VERSION_10_5 <= MAC_OS_X_VERSION_MAX_ALLOWED
  NSString *userHomeDir = NSHomeDirectory();
  pPath->Set([userHomeDir UTF8String]);
  pPath->Append("/Music");
#elif MAC_OS_X_VERSION_10_6 <= MAC_OS_X_VERSION_MAX_ALLOWED
  NSArray *paths = NSSearchPathForDirectoriesInDomains(NSMusicDirectory, NSUserDomainMask, YES);
  NSString *userMusicDirectory = [paths objectAtIndex:0];
  pPath->Set([userMusicDirectory UTF8String]);
#endif
}

// extensions = "txt wav" for example
void IGraphicsMac::PromptForFile(WDL_String* pFilename, EFileAction action, WDL_String* pDir, char* extensions)
{
  if (!WindowIsOpen())
  {
    pFilename->Set("");
    return;
  }

  NSString* defaultFileName;
  NSString* defaultPath;
  NSArray* fileTypes = nil;

  if (pFilename->GetLength())
  {
    defaultFileName = [NSString stringWithCString:pFilename->Get() encoding:NSUTF8StringEncoding];
  }
  else
  {
    defaultFileName = [NSString stringWithCString:"" encoding:NSUTF8StringEncoding];
  }

  if (pDir->GetLength())
  {
    defaultPath = [NSString stringWithCString:pDir->Get() encoding:NSUTF8StringEncoding];
  }
  else
  {
    defaultPath = [NSString stringWithCString:DEFAULT_PATH_OSX encoding:NSUTF8StringEncoding];
    pDir->Set(DEFAULT_PATH_OSX);
  }

  pFilename->Set(""); // reset it

  //if (CSTR_NOT_EMPTY(extensions))
  fileTypes = [[NSString stringWithUTF8String:extensions] componentsSeparatedByString: @" "];

  if (action == kFileSave)
  {
    NSSavePanel* panelSave = [NSSavePanel savePanel];

    //[panelOpen setTitle:title];
    [panelSave setAllowedFileTypes: fileTypes];
    [panelSave setAllowsOtherFileTypes: NO];

    int result = [panelSave runModalForDirectory:defaultPath file:defaultFileName];

    if (result == NSOKButton)
    {
      NSString* fullPath = [ panelSave filename ] ;
      pFilename->Set( [fullPath UTF8String] );

      NSString* truncatedPath = [fullPath stringByDeletingLastPathComponent];

      if (truncatedPath)
      {
        pDir->Set([truncatedPath UTF8String]);
        pDir->Append("/");
      }
    }
  }
  else
  {
    NSOpenPanel* panelOpen = [NSOpenPanel openPanel];

    //[panelOpen setTitle:title];
    //[panelOpen setAllowsMultipleSelection:(allowmul?YES:NO)];
    [panelOpen setCanChooseFiles:YES];
    [panelOpen setCanChooseDirectories:NO];
    [panelOpen setResolvesAliases:YES];

    int result = [panelOpen runModalForDirectory:defaultPath file:defaultFileName types:fileTypes];

    if (result == NSOKButton)
    {
      NSString* fullPath = [ panelOpen filename ] ;
      pFilename->Set( [fullPath UTF8String] );

      NSString* truncatedPath = [fullPath stringByDeletingLastPathComponent];

      if (truncatedPath)
      {
        pDir->Set([truncatedPath UTF8String]);
        pDir->Append("/");
      }
    }
  }

// dont know if you have to free these
// [defaultFileName release];
// [defaultPath release];
// [fileTypes release];
}

bool IGraphicsMac::PromptForColor(IColor* pColor, char* prompt)
{
//  NSColorPanel *colorPanel = [NSColorPanel sharedColorPanel];
//	[colorPanel setTarget:self]; // target??
//	[colorPanel setAction:@selector(colorPanelAction:)];
//	[NSApp orderFrontColorPanel:self];

  return false;
}

IPopupMenu* IGraphicsMac::CreateIPopupMenu(IPopupMenu* pMenu, IRECT* pTextRect)
{
  ReleaseMouseCapture();

  if (mGraphicsCocoa)
  {
    NSRect areaRect = ToNSRect(this, pTextRect);
    return [(IGRAPHICS_COCOA*) mGraphicsCocoa createIPopupMenu: pMenu: areaRect];
  }
  #ifndef IPLUG_NO_CARBON_SUPPORT
  else if (mGraphicsCarbon)
  {
    return mGraphicsCarbon->CreateIPopupMenu(pMenu, pTextRect);
  }
  #endif
  else return 0;
}

void IGraphicsMac::CreateTextEntry(IControl* pControl, IText* pText, IRECT* pTextRect, const char* pString, IParam* pParam)
{
  if (mGraphicsCocoa)
  {
    NSRect areaRect = ToNSRect(this, pTextRect);
    [(IGRAPHICS_COCOA*) mGraphicsCocoa createTextEntry: pControl: pParam: pText: pString: areaRect];
  }
  #ifndef IPLUG_NO_CARBON_SUPPORT
  else if (mGraphicsCarbon)
  {
    mGraphicsCarbon->CreateTextEntry(pControl, pText, pTextRect, pString, pParam);
  }
  #endif
}

bool IGraphicsMac::OpenURL(const char* url, const char* msgWindowTitle, const char* confirmMsg, const char* errMsgOnFailure)
{
  #pragma REMINDER("Warning and error messages for OpenURL not implemented")
  NSURL* pURL = 0;
  if (strstr(url, "http"))
  {
    pURL = [NSURL URLWithString:ToNSString(url)];
  }
  else
  {
    pURL = [NSURL fileURLWithPath:ToNSString(url)];
  }
  if (pURL)
  {
    bool ok = ([[NSWorkspace sharedWorkspace] openURL:pURL]);
    // [pURL release];
    return ok;
  }
  return true;
}

void* IGraphicsMac::GetWindow()
{
  if (mGraphicsCocoa) return mGraphicsCocoa;
  else return 0;
}

// static
int IGraphicsMac::GetUserOSVersion()   // Returns a number like 0x1050 (10.5).
{
  SInt32 ver = 0;
  Gestalt(gestaltSystemVersion, &ver);
  Trace(TRACELOC, "%x", ver);
  return ver;
}

bool IGraphicsMac::GetTextFromClipboard(WDL_String* pStr)
{
  NSString* text = [[NSPasteboard generalPasteboard] stringForType: NSStringPboardType];
  
  if (text == nil)
  {
    pStr->Set("");
    return false;
  }
  else
  {
    pStr->Set([text UTF8String]);
    return true;
  }
}

