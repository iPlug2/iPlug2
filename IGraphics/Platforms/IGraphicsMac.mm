/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include "IGraphicsMac.h"
#import "IGraphicsMac_view.h"

#include "IControl.h"
#include "IPopupMenuControl.h"

#pragma clang diagnostic ignored "-Wdeprecated-declarations"

using namespace iplug;
using namespace igraphics;

static int GetSystemVersion()
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

StaticStorage<CoreTextFontDescriptor> sFontDescriptorCache;

#pragma mark -

IGraphicsMac::IGraphicsMac(IGEditorDelegate& dlg, int w, int h, int fps, float scale)
: IGRAPHICS_DRAW_CLASS(dlg, w, h, fps, scale)
{
  NSApplicationLoad();
  StaticStorage<CoreTextFontDescriptor>::Accessor storage(sFontDescriptorCache);
  storage.Retain();
}

IGraphicsMac::~IGraphicsMac()
{
  StaticStorage<CoreTextFontDescriptor>::Accessor storage(sFontDescriptorCache);
  storage.Release();
  
  CloseWindow();
}

PlatformFontPtr IGraphicsMac::LoadPlatformFont(const char* fontID, const char* fileNameOrResID)
{
  return CoreTextHelpers::LoadPlatformFont(fontID, fileNameOrResID, GetBundleID(), GetSharedResourcesSubPath());
}

PlatformFontPtr IGraphicsMac::LoadPlatformFont(const char* fontID, const char* fontName, ETextStyle style)
{
  return CoreTextHelpers::LoadPlatformFont(fontID, fontName, style);
}

PlatformFontPtr IGraphicsMac::LoadPlatformFont(const char* fontID, void* pData, int dataSize)
{
  return CoreTextHelpers::LoadPlatformFont(fontID, pData, dataSize);
}

void IGraphicsMac::CachePlatformFont(const char* fontID, const PlatformFontPtr& font)
{
  CoreTextHelpers::CachePlatformFont(fontID, font, sFontDescriptorCache);
}

float IGraphicsMac::MeasureText(const IText& text, const char* str, IRECT& bounds) const
{
  return IGRAPHICS_DRAW_CLASS::MeasureText(text, str, bounds);
}

void* IGraphicsMac::OpenWindow(void* pParent)
{
  TRACE
  CloseWindow();
  IGRAPHICS_VIEW* pView = [[IGRAPHICS_VIEW alloc] initWithIGraphics: this];
  mView = (void*) pView;
    
#ifdef IGRAPHICS_GL
  [[pView openGLContext] makeCurrentContext];
#endif
    
  OnViewInitialized([pView layer]);
  SetScreenScale([[NSScreen mainScreen] backingScaleFactor]);
  GetDelegate()->LayoutUI(this);
  UpdateTooltips();
  GetDelegate()->OnUIOpen();
  
  if (pParent)
  {
    [(NSView*) pParent addSubview: pView];
  }

  return mView;
}

void IGraphicsMac::AttachPlatformView(const IRECT& r, void* pView)
{
  NSView* pNewSubView = (NSView*) pView;
  [pNewSubView setFrame:ToNSRect(this, r)];
  
  [(IGRAPHICS_VIEW*) mView addSubview:(NSView*) pNewSubView];
}

void IGraphicsMac::RemovePlatformView(void* pView)
{
  [(NSView*) pView removeFromSuperview];
}

void IGraphicsMac::CloseWindow()
{
  if (mView)
  {    
    IGRAPHICS_VIEW* pView = (IGRAPHICS_VIEW*) mView;
      
#ifdef IGRAPHICS_GL
    [[pView openGLContext] makeCurrentContext];
#endif
      
    [pView removeAllToolTips];
    [pView killTimer];
    [pView removeFromSuperview];
    [pView release];
      
    mView = nullptr;
    OnViewDestroyed();
  }
}

bool IGraphicsMac::WindowIsOpen()
{
  return mView;
}

void IGraphicsMac::PlatformResize(bool parentHasResized)
{
  if (mView)
  {
    NSSize size = { static_cast<CGFloat>(WindowWidth()), static_cast<CGFloat>(WindowHeight()) };

    [NSAnimationContext beginGrouping]; // Prevent animated resizing
    [[NSAnimationContext currentContext] setDuration:0.0];
    [(IGRAPHICS_VIEW*) mView setFrameSize: size ];
    
    [NSAnimationContext endGrouping];
  }
    
  UpdateTooltips();
}

void IGraphicsMac::PointToScreen(float& x, float& y) const
{
  if (mView)
  {
    x *= GetDrawScale();
    y *= GetDrawScale();
    NSWindow* pWindow = [(IGRAPHICS_VIEW*) mView window];
    NSPoint wndpt = [(IGRAPHICS_VIEW*) mView convertPoint:NSMakePoint(x, y) toView:nil];
    NSPoint pt = [pWindow convertRectToScreen: NSMakeRect(wndpt.x, wndpt.y, 0.0, 0.0)].origin;
      
    x = pt.x;
    y = pt.y;
  }
}

