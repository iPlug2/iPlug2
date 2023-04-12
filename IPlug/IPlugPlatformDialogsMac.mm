 /*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#if __has_feature(objc_arc)
#error This file must be compiled without Arc. Don't use -fobjc-arc flag!
#endif

#include "IPlugPlatformDialogs.h"
#include "IPlugDialogHandlerViewMac.h"
#include "IPlugUtilities.h"
#include "IPlugPaths.h"

#include <CoreGraphics/CoreGraphics.h>
#include <numeric>

#import <Cocoa/Cocoa.h>

using namespace iplug;

void RepositionCursor(float x, float y)
{
  CGPoint point = CGPoint{x, CGDisplayPixelsHigh(CGMainDisplayID()) - y};
  CGAssociateMouseAndMouseCursorPosition(false);
  CGDisplayMoveCursorToPoint(CGMainDisplayID(), point);
  CGAssociateMouseAndMouseCursorPosition(true);
}

void PointToScreen(void* pView, float& x, float& y, float scaleFactor = 1.0f) {
  auto pNSView = (NSView*) pView;

  if (pView)
  {
    x *= scaleFactor;
    y *= scaleFactor;
    NSWindow* pWindow = [pNSView window];
    NSPoint wndpt = [pNSView convertPoint:NSMakePoint(x, y) toView:nil];
    NSPoint pt = [pWindow convertRectToScreen: NSMakeRect(wndpt.x, wndpt.y, 0.0, 0.0)].origin;

    x = pt.x;
    y = pt.y;
  }
}

void ScreenToPoint(void* pView, float& x, float& y, float scaleFactor = 1.0f)
{
  auto pNSView = (NSView*) pView;

  if (pView)
  {
    NSWindow* pWindow = [pNSView window];
    NSPoint wndpt = [pWindow convertRectFromScreen: NSMakeRect(x, y, 0.0, 0.0)].origin;
    NSPoint pt = [pNSView convertPoint:NSMakePoint(wndpt.x, wndpt.y) fromView:nil];

    x = pt.x / scaleFactor;
    y = pt.y / scaleFactor;
  }
}

void IPlatformDialogs::StoreCursorPosition()
{
  // Get position in screen coordinates
  NSPoint mouse = [NSEvent mouseLocation];
  mCursorX = mouse.x = std::round(mouse.x);
  mCursorY = mouse.y = std::round(mouse.y);
  mCursorLockX = mouse.x;
  mCursorLockY = mouse.y;
  
  // Convert to view coordinates
  ScreenToPoint(mOwningView, mCursorX, mCursorY);
}

void IPlatformDialogs::GetMouseLocation(float& x, float&y) const
{
  // Get position in screen coordinates
  NSPoint mouse = [NSEvent mouseLocation];
  x = mouse.x;
  y = mouse.y;
  
  // Convert to view coordinates
  ScreenToPoint(mOwningView, x, y);
}

void IPlatformDialogs::HideMouseCursor(bool hide, bool lock)
{
#if defined AU_API
  if (!IsXPCAuHost())
#elif defined AUv3_API
  if (!IsOOPAuv3AppExtension())
#endif
  {
    if (mCursorHidden == hide)
      return;
        
    if (hide)
    {
      StoreCursorPosition();
      CGDisplayHideCursor(kCGDirectMainDisplay);
      mCursorLock = lock;
    }
    else
    {
      auto DoCursorLock = [&](float x, float y, float& prevX, float& prevY) {
        if (mCursorHidden && mCursorLock && !mTabletInput)
        {
          RepositionCursor(mCursorLockX, mCursorLockY);
          prevX = mCursorX;
          prevY = mCursorY;
        }
        else
        {
          mCursorX = prevX = x;
          mCursorY = prevY = y;
        }
      };
      DoCursorLock(mCursorX, mCursorY, mCursorX, mCursorY);
      CGDisplayShowCursor(kCGDirectMainDisplay);
      mCursorLock = false;
    }
    
    mCursorHidden = hide;
  }
}

void IPlatformDialogs::MoveMouseCursor(float x, float y)
{
  if (mTabletInput)
    return;
    
  PointToScreen(mOwningView, x, y);
  RepositionCursor(x, y);
  StoreCursorPosition();
}

EMsgBoxResult IPlatformDialogs::ShowMessageBox(const char* str, const char* caption, EMsgBoxType type, IMsgBoxCompletionHandlerFunc completionHandler)
{
//  ReleaseMouseCapture();

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
  
  if (completionHandler)
    completionHandler(static_cast<EMsgBoxResult>(result));
  
  return static_cast<EMsgBoxResult>(result);
}

void IPlatformDialogs::PromptForFile(WDL_String& fileName, WDL_String& path, EFileAction action, const char* ext, IFileDialogCompletionHandlerFunc completionHandler)
{
//  if (!WindowIsOpen())
//  {
//    fileName.Set("");
//    return;
//  }

  NSString* pDefaultFileName = nil;
  NSString* pDefaultPath = nil;
  NSArray* pFileTypes = nil;
  
  if (fileName.GetLength())
    pDefaultFileName = [NSString stringWithCString:fileName.Get() encoding:NSUTF8StringEncoding];
  else
    pDefaultFileName = @"";
  
  if (path.GetLength())
    pDefaultPath = [NSString stringWithCString:path.Get() encoding:NSUTF8StringEncoding];
  else
    pDefaultPath = @"";
  
  fileName.Set(""); // reset it
  
  if (CStringHasContents(ext))
    pFileTypes = [[NSString stringWithUTF8String:ext] componentsSeparatedByString: @" "];
  
  auto doHandleResponse = [](NSPanel* pPanel, NSModalResponse response, WDL_String& fileName, WDL_String& path, IFileDialogCompletionHandlerFunc completionHandler){
    if (response == NSOKButton)
    {
      NSString* pFullPath = [(NSSavePanel*) pPanel filename] ;
      fileName.Set([pFullPath UTF8String]);
      
      NSString* pTruncatedPath = [pFullPath stringByDeletingLastPathComponent];
      
      if (pTruncatedPath)
      {
        path.Set([pTruncatedPath UTF8String]);
        path.Append("/");
      }
    }
    
    if (completionHandler)
      completionHandler(fileName, path);
  };
  
  NSPanel* pPanel = nullptr;
  
  if (action == EFileAction::Save)
  {
    pPanel = [NSSavePanel savePanel];
    
    [(NSSavePanel*) pPanel setAllowedFileTypes: pFileTypes];
    [(NSSavePanel*) pPanel setDirectoryURL: [NSURL fileURLWithPath: pDefaultPath]];
    [(NSSavePanel*) pPanel setNameFieldStringValue: pDefaultFileName];
    [(NSSavePanel*) pPanel setAllowsOtherFileTypes: NO];
  }
  else
  {
    pPanel = [NSOpenPanel openPanel];
    
    [(NSOpenPanel*) pPanel setAllowedFileTypes: pFileTypes];
    [(NSOpenPanel*) pPanel setDirectoryURL: [NSURL fileURLWithPath: pDefaultPath]];
    [(NSOpenPanel*) pPanel setCanChooseFiles:YES];
    [(NSOpenPanel*) pPanel setCanChooseDirectories:NO];
    [(NSOpenPanel*) pPanel setResolvesAliases:YES];
  }
  [pPanel setFloatingPanel: YES];
  
  if (completionHandler)
  {
    [(NSSavePanel*) pPanel beginWithCompletionHandler:^(NSModalResponse response){
      WDL_String fileNameAsync, pathAsync;
      doHandleResponse(pPanel, response, fileNameAsync, pathAsync, completionHandler);
    }];
  }
  else
  {
    NSModalResponse response = [(NSSavePanel*) pPanel runModal];
    doHandleResponse(pPanel, response, fileName, path, nullptr);
  }
}

void IPlatformDialogs::PromptForDirectory(WDL_String& dir, IFileDialogCompletionHandlerFunc completionHandler)
{
  if (!dir.GetLength())
    DesktopPath(dir);

  NSString* defaultPath = [NSString stringWithCString:dir.Get() encoding:NSUTF8StringEncoding];

  NSOpenPanel* panelOpen = [NSOpenPanel openPanel];

  [panelOpen setTitle:@"Choose a Directory"];
  [panelOpen setCanChooseFiles:NO];
  [panelOpen setCanChooseDirectories:YES];
  [panelOpen setResolvesAliases:YES];
  [panelOpen setCanCreateDirectories:YES];
  [panelOpen setFloatingPanel: YES];
  [panelOpen setDirectoryURL: [NSURL fileURLWithPath: defaultPath]];

  auto doHandleResponse = [](NSOpenPanel* pPanel, NSModalResponse response, WDL_String& chosenDir, IFileDialogCompletionHandlerFunc completionHandler){
    if (response == NSOKButton)
    {
      NSString* fullPath = [pPanel filename] ;
      chosenDir.Set([fullPath UTF8String]);
      chosenDir.Append("/");
    }
    else
    {
      chosenDir.Set("");
    }

    if (completionHandler)
    {
      WDL_String fileName; // not used
      completionHandler(fileName, chosenDir);
    }
  };

  if (completionHandler)
  {
    NSModalResponse response = [panelOpen runModal];
    doHandleResponse(panelOpen, response, dir, completionHandler);
  }
  else
  {
    [panelOpen beginWithCompletionHandler:^(NSModalResponse response) {
      WDL_String path;
      doHandleResponse(panelOpen, response, path, nullptr);
    }];
  }
}

bool IPlatformDialogs::RevealPathInExplorerOrFinder(WDL_String& path, bool select)
{
  BOOL success = FALSE;

  @autoreleasepool {
    
  if (path.GetLength())
  {
    NSString* pPath = [NSString stringWithCString:path.Get() encoding:NSUTF8StringEncoding];

    if ([[NSFileManager defaultManager] fileExistsAtPath : pPath] == YES)
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

bool IPlatformDialogs::OpenURL(const char* url, const char* msgWindowTitle, const char* confirmMsg, const char* errMsgOnFailure)
{
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

void IPlatformDialogs::CreatePopupMenu(IPopupMenu& menu, float x, float y, IPopupMenuCompletionHandlerFunc completionHandler)
{
  NSPoint wp = {x, y};
  IPLUG_DIALOG_HANDLER_VIEW* pView = (IPLUG_DIALOG_HANDLER_VIEW*) mOwningView;
  [pView showPopupMenuAtLocation:menu :wp : completionHandler];
}
