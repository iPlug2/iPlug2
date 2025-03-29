
#include "IGraphicsCoreText.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

namespace IGraphicsMacUtilities
{
  using FontType = CoreTextFontDescriptor;
  
  // GUI Load
  
  void GUILoad()
  {
    NSApplicationLoad();
  }
  
  // Cursor Utilities
  
  void HideCursor(bool hide)
  {
    if (hide)
      CGDisplayHideCursor(kCGDirectMainDisplay);
    else
      CGDisplayShowCursor(kCGDirectMainDisplay);
  }

  void GetCursorPosition(float& x, float& y)
  {
    CGEventRef ourEvent = CGEventCreate(NULL);
    CGPoint point = CGEventGetLocation(ourEvent);
    CFRelease(ourEvent);
    
    x = point.x;
    y = point.y;
  }
  
  bool RepositionCursor(float x, float y)
  {    
    CGPoint point{x, y};
    CGAssociateMouseAndMouseCursorPosition(false);
    auto err = CGDisplayMoveCursorToPoint(CGMainDisplayID(), point);
    CGAssociateMouseAndMouseCursorPosition(true);
    
    return err == kCGErrorSuccess;
  }

  // Mouse Modifiers
  
  IMouseMod GetMouseModifiers(bool l, bool r)
  {
    int mods = (int) [NSEvent modifierFlags];

    return IMouseMod(l, r, (mods & NSShiftKeyMask), (mods & NSControlKeyMask), (mods & NSAlternateKeyMask));
  }
  
  // Reveal
  
  bool RevealPathInExplorerOrFinder(WDL_String& path, bool select)
  {
    BOOL success = FALSE;

    @autoreleasepool
    {
      if (path.GetLength())
      {
        NSString* pPath = [NSString stringWithUTF8String:path.Get()];
        
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
          else
          {
            success = [[NSWorkspace sharedWorkspace] openFile:pPath];
          }
        }
      }
    }
    
    return (bool) success;
  }

  bool OpenURL(const char* url, const char* msgWindowTitle, const char* confirmMsg, const char* errMsgOnFailure)
  {
    #pragma REMINDER("Warning and error messages for OpenURL not implemented")

    NSURL* pNSURL = nullptr;
    
    if (strstr(url, "http"))
      pNSURL = [NSURL URLWithString:[NSString stringWithUTF8String:url]];
    else
      pNSURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:url]];

    if (pNSURL)
      return [[NSWorkspace sharedWorkspace] openURL:pNSURL];
    
    return true;
  }
  
  // Clipboard
  
  bool GetTextFromClipboard(WDL_String& str)
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

  bool SetTextInClipboard(const char* str)
  {
    NSString* pTextForClipboard = [NSString stringWithUTF8String:str];
    [[NSPasteboard generalPasteboard] clearContents];
    return [[NSPasteboard generalPasteboard] setString:pTextForClipboard forType:NSStringPboardType];
  }

  bool SetFilePathInClipboard(const char* path)
  {
    NSPasteboard* pPasteboard = [NSPasteboard generalPasteboard];
    [pPasteboard clearContents]; // clear pasteboard to take ownership
    NSURL* pFileURL = [NSURL fileURLWithPath: [NSString stringWithUTF8String:path]];
    BOOL success = [pPasteboard writeObjects: [NSArray arrayWithObject:pFileURL]];
    return (bool)success;
  }
  
  // Font Loading
  
  PlatformFontPtr LoadPlatformFont(const char* fontID, const char* fileNameOrResID, const char* bundleID, const char* sharedResourcesSubPath)
  {
    return CoreTextHelpers::LoadPlatformFont(fontID, fileNameOrResID, bundleID, sharedResourcesSubPath);
  }

  PlatformFontPtr LoadPlatformFont(const char* fontID, const char* fontName, ETextStyle style)
  {
    return CoreTextHelpers::LoadPlatformFont(fontID, fontName, style);
  }

  PlatformFontPtr LoadPlatformFont(const char* fontID, void* pData, int dataSize)
  {
    return CoreTextHelpers::LoadPlatformFont(fontID, pData, dataSize);
  }

  void CachePlatformFont(const char* fontID, const PlatformFontPtr& font, StaticStorage<CoreTextFontDescriptor>& cache)
  {
    CoreTextHelpers::CachePlatformFont(fontID, font, cache);
  }
}

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
