#ifndef NO_IGRAPHICS
#import <QuartzCore/QuartzCore.h>
#import "IGraphicsIOS_view.h"

#include "IGraphicsIOS.h"
#include "IControl.h"

#include "IPlugPluginDelegate.h"

NSString* ToNSString(const char* cStr)
{
  return [NSString stringWithCString:cStr encoding:NSUTF8StringEncoding];
}

#pragma clang diagnostic ignored "-Wdeprecated-declarations"

//int GetSystemVersion()
//{
//  static int32_t v;
//  if (!v)
//  {
//    if (NSAppKitVersionNumber >= 1266.0)
//    {
//      if (NSAppKitVersionNumber >= 1404.0)
//        v = 0x10b0;
//      else
//        v = 0x10a0; // 10.10+ Gestalt(gsv) return 0x109x, so we bump this to 0x10a0
//    }
//    else
//    {
//      SInt32 a = 0x1040;
//      Gestalt(gestaltSystemVersion,&a);
//      v=a;
//    }
//  }
//  return v;
//}

struct CocoaAutoReleasePool
{
//  NSAutoreleasePool* mPool;
//
//  CocoaAutoReleasePool()
//  {
//    mPool = [[NSAutoreleasePool alloc] init];
//  }
//
//  ~CocoaAutoReleasePool()
//  {
//    [mPool release];
//  }
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

IGraphicsIOS::IGraphicsIOS(IDelegate& dlg, int w, int h, int fps)
: IGraphicsNanoVG(dlg, w, h, fps)
, mView(nullptr)
{
  SetDisplayScale(1);
//  NSApplicationLoad();
}

IGraphicsIOS::~IGraphicsIOS()
{
  CloseWindow();
}

bool IGraphicsIOS::IsSandboxed()
{
  NSString* pHomeDir = NSHomeDirectory();
  
  if ([pHomeDir containsString:@"Library/Containers/"])
  {
    return true;
  }
  return false;
}

void IGraphicsIOS::CreateMetalLayer()
{
#ifdef IGRAPHICS_NANOVG
  mLayer = [CAMetalLayer new];
  ViewInitialized(mLayer);
#endif
}

bool IGraphicsIOS::GetResourcePathFromBundle(const char* fileName, const char* searchExt, WDL_String& fullPath)
{
  CocoaAutoReleasePool pool;
  
  const char* ext = fileName+strlen(fileName)-1;
  while (ext >= fileName && *ext != '.') --ext;
  ++ext;
  
  bool isCorrectType = !strcasecmp(ext, searchExt);
  
  NSBundle* pBundle = [NSBundle bundleWithIdentifier:ToNSString(GetBundleID())];
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

bool IGraphicsIOS::GetResourcePathFromUsersMusicFolder(const char* fileName, const char* searchExt, WDL_String& fullPath)
{
  CocoaAutoReleasePool pool;
  
  const char* ext = fileName+strlen(fileName)-1;
  while (ext >= fileName && *ext != '.') --ext;
  ++ext;
  
  bool isCorrectType = !strcasecmp(ext, searchExt);
  
  NSString* pFile = [[[NSString stringWithCString:fileName encoding:NSUTF8StringEncoding] lastPathComponent] stringByDeletingPathExtension];
  NSString* pExt = [NSString stringWithCString:searchExt encoding:NSUTF8StringEncoding];
  
  if (isCorrectType && pFile)
  {
    WDL_String musicFolder;
    SandboxSafeAppSupportPath(musicFolder);
    NSString* pPluginName = [NSString stringWithCString: dynamic_cast<IPluginDelegate&>(GetDelegate()).GetPluginName() encoding:NSUTF8StringEncoding];
    NSString* pMusicLocation = [NSString stringWithCString: musicFolder.Get() encoding:NSUTF8StringEncoding];
    NSString* pPath = [[[[pMusicLocation stringByAppendingPathComponent:pPluginName] stringByAppendingPathComponent:@"Resources"] stringByAppendingPathComponent: pFile] stringByAppendingPathExtension:pExt];
    
    if (pPath)
    {
      fullPath.Set([pPath UTF8String]);
      return true;
    }
  }
  
  fullPath.Set("");
  return false;
}

bool IGraphicsIOS::OSFindResource(const char* name, const char* type, WDL_String& result)
{
  if(CStringHasContents(name))
  {
    // first check this bundle
    if(GetResourcePathFromBundle(name, type, result))
      return true;
    
    // then check ~/Music/PLUG_NAME, which is a shared folder that can be accessed from app sandbox
    if(GetResourcePathFromUsersMusicFolder(name, type, result))
      return true;
    
    // finally check name, which might be a full path - if the plug-in is trying to load a resource at runtime (e.g. skinablle UI)
    NSString* pPath = [NSString stringWithCString:name encoding:NSUTF8StringEncoding];
    
    if([[NSFileManager defaultManager] fileExistsAtPath : pPath] == YES)
    {
      result.Set([pPath UTF8String]);
      return true;
    }
  }
  return false;
}

void* IGraphicsIOS::OpenWindow(void* pParent)
{
  TRACE;
  CloseWindow();
  mView = (IGraphicsIOS_View*) [[IGraphicsIOS_View alloc] initWithIGraphics: this];
  
  if (pParent) // Cocoa VST host.
  {
    [(UIView*) pParent addSubview: (IGraphicsIOS_View*) mView];
  }
  
  UpdateTooltips();
  
  return mView;
}

void IGraphicsIOS::CloseWindow()
{
  if (mView)
  {
    IGraphicsIOS_View* view = (IGraphicsIOS_View*) mView;
//    [view removeAllToolTips];
    [view killTimer];
    mView = nullptr;
    
    if (view->mGraphics)
    {
      [view removeFromSuperview];   // Releases.
    }
  }
}

bool IGraphicsIOS::WindowIsOpen()
{
  return mView;
}

void IGraphicsIOS::Resize(int w, int h, float scale)
{
  if (w == Width() && h == Height() && scale == GetScale()) return;
  
  IGraphics::Resize(w, h, scale);
  
  if (mView)
  {
//    CGSize size = { static_cast<CGFloat>(WindowWidth()), static_cast<CGFloat>(WindowHeight()) };
    
    // Prevent animation during resize
    // N.B. - The bounds perform scaling on the window, and so use the nominal size
    
//    [NSAnimationContext beginGrouping]; // Prevent animated resizing
//    [[NSAnimationContext currentContext] setDuration:0.0];
//    [(IGraphicsIOS_View*) mView setFrameSize: size ];
//    [(IGraphicsIOS_View*) mView setBoundsSize:CGSizeMake(Width(), Height())];
//    [NSAnimationContext endGrouping];
    
    SetAllControlsDirty();
  }
}

void IGraphicsIOS::HideMouseCursor(bool hide, bool returnToStartPosition)
{
}

void IGraphicsIOS::MoveMouseCursor(float x, float y)
{
}

void IGraphicsIOS::SetMousePosition(float x, float y)
{
}

int IGraphicsIOS::ShowMessageBox(const char* str, const char* caption, int type)
{
  return 0;
}

void IGraphicsIOS::ForceEndUserEdit()
{
  if (mView)
  {
    [(IGraphicsIOS_View*) mView endUserInput];
  }
}

void IGraphicsIOS::UpdateTooltips()
{
}

const char* IGraphicsIOS::GetPlatformAPIStr()
{
  return "iOS";
}

void IGraphicsIOS::HostPath(WDL_String& path)
{
}

void IGraphicsIOS::PluginPath(WDL_String& path)
{
}

void IGraphicsIOS::DesktopPath(WDL_String& path)
{
}

void IGraphicsIOS::UserHomePath(WDL_String& path)
{
}

void IGraphicsIOS::VST3PresetsPath(WDL_String& path, const char* mfrName, const char* pluginName, bool isSystem)
{
}

void IGraphicsIOS::AppSupportPath(WDL_String& path, bool isSystem)
{
  NSArray *pPaths;
  
  if (isSystem)
    pPaths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSSystemDomainMask, YES);
  else
    pPaths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
  
  NSString *pApplicationSupportDirectory = [pPaths objectAtIndex:0];
  path.Set([pApplicationSupportDirectory UTF8String]);
}

void IGraphicsIOS::SandboxSafeAppSupportPath(WDL_String& path)
{
  NSArray *pPaths = NSSearchPathForDirectoriesInDomains(NSMusicDirectory, NSUserDomainMask, YES);
  NSString *pUserMusicDirectory = [pPaths objectAtIndex:0];
  path.Set([pUserMusicDirectory UTF8String]);
}

bool IGraphicsIOS::RevealPathInExplorerOrFinder(WDL_String& path, bool select)
{
  return (bool) false;
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

IPopupMenu* IGraphicsIOS::CreatePopupMenu(IPopupMenu& menu, const IRECT& bounds, IControl* pCaller)
{
  return nullptr;
}

void IGraphicsIOS::CreateTextEntry(IControl& control, const IText& text, const IRECT& bounds, const char* str)
{
}

bool IGraphicsIOS::OpenURL(const char* url, const char* msgWindowTitle, const char* confirmMsg, const char* errMsgOnFailure)
{
  return false;
}

void* IGraphicsIOS::GetWindow()
{
  if (mView) return mView;
  else return 0;
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
