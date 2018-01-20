#include <Foundation/NSArchiver.h>
#ifdef IGRAPHICS_NANOVG
#import <QuartzCore/QuartzCore.h>
#endif

#include "IGraphicsMac.h"
#import "IGraphicsView.h"
#include "IControl.h"

#include "swell.h"

#include "Log.h"

#pragma clang diagnostic ignored "-Wdeprecated-declarations"

int GetSystemVersion() 
{
  static SInt32 v;
  if (!v)
  {
    if (NSAppKitVersionNumber >= 1266.0) 
    {
      if (NSAppKitVersionNumber >= 1404.0)
        v = 0x10b0;
      else
        v = 0x10a0; // 10.10+ Gestalt(gsv) return 0x109x, so we bump this to 0x10a0
    }
    else 
    {
      SInt32 a = 0x1040;
      Gestalt(gestaltSystemVersion,&a);
      v=a;
    }
  }
  return v;
}

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

#pragma mark -

IGraphicsMac::IGraphicsMac(IPlugBaseGraphics& plug, int w, int h, int fps)
  : IGRAPHICS_DRAW_CLASS(plug, w, h, fps)
  , mView(nullptr)
{
  NSApplicationLoad();
}

IGraphicsMac::~IGraphicsMac()
{
  CloseWindow();
}

void IGraphicsMac::CreateMetalLayer()
{
#ifdef IGRAPHICS_NANOVG
  mLayer = [CAMetalLayer new];
  ViewInitialized(mLayer);
#endif
}

bool GetResourcePathFromBundle(const char* bundleID, const char* fileName, const char* searchExt, WDL_String& fullPath)
{
  CocoaAutoReleasePool pool;

  const char* ext = fileName+strlen(fileName)-1;
  while (ext >= fileName && *ext != '.') --ext;
  ++ext;

  bool isCorrectType = !stricmp(ext, searchExt);

  NSBundle* pBundle = [NSBundle bundleWithIdentifier:ToNSString(bundleID)];
  NSString* pFile = [[[NSString stringWithCString:fileName encoding:NSUTF8StringEncoding] lastPathComponent] stringByDeletingPathExtension];
  
  if (isCorrectType && pBundle && pFile)
  {
    NSString* pPath = [pBundle pathForResource:pFile ofType:ToNSString(searchExt)];
    
    if (pPath)
    {
      fullPath.Set([pPath cString]);
      return true;
    }
  }

  fullPath.Set("");
  return false;
}

bool IGraphicsMac::OSFindResource(const char* name, const char* type, WDL_String& result)
{
  return GetResourcePathFromBundle(GetBundleID(), name, type, result);
}

bool IGraphicsMac::MeasureIText(const IText& text, const char* str, IRECT& destRect)
{
  CocoaAutoReleasePool pool;

  return IGRAPHICS_DRAW_CLASS::MeasureIText(text, str, destRect);
}

void* IGraphicsMac::OpenWindow(void* pParent)
{
  TRACE;
  CloseWindow();
  mView = (IGRAPHICS_VIEW*) [[IGRAPHICS_VIEW alloc] initWithIGraphics: this];
  
  if (pParent) // Cocoa VST host.
  {
    [(NSView*) pParent addSubview: (IGRAPHICS_VIEW*) mView];
  }
  
  UpdateTooltips();
  
  return mView;
}

void IGraphicsMac::CloseWindow()
{
  if (mView)
  {
    IGRAPHICS_VIEW* view = (IGRAPHICS_VIEW*) mView;
    [view removeAllToolTips];
    [view killTimer];
    mView = nullptr;

    if (view->mGraphics)
    {
      [view removeFromSuperview];   // Releases.
    }
  }
}

bool IGraphicsMac::WindowIsOpen()
{
  return mView;
}

void IGraphicsMac::Resize(int w, int h, double scale)
{
  if (w == Width() && h == Height() && scale == Scale()) return;

  IGraphics::Resize(w, h, scale);

  if (mView)
  {
    NSSize size = { static_cast<CGFloat>(w * scale), static_cast<CGFloat>(h * scale) };
    [(IGRAPHICS_VIEW*) mView setFrameSize: size ];
    [(IGRAPHICS_VIEW*) mView setBoundsSize:NSMakeSize(w, h)];
  }
}

void IGraphicsMac::HideMouseCursor()
{
  if (!mCursorHidden)
  {
    if (CGDisplayHideCursor(CGMainDisplayID()) == CGDisplayNoErr) mCursorHidden = true;
    if (!mTabletInput) CGAssociateMouseAndMouseCursorPosition(false);
  }
}

void IGraphicsMac::ShowMouseCursor()
{
  if (mCursorHidden)
  {
    if (CGDisplayShowCursor(CGMainDisplayID()) == CGDisplayNoErr) mCursorHidden = false;
    CGAssociateMouseAndMouseCursorPosition(true);
  }
}

