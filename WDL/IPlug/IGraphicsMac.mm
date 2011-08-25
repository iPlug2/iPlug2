#include <Foundation/NSArchiver.h>
#include "IGraphicsMac.h"
#include "IControl.h"
#include "Log.h"
#import "IGraphicsCocoa.h"
#ifndef IPLUG_NO_CARBON_SUPPORT
	#include "IGraphicsCarbon.h"
#endif
#include "../swell/swell-internal.h"
//#include "Hosts.h"

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
	mGraphicsCocoa(0)
{
  NSApplicationLoad();
}

IGraphicsMac::~IGraphicsMac()
{
	CloseWindow();
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
  if (mGraphicsCocoa) {
    pCGC = (CGContextRef) [[NSGraphicsContext currentContext] graphicsPort];  // Leak?
    NSGraphicsContext* gc = [NSGraphicsContext graphicsContextWithGraphicsPort: pCGC flipped: YES];
    pCGC = (CGContextRef) [gc graphicsPort];    
  }
#ifndef IPLUG_NO_CARBON_SUPPORT
  else
  if (mGraphicsCarbon) {
    pCGC = mGraphicsCarbon->GetCGContext();
    mGraphicsCarbon->OffsetContentRect(&r);
    // Flipping is handled in IGraphicsCarbon.
  }
#endif
  if (!pCGC) {
    return false;
  }
  
  HDC__ * srcCtx = (HDC__*) mDrawBitmap->getDC();
  CGImageRef img = CGBitmapContextCreateImage(srcCtx->ctx); 
  CGContextDrawImage(pCGC, r, img);
  CGImageRelease(img);
  return true;
}

void* IGraphicsMac::OpenWindow(void* pParent)
{ 
  return OpenCocoaWindow(pParent);
}

#ifndef IPLUG_NO_CARBON_SUPPORT
void* IGraphicsMac::OpenWindow(void* pWindow, void* pControl)
{
  return OpenCarbonWindow(pWindow, pControl);
}
#endif

void* IGraphicsMac::OpenCocoaWindow(void* pParentView)
{
  TRACE;
  CloseWindow();
  mGraphicsCocoa = (IGRAPHICS_COCOA*) [[IGRAPHICS_COCOA alloc] initWithIGraphics: this];
  if (pParentView) {    // Cocoa VST host.
    [(NSView*) pParentView addSubview: (IGRAPHICS_COCOA*) mGraphicsCocoa];
  }
  // Else we are being called by IGraphicsCocoaFactory, which is being called by a Cocoa AU host, 
  // and the host will take care of attaching the view to the window. 
  return mGraphicsCocoa;
}

#ifndef IPLUG_NO_CARBON_SUPPORT
void* IGraphicsMac::OpenCarbonWindow(void* pParentWnd, void* pParentControl)
{
  TRACE;
  CloseWindow();
  WindowRef pWnd = (WindowRef) pParentWnd;
  ControlRef pControl = (ControlRef) pParentControl;
  // On 10.5 or later we could have used HICocoaViewCreate, but for 10.4 we have to support Carbon explicitly.
  mGraphicsCarbon = new IGraphicsCarbon(this, pWnd, pControl);
  return mGraphicsCarbon->GetView();
}
#endif

void IGraphicsMac::AttachSubWindow (void* hostWindowRef)
{
  CocoaAutoReleasePool pool;
  
  NSWindow* hostWindow = [[NSWindow alloc] initWithWindowRef: hostWindowRef];
  [hostWindow retain];
  [hostWindow setCanHide: YES];
  [hostWindow setReleasedWhenClosed: YES];
  
  NSRect w = [hostWindow frame];
  
  int xOffset = 0;
  
  if (w.size.width > Width()) {
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
  [childWindow orderFront: nil];  
  
  mHostNSWindow = (void*) hostWindow;
}

void IGraphicsMac::RemoveSubWindow ()
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
  IGraphics::Resize(w, h);
  if (mDrawBitmap) {
    mDrawBitmap->resize(w, h);
  } 
  
#ifndef IPLUG_NO_CARBON_SUPPORT
  if (mGraphicsCarbon) {
    mGraphicsCarbon->Resize(w, h);
  }
  else
#endif
  if (mGraphicsCocoa) {
    NSSize size = { w, h };
    [(IGRAPHICS_COCOA*) mGraphicsCocoa setFrameSize: size ];
  }  
}

void IGraphicsMac::ForceEndUserEdit()
{
#ifndef IPLUG_NO_CARBON_SUPPORT
  if (mGraphicsCarbon) {
    mGraphicsCarbon->EndUserInput(false);
  }
#endif
  if (mGraphicsCocoa) 
  {
    [(IGRAPHICS_COCOA*) mGraphicsCocoa endUserInput];
  }
}