void IGraphicsMac::ScreenToPoint(float& x, float& y) const
{
  if (mView)
  {
    NSWindow* pWindow = [(IGRAPHICS_VIEW*) mView window];
    NSPoint wndpt = [pWindow convertRectFromScreen: NSMakeRect(x, y, 0.0, 0.0)].origin;
    NSPoint pt = [(IGRAPHICS_VIEW*) mView convertPoint:NSMakePoint(wndpt.x, wndpt.y) fromView:nil];

    x = pt.x / GetDrawScale();
    y = pt.y / GetDrawScale();
  }
}

void IGraphicsMac::HideMouseCursor(bool hide, bool lock)
{
#if defined AU_API
  if (!IsXPCAuHost())
#elif defined AUv3_API
  if (!IsOOPAuv3AppExtension())
#endif
  {
    if (mCursorHidden == hide)
      return;
    
    mCursorHidden = hide;
    
    if (hide)
    {
      StoreCursorPosition();
      CGDisplayHideCursor(kCGDirectMainDisplay);
      mCursorLock = lock;
    }
    else
    {
      DoCursorLock(mCursorX, mCursorY, mCursorX, mCursorY);
      CGDisplayShowCursor(kCGDirectMainDisplay);
      mCursorLock = false;
    }
  }
}

void IGraphicsMac::MoveMouseCursor(float x, float y)
{
  if (mTabletInput)
    return;
    
  PointToScreen(x, y);
  RepositionCursor(CGPoint{x, y});
  StoreCursorPosition();
}

void IGraphicsMac::DoCursorLock(float x, float y, float& prevX, float& prevY)
{
  if (mCursorHidden && mCursorLock && !mTabletInput)
  {
    RepositionCursor(mCursorLockPosition);
    prevX = mCursorX;
    prevY = mCursorY;
  }
  else
  {
    mCursorX = prevX = x;
    mCursorY = prevY = y;
  }
}

void IGraphicsMac::RepositionCursor(CGPoint point)
{
  point = CGPoint{point.x, CGDisplayPixelsHigh(CGMainDisplayID()) - point.y};
  CGAssociateMouseAndMouseCursorPosition(false);
  CGDisplayMoveCursorToPoint(CGMainDisplayID(), point);
  CGAssociateMouseAndMouseCursorPosition(true);
}

void IGraphicsMac::StoreCursorPosition()
{
  // Get position in screen coordinates
  NSPoint mouse = [NSEvent mouseLocation];
  mCursorX = mouse.x = std::round(mouse.x);
  mCursorY = mouse.y = std::round(mouse.y);
  mCursorLockPosition = CGPoint{mouse.x, mouse.y};
  
  // Convert to IGraphics coordinates
  ScreenToPoint(mCursorX, mCursorY);
}

void IGraphicsMac::GetMouseLocation(float& x, float&y) const
{
  // Get position in screen coordinates
  NSPoint mouse = [NSEvent mouseLocation];
  x = mouse.x;
  y = mouse.y;
  
  // Convert to IGraphics coordinates
  ScreenToPoint(x, y);
}

EMsgBoxResult IGraphicsMac::ShowMessageBox(const char* str, const char* caption, EMsgBoxType type, IMsgBoxCompletionHanderFunc completionHandler)
{
  ReleaseMouseCapture();

  long result = (long) kCANCEL;
  
  if (!str) str= "";
  if (!caption) caption= "";
  
  NSString* msg = (NSString*) CFStringCreateWithCString(NULL,str,kCFStringEncodingUTF8);
  NSString* cap = (NSString*) CFStringCreateWithCString(NULL,caption,kCFStringEncodingUTF8);
 
  msg = msg ? msg : (NSString*) CFStringCreateWithCString(NULL, str, kCFStringEncodingASCII);
  cap = cap ? cap : (NSString*) CFStringCreateWithCString(NULL, caption, kCFStringEncodingASCII);
  
  switch (type)
  {
    case kMB_OK:
      NSRunAlertPanel(msg, @"%@", @"OK", @"", @"", cap);
      result = kOK;
      break;
    case kMB_OKCANCEL:
      result = NSRunAlertPanel(msg, @"%@", @"OK", @"Cancel", @"", cap);
      result = result ? kOK : kCANCEL;
      break;
    case kMB_YESNO:
      result = NSRunAlertPanel(msg, @"%@", @"Yes", @"No", @"", cap);
      result = result ? kYES : kNO;
      break;
    case kMB_RETRYCANCEL:
      result = NSRunAlertPanel(msg, @"%@", @"Retry", @"Cancel", @"", cap);
      result = result ? kRETRY : kCANCEL;
      break;
    case kMB_YESNOCANCEL:
      result = NSRunAlertPanel(msg, @"%@", @"Yes", @"Cancel", @"No", cap);
      result = (result == 1) ? kYES : (result == -1) ? kNO : kCANCEL;
      break;
  }
  
  [msg release];
  [cap release];
  
  if(completionHandler)
    completionHandler(static_cast<EMsgBoxResult>(result));
  
  return static_cast<EMsgBoxResult>(result);
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
  if (!(mView && TooltipsEnabled()))
    return;

  @autoreleasepool {

  [(IGRAPHICS_VIEW*) mView removeAllToolTips];

  if (GetPopupMenuControl() && GetPopupMenuControl()->GetState() > IPopupMenuControl::kCollapsed)
  {
    return;
  }

  auto func = [this](IControl* pControl)
  {
    if (pControl->GetTooltip() && !pControl->IsHidden())
    {
      IRECT pR = pControl->GetTargetRECT();
      if (!pR.Empty())
      {
        [(IGRAPHICS_VIEW*) mView registerToolTip: pR];
      }
    }
  };

  ForStandardControlsFunc(func);
  
  }
}