int IGraphicsMac::ShowMessageBox(const char* str, const char* pCaption, int type)
{
  int result = 0;

  CFStringRef defaultButtonTitle = NULL;
  CFStringRef alternateButtonTitle = NULL;
  CFStringRef otherButtonTitle = NULL;

  CFStringRef alertMessage = CFStringCreateWithCStringNoCopy(NULL, str, 0, kCFAllocatorNull);
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
  if (mView)
  {
    [(IGRAPHICS_VIEW*) mView endUserInput];
  }
}

void IGraphicsMac::UpdateTooltips()
{
  if (!(mView && TooltipsEnabled())) return;

  CocoaAutoReleasePool pool;
  
  [(IGRAPHICS_VIEW*) mView removeAllToolTips];
  
  IControl** ppControl = mControls.GetList();
  
  for (int i = 0, n = mControls.GetSize(); i < n; ++i, ++ppControl) 
  {
    IControl* pControl = *ppControl;
    const char* tooltip = pControl->GetTooltip();
    if (tooltip && !pControl->IsHidden()) 
    {
      IRECT pR = pControl->GetTargetRECT();
      if (!pControl->GetTargetRECT().Empty())
      {
        [(IGRAPHICS_VIEW*) mView registerToolTip: pR];
      }
    }
  }
}

const char* IGraphicsMac::GetGUIAPI()
{
  return "Cocoa";
}

void IGraphicsMac::HostPath(WDL_String& path)
{
  CocoaAutoReleasePool pool;
  NSBundle* pBundle = [NSBundle bundleWithIdentifier: ToNSString(GetBundleID())];
  
  if (pBundle)
  {
    NSString* pPath = [pBundle executablePath];
    if (pPath)
    {
      path.Set([pPath UTF8String]);
    }
  }
}

void IGraphicsMac::PluginPath(WDL_String& path)
{
  CocoaAutoReleasePool pool;
  NSBundle* pBundle = [NSBundle bundleWithIdentifier: ToNSString(GetBundleID())];
  
  if (pBundle)
  {
    NSString* pPath = [[pBundle bundlePath] stringByDeletingLastPathComponent];
    
    if (pPath)
    {
      path.Set([pPath UTF8String]);
      path.Append("/");
    }
  }
}

void IGraphicsMac::DesktopPath(WDL_String& path)
{
  NSArray* pPaths = NSSearchPathForDirectoriesInDomains(NSDesktopDirectory, NSUserDomainMask, YES);
  NSString* pDesktopDirectory = [pPaths objectAtIndex:0];
  path.Set([pDesktopDirectory UTF8String]);
}

void IGraphicsMac::VST3PresetsPath(WDL_String& path, bool isSystem)
{
  NSArray* pPaths;
  if (isSystem)
    pPaths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSLocalDomainMask, YES);
  else
    pPaths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES);
  
  NSString* pApplicationSupportDirectory = [pPaths objectAtIndex:0];
  path.SetFormatted(MAX_PATH, "%s/Audio/Presets/%s/%s/", [pApplicationSupportDirectory UTF8String], mPlug.GetMfrName(), mPlug.GetEffectName());
}

void IGraphicsMac::AppSupportPath(WDL_String& path, bool isSystem)
{
  NSArray *pPaths;
  
  if (isSystem)
    pPaths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSSystemDomainMask, YES);
  else
    pPaths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
  
  NSString *pApplicationSupportDirectory = [pPaths objectAtIndex:0];
  path.Set([pApplicationSupportDirectory UTF8String]);
}

void IGraphicsMac::SandboxSafeAppSupportPath(WDL_String& path)
{
  NSArray *pPaths = NSSearchPathForDirectoriesInDomains(NSMusicDirectory, NSUserDomainMask, YES);
  NSString *pUserMusicDirectory = [pPaths objectAtIndex:0];
  path.Set([pUserMusicDirectory UTF8String]);
}

bool IGraphicsMac::RevealPathInExplorerOrFinder(WDL_String& path, bool select)
{
  CocoaAutoReleasePool pool;
  
  BOOL success = FALSE;
  
  if(path.GetLength())
  {
    NSString* pPath = [NSString stringWithCString:path.Get() encoding:NSUTF8StringEncoding];
    
    if([[NSFileManager defaultManager] fileExistsAtPath : pPath] == YES)
    {
      if (select)
      {
        NSString* pParentDirectoryPath = [pPath stringByDeletingLastPathComponent];
        
        if (pParentDirectoryPath)
        {
          success = [[NSWorkspace sharedWorkspace] openFile:pParentDirectoryPath];
          
          if (success)
            success = [[NSWorkspace sharedWorkspace] selectFile: pPath inFileViewerRootedAtPath:pParentDirectoryPath];
        }
      }
      else {
        success = [[NSWorkspace sharedWorkspace] openFile:pPath];
      }
      
    }
  }
  
  return (bool) success;
}

