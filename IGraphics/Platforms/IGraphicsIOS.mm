#ifndef NO_IGRAPHICS
#import <QuartzCore/QuartzCore.h>
#import "IGraphicsIOS_view.h"

#include "IGraphicsIOS.h"
#include "IControl.h"

#include "IPlugPluginBase.h"
#include "IPlugPaths.h"

NSString* ToNSString(const char* cStr)
{
  return [NSString stringWithCString:cStr encoding:NSUTF8StringEncoding];
}

#pragma clang diagnostic ignored "-Wdeprecated-declarations"

//struct CocoaAutoReleasePool
//{
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
//};

#pragma mark -

IGraphicsIOS::IGraphicsIOS(IEditorDelegate& dlg, int w, int h, int fps)
: IGraphicsNanoVG(dlg, w, h, fps)
{
  SetDisplayScale(1);
//  NSApplicationLoad();
}

IGraphicsIOS::~IGraphicsIOS()
{
  CloseWindow();
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
//  CocoaAutoReleasePool pool;
  
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
  //  CocoaAutoReleasePool pool; TODO:
  
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
    NSString* pPluginName = [NSString stringWithCString: dynamic_cast<IPluginBase&>(GetDelegate()).GetPluginName() encoding:NSUTF8StringEncoding];
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
      [view killTimer];
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
    //TODO
    
    SetAllControlsDirty();
  }
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