const char* IGraphicsMac::GetPlatformAPIStr()
{
  return "Cocoa";
}

bool IGraphicsMac::RevealPathInExplorerOrFinder(WDL_String& path, bool select)
{
  BOOL success = FALSE;

  @autoreleasepool {
    
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

  NSString* pDefaultFileName = nil;
  NSString* pDefaultPath = nil;
  NSArray* pFileTypes = nil;

  if (fileName.GetLength())
    pDefaultFileName = [NSString stringWithCString:fileName.Get() encoding:NSUTF8StringEncoding];

  if(path.GetLength())
    pDefaultPath = [NSString stringWithCString:path.Get() encoding:NSUTF8StringEncoding];

  fileName.Set(""); // reset it

  if (CStringHasContents(ext))
    pFileTypes = [[NSString stringWithUTF8String:ext] componentsSeparatedByString: @" "];

  if (action == EFileAction::Save)
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

bool IGraphicsMac::PromptForColor(IColor& color, const char* str, IColorPickerHandlerFunc func)
{
  if (mView)
    return [(IGRAPHICS_VIEW*) mView promptForColor:color : func];

  return false;
}

IPopupMenu* IGraphicsMac::CreatePlatformPopupMenu(IPopupMenu& menu, const IRECT& bounds, bool& isAsync)
{
  IPopupMenu* pReturnMenu = nullptr;

  if (mView)
  {
    NSRect areaRect = ToNSRect(this, bounds);
    pReturnMenu = [(IGRAPHICS_VIEW*) mView createPopupMenu: menu: areaRect];
  }

  //synchronous
  if(pReturnMenu && pReturnMenu->GetFunction())
    pReturnMenu->ExecFunction();

  return pReturnMenu;
}

void IGraphicsMac::CreatePlatformTextEntry(int paramIdx, const IText& text, const IRECT& bounds, int length, const char* str)
{
  if (mView)
  {
    NSRect areaRect = ToNSRect(this, bounds);
    [(IGRAPHICS_VIEW*) mView createTextEntry: paramIdx : text: str: length: areaRect];
  }
}

ECursor IGraphicsMac::SetMouseCursor(ECursor cursorType)
{
  if (mView)
    [(IGRAPHICS_VIEW*) mView setMouseCursor: cursorType];
    
  return IGraphics::SetMouseCursor(cursorType);
}

bool IGraphicsMac::OpenURL(const char* url, const char* msgWindowTitle, const char* confirmMsg, const char* errMsgOnFailure)
{
  #pragma REMINDER("Warning and error messages for OpenURL not implemented")
  NSURL* pNSURL = nullptr;
  if (strstr(url, "http"))
    pNSURL = [NSURL URLWithString:[NSString stringWithCString:url encoding:NSUTF8StringEncoding]];
  else
    pNSURL = [NSURL fileURLWithPath:[NSString stringWithCString:url encoding:NSUTF8StringEncoding]];

  if (pNSURL)
  {
    bool ok = ([[NSWorkspace sharedWorkspace] openURL:pNSURL]);
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

bool IGraphicsMac::SetTextInClipboard(const char* str)
{
  NSString* pTextForClipboard = [NSString stringWithUTF8String:str];
  [[NSPasteboard generalPasteboard] clearContents];
  return [[NSPasteboard generalPasteboard] setString:pTextForClipboard forType:NSStringPboardType];
}

EUIAppearance IGraphicsMac::GetUIAppearance() const
{
  if (@available(macOS 10.14, *)) {
    if(mView)
    {
      IGRAPHICS_VIEW* pView = (IGRAPHICS_VIEW*) mView;
      BOOL isDarkMode = [[[pView effectiveAppearance] name] isEqualToString: (NSAppearanceNameDarkAqua)];
      return isDarkMode ? EUIAppearance::Dark :  EUIAppearance::Light;
    }
  }
  
  return EUIAppearance::Light;
}

#if defined IGRAPHICS_NANOVG
  #include "IGraphicsNanoVG.cpp"
#elif defined IGRAPHICS_SKIA
  #include "IGraphicsSkia.cpp"
#else
  #error Either NO_IGRAPHICS or one and only one choice of graphics library must be defined!
#endif