void IGraphicsMac::PromptForFile(WDL_String& fileName, WDL_String& path, EFileAction action, const char* ext)
{
  if (!WindowIsOpen())
  {
    fileName.Set("");
    return;
  }

  NSString* pDefaultFileName;
  NSString* pDefaultPath;
  NSArray* pFileTypes = nil;

  if (fileName.GetLength())
    pDefaultFileName = [NSString stringWithCString:fileName.Get() encoding:NSUTF8StringEncoding];
  else
    pDefaultFileName = [NSString stringWithCString:"" encoding:NSUTF8StringEncoding];

  if(!path.GetLength())
    DesktopPath(path);

  pDefaultPath = [NSString stringWithCString:path.Get() encoding:NSUTF8StringEncoding];

  fileName.Set(""); // reset it

  //if (CSTR_NOT_EMPTY(ext))
  pFileTypes = [[NSString stringWithUTF8String:ext] componentsSeparatedByString: @" "];

  if (action == kFileSave)
  {
    NSSavePanel* pSavePanel = [NSSavePanel savePanel];

    //[panelOpen setTitle:title];
    [pSavePanel setAllowedFileTypes: pFileTypes];
    [pSavePanel setAllowsOtherFileTypes: NO];

    long result = [pSavePanel runModalForDirectory:pDefaultPath file:pDefaultFileName];

    if (result == NSOKButton)
    {
      NSString* pFullPath = [pSavePanel filename] ;
      fileName.Set([pFullPath UTF8String]);

      NSString* pTruncatedPath = [pFullPath stringByDeletingLastPathComponent];

      if (pTruncatedPath)
      {
        path.Set([pTruncatedPath UTF8String]);
        path.Append("/");
      }
    }
  }
  else
  {
    NSOpenPanel* pOpenPanel = [NSOpenPanel openPanel];

    //[pOpenPanel setTitle:title];
    //[pOpenPanel setAllowsMultipleSelection:(allowmul?YES:NO)];
    [pOpenPanel setCanChooseFiles:YES];
    [pOpenPanel setCanChooseDirectories:NO];
    [pOpenPanel setResolvesAliases:YES];

    long result = [pOpenPanel runModalForDirectory:pDefaultPath file:pDefaultFileName types:pFileTypes];

    if (result == NSOKButton)
    {
      NSString* pFullPath = [pOpenPanel filename] ;
      fileName.Set([pFullPath UTF8String]);

      NSString* pTruncatedPath = [pFullPath stringByDeletingLastPathComponent];

      if (pTruncatedPath)
      {
        path.Set([pTruncatedPath UTF8String]);
        path.Append("/");
      }
    }
  }
}

bool IGraphicsMac::PromptForColor(IColor& color, const char* str)
{
  //TODO:
  return false;
}

IPopupMenu* IGraphicsMac::CreateIPopupMenu(IPopupMenu& menu, IRECT& textRect)
{
  ReleaseMouseCapture();

  if (mView)
  {
    NSRect areaRect = ToNSRect(this, textRect);
    return [(IGRAPHICS_VIEW*) mView createIPopupMenu: menu: areaRect];
  }
  else return 0;
}

void IGraphicsMac::CreateTextEntry(IControl* pControl, const IText& text, const IRECT& textRect, const char* str, IParam* pParam)
{
  if (mView)
  {
    NSRect areaRect = ToNSRect(this, textRect);
    [(IGRAPHICS_VIEW*) mView createTextEntry: pControl: pParam: text: str: areaRect];
  }
}

bool IGraphicsMac::OpenURL(const char* url, const char* msgWindowTitle, const char* confirmMsg, const char* errMsgOnFailure)
{
  #pragma REMINDER("Warning and error messages for OpenURL not implemented")
  NSURL* pNSURL = 0;
  if (strstr(url, "http"))
  {
    pNSURL = [NSURL URLWithString:ToNSString(url)];
  }
  else
  {
    pNSURL = [NSURL fileURLWithPath:ToNSString(url)];
  }
  if (pNSURL)
  {
    bool ok = ([[NSWorkspace sharedWorkspace] openURL:pNSURL]);
    // [pURL release];
    return ok;
  }
  return true;
}

void* IGraphicsMac::GetWindow()
{
  if (mView) return mView;
  else return 0;
}

// static
int IGraphicsMac::GetUserOSVersion()   // Returns a number like 0x1050 (10.5).
{
  SInt32 ver = GetSystemVersion();
  
  Trace(TRACELOC, "%x", ver);
  return (int) ver;
}

bool IGraphicsMac::GetTextFromClipboard(WDL_String& str)
{
  NSString* pTextOnClipboard = [[NSPasteboard generalPasteboard] stringForType: NSStringPboardType];
  
  if (pTextOnClipboard == nil)
  {
    str.Set("");
    return false;
  }
  else
  {
    str.Set([pTextOnClipboard UTF8String]);
    return true;
  }
}

//TODO: THIS IS TEMPORARY, TO EASE DEVELOPMENT
#ifndef NO_IGRAPHICS
#ifdef IGRAPHICS_AGG
#include "IGraphicsAGG.cpp"
#elif defined IGRAPHICS_CAIRO
#include "IGraphicsCairo.cpp"
#elif defined IGRAPHICS_NANOVG
#include "IGraphicsNanoVG.cpp"
#include "nanovg.c"
//#include "nanovg_mtl.m"
#else
#include "IGraphicsLice.cpp"
#endif
#endif
