#ifndef NO_IGRAPHICS
#include <Foundation/NSArchiver.h>
#ifdef IGRAPHICS_NANOVG
#import <QuartzCore/QuartzCore.h>
#endif

#include "IGraphicsMac.h"
#include "IControl.h"
#import "IGraphicsMac_view.h"

#include "swell.h"

#pragma clang diagnostic ignored "-Wdeprecated-declarations"

int GetSystemVersion()
{
  static int32_t v;
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

IGraphicsMac::IGraphicsMac(IDelegate& dlg, int w, int h, int fps)
  : IGRAPHICS_DRAW_CLASS(dlg, w, h, fps)
  , mView(nullptr)
{
  SetDisplayScale(1);
  NSApplicationLoad();
}

IGraphicsMac::~IGraphicsMac()
{
  CloseWindow();
}

bool IGraphicsMac::IsSandboxed()
{
  NSString* pHomeDir = NSHomeDirectory();
  //  NSString* pBundleIdentifier = [[NSBundle mainBundle] bundleIdentifier];

  if ([pHomeDir containsString:@"Library/Containers/"])
  {
    return true;
  }
  return false;
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

  bool isCorrectType = !strcasecmp(ext, searchExt);

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
  if(CStringHasContents(name))
  {
    if(IsSandboxed())
    {
      printf("SAND");
    }

    bool foundInBundle = GetResourcePathFromBundle(GetBundleID(), name, type, result);

    if(foundInBundle)
      return true;
    else
    {
      NSString* pPath = [NSString stringWithCString:name encoding:NSUTF8StringEncoding];

      if([[NSFileManager defaultManager] fileExistsAtPath : pPath] == YES)
      {
        result.Set(name);
        return true;
      }
    }

  }
  return false;
}

bool IGraphicsMac::MeasureText(const IText& text, const char* str, IRECT& bounds)
{
  CocoaAutoReleasePool pool;

  return IGRAPHICS_DRAW_CLASS::MeasureText(text, str, bounds);
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

void IGraphicsMac::Resize(int w, int h, float scale)
{
  if (w == Width() && h == Height() && scale == GetScale()) return;

  IGraphics::Resize(w, h, scale);

  if (mView)
  {
    NSSize size = { static_cast<CGFloat>(WindowWidth()), static_cast<CGFloat>(WindowHeight()) };

    // Prevent animation during resize
    // N.B. - The bounds perform scaling on the window, and so use the nominal size

    [NSAnimationContext beginGrouping]; // Prevent animated resizing
    [[NSAnimationContext currentContext] setDuration:0.0];
    [(IGRAPHICS_VIEW*) mView setFrameSize: size ];
    [(IGRAPHICS_VIEW*) mView setBoundsSize:NSMakeSize(Width(), Height())];
    [NSAnimationContext endGrouping];

    SetAllControlsDirty();
  }
}

void IGraphicsMac::HideMouseCursor(bool hide, bool returnToStartPosition)
{
  if(hide)
  {
    if (!mCursorHidden)
    {
      CGDisplayHideCursor(CGMainDisplayID());
      
      CGAssociateMouseAndMouseCursorPosition(false);
      
      if (returnToStartPosition)
      {
        NSPoint mouse = [NSEvent mouseLocation];
        mCursorX = mouse.x;
        mCursorY = CGDisplayPixelsHigh(CGMainDisplayID()) - mouse.y; // flipped
      }
      else
      {
        mCursorX = -1.f;
        mCursorY = -1.f;
      }
  
      mCursorHidden = true;
    }
  }
  else
  {
    if (mCursorHidden)
    {
      CGAssociateMouseAndMouseCursorPosition(true);
      
      if ((mCursorX + mCursorY) > 0.f)
      {
        CGPoint point;
        point.x = mCursorX;
        point.y = mCursorY;
        CGDisplayMoveCursorToPoint(CGMainDisplayID(), point);
        mCursorX = -1.f;
        mCursorY = -1.f;
      }

      CGDisplayShowCursor(CGMainDisplayID());
    }
    
    mCursorHidden = false;
  }
}

void IGraphicsMac::MoveMouseCursor(float x, float y)
{
  CGPoint point;
  NSPoint mouse = [NSEvent mouseLocation];
  double mouseY = CGDisplayPixelsHigh(CGMainDisplayID()) - mouse.y;
  point.x = x / GetDisplayScale() + (mouse.x - mCursorX / GetDisplayScale());
  point.y = y / GetDisplayScale() + (mouseY - mCursorY / GetDisplayScale());

  if (!mTabletInput && CGDisplayMoveCursorToPoint(CGMainDisplayID(), point) == CGDisplayNoErr)
  {
    mCursorX = x;
    mCursorY = y;
  }

  CGAssociateMouseAndMouseCursorPosition(true);
}

void IGraphicsMac::SetMousePosition(float x, float y)
{
//  mMouseX = x;
//  mMouseY = y;
}

int IGraphicsMac::ShowMessageBox(const char* str, const char* caption, int type)
{
  int result = 0;

  CFStringRef button1 = NULL;
  CFStringRef button2 = NULL;
  CFStringRef button3 = NULL;

  CFStringRef alertMessage = CFStringCreateWithCStringNoCopy(NULL, str, 0, kCFAllocatorNull);
  CFStringRef alertHeader = CFStringCreateWithCStringNoCopy(NULL, caption, 0, kCFAllocatorNull);

  switch (type)
  {
    case MB_OK:
      button1 = CFSTR("OK");
      break;
    case MB_OKCANCEL:
      button1 = CFSTR("OK");
      button2 = CFSTR("Cancel");
      break;
    case MB_YESNO:
      button1 = CFSTR("Yes");
      button2 = CFSTR("No");
      break;
    case MB_YESNOCANCEL:
      button1 = CFSTR("Yes");
      button2 = CFSTR("No");
      button3 = CFSTR("Cancel");
      break;
  }

  CFOptionFlags response = 0;
  CFUserNotificationDisplayAlert(0, kCFUserNotificationNoteAlertLevel, NULL, NULL, NULL, alertHeader, alertMessage, button1, button2, button3, &response);

  CFRelease(alertMessage);
  CFRelease(alertHeader);

  switch (response)
  {
    case kCFUserNotificationDefaultResponse:
      if(type == MB_OK || type == MB_OKCANCEL)
        result = IDOK;
      else
        result = IDYES;
      break;
    case kCFUserNotificationAlternateResponse:
      if(type == MB_OKCANCEL)
        result = IDCANCEL;
      else
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

const char* IGraphicsMac::GetPlatformAPIStr()
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

void IGraphicsMac::UserHomePath(WDL_String& path)
{
  NSString* pHomeDir = NSHomeDirectory();
  path.Set([pHomeDir UTF8String]);
}

void IGraphicsMac::VST3PresetsPath(WDL_String& path, const char* mfrName, const char* pluginName, bool isSystem)
{
  NSArray* pPaths;
  if (isSystem)
    pPaths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSLocalDomainMask, YES);
  else
    pPaths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES);

  NSString* pApplicationSupportDirectory = [pPaths objectAtIndex:0];
  path.SetFormatted(MAX_PATH, "%s/Audio/Presets/%s/%s/", [pApplicationSupportDirectory UTF8String], mfrName, pluginName);
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

  //if (CStringHasContents(ext))
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

void IGraphicsMac::PromptForDirectory(WDL_String& dir)
{
  NSString* defaultPath;
  
  if (dir.GetLength())
  {
    defaultPath = [NSString stringWithCString:dir.Get() encoding:NSUTF8StringEncoding];
  }
  else
  {
    defaultPath = [NSString stringWithCString:DEFAULT_PATH encoding:NSUTF8StringEncoding];
    dir.Set(DEFAULT_PATH);
  }
  
  NSOpenPanel* panelOpen = [NSOpenPanel openPanel];
  
  [panelOpen setTitle:@"Choose a Directory"];
  [panelOpen setCanChooseFiles:NO];
  [panelOpen setCanChooseDirectories:YES];
  [panelOpen setResolvesAliases:YES];
  [panelOpen setCanCreateDirectories:YES];
  
  [panelOpen setDirectoryURL: [NSURL fileURLWithPath: defaultPath]];
  
  if ([panelOpen runModal] == NSOKButton)
  {
    NSString* fullPath = [ panelOpen filename ] ;
    dir.Set( [fullPath UTF8String] );
    dir.Append("/");
  }
  else
  {
    dir.Set("");
  }
}

bool IGraphicsMac::PromptForColor(IColor& color, const char* str)
{
  //TODO:
  return false;
}

IPopupMenu* IGraphicsMac::CreatePopupMenu(const IPopupMenu& menu, const IRECT& bounds)
{
  ReleaseMouseCapture();

  if (mView)
  {
    NSRect areaRect = ToNSRect(this, bounds);
    return [(IGRAPHICS_VIEW*) mView createPopupMenu: menu: areaRect];
  }
  else return 0;
}

void IGraphicsMac::CreateTextEntry(IControl& control, const IText& text, const IRECT& bounds, const char* str)
{
  if (mView)
  {
    NSRect areaRect = ToNSRect(this, bounds);
    [(IGRAPHICS_VIEW*) mView createTextEntry: control: text: str: areaRect];
  }
}

bool IGraphicsMac::OpenURL(const char* url, const char* msgWindowTitle, const char* confirmMsg, const char* errMsgOnFailure)
{
  #pragma REMINDER("Warning and error messages for OpenURL not implemented")
  NSURL* pNSURL = nullptr;
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
  return (int) GetSystemVersion();
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
#ifdef IGRAPHICS_AGG
#include "IGraphicsAGG.cpp"
#include "agg_mac_pmap.mm"
#include "agg_mac_font.mm"
#elif defined IGRAPHICS_CAIRO
#include "IGraphicsCairo.cpp"
#elif defined IGRAPHICS_NANOVG
#include "IGraphicsNanoVG.cpp"
#include "nanovg.c"
//#include "nanovg_mtl.m"
#else
#include "IGraphicsLice.cpp"
#endif

#endif// NO_IGRAPHICS