const char* IGraphicsMac::GetGUIAPI()
{
#ifndef IPLUG_NO_CARBON_SUPPORT
  if (mGraphicsCarbon) {
    if (mGraphicsCarbon->GetIsComposited())
      return "Carbon Composited gui";
    else
      return "Carbon Non-Composited gui";
  }
  else
#endif
    return "Cocoa gui";
}

void IGraphicsMac::HostPath(WDL_String* pPath)
{
  CocoaAutoReleasePool pool;
  NSBundle* pBundle = [NSBundle bundleWithIdentifier: ToNSString(GetBundleID())];
  if (pBundle) {
    NSString* path = [pBundle executablePath];
    if (path) {
      pPath->Set([path UTF8String]);
    }
  }
}

void IGraphicsMac::PluginPath(WDL_String* pPath)
{
  CocoaAutoReleasePool pool;
  NSBundle* pBundle = [NSBundle bundleWithIdentifier: ToNSString(GetBundleID())];
  if (pBundle) {
    NSString* path = [[pBundle bundlePath] stringByDeletingLastPathComponent]; 
    if (path) {
      pPath->Set([path UTF8String]);
      pPath->Append("/");
    }
  }
}

// extensions = "txt wav" for example
void IGraphicsMac::PromptForFile(WDL_String* pFilename, EFileAction action, char* dir, char* extensions)
{
  if (!WindowIsOpen()) { 
    pFilename->Set("");
    return;
  }
  
  NSString* defaultFileName;
  NSString* defaultPath;
  NSArray* fileTypes = nil;

  if (pFilename->GetLength())
    defaultFileName = [NSString stringWithCString:pFilename->Get() encoding:NSUTF8StringEncoding];
  else {
    defaultFileName = [NSString stringWithCString:"" encoding:NSUTF8StringEncoding];
  }

  if (CSTR_NOT_EMPTY(dir))
    defaultPath = [NSString stringWithCString:dir encoding:NSUTF8StringEncoding];
  else 
    defaultPath = [NSString stringWithCString:DEFAULT_PATH_OSX encoding:NSUTF8StringEncoding];

  pFilename->Set(""); // reset it
  
  //if (CSTR_NOT_EMPTY(extensions))
  fileTypes = [[NSString stringWithUTF8String:extensions] componentsSeparatedByString: @" "];
  
  if (action == kFileSave) {
    NSSavePanel* panelSave = [NSSavePanel savePanel];
    
    //[panelOpen setTitle:title];
    [panelSave setAllowedFileTypes: fileTypes];
    [panelSave setAllowsOtherFileTypes: NO];
    
    int result = [panelSave runModalForDirectory:defaultPath file:defaultFileName];
    
    if (result == NSOKButton)
      pFilename->Set( [[ panelSave filename ] UTF8String] );
  }
  else {
    NSOpenPanel* panelOpen = [NSOpenPanel openPanel];
    
    //[panelOpen setTitle:title];
    //[panelOpen setAllowsMultipleSelection:(allowmul?YES:NO)];
    [panelOpen setCanChooseFiles:YES];
    [panelOpen setCanChooseDirectories:NO];
    [panelOpen setResolvesAliases:YES];
    
    int result = [panelOpen runModalForDirectory:defaultPath file:defaultFileName types:fileTypes];

    if (result == NSOKButton)
      pFilename->Set( [[ panelOpen filename ] UTF8String] );
  }
  
// dont know if you have to free these  
// [defaultFileName release];
// [defaultPath release];
// [fileTypes release];
}

bool IGraphicsMac::PromptForColor(IColor* pColor, char* prompt)
{
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
    //CGRect areaRect = ToCGRect(Height(), pTextRect);
//    return mGraphicsCarbon->CreateTextEntry(pMenu, pControl, pParam, pText, pString, areaRect);
    mGraphicsCarbon->CreateTextEntry(pControl, pText, pTextRect, pString, pParam);
	}
#endif
}

bool IGraphicsMac::OpenURL(const char* url,
  const char* msgWindowTitle, const char* confirmMsg, const char* errMsgOnFailure)
{
#pragma REMINDER("Warning and error messages for OpenURL not implemented")
  NSURL* pURL = 0;
  if (strstr(url, "http")) {
    pURL = [NSURL URLWithString:ToNSString(url)];
  }
  else {
    pURL = [NSURL fileURLWithPath:ToNSString(url)];
  }    
  if (pURL) {
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