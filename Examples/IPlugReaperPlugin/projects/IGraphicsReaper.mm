
#include "IGraphicsReaper.h"

#pragma warning( push )
#pragma warning( disable : 4244 )
#include "include/core/SkBitmap.h"

using namespace iplug;
using namespace igraphics;

// NSRect

inline NSRect ToNSRect(IGraphics* pGraphics, const IRECT& bounds)
{
  const float scale = pGraphics->GetDrawScale();
  const float x = floor(bounds.L * scale);
  const float y = floor(bounds.T * scale);
  const float x2 = ceil(bounds.R * scale);
  const float y2 = ceil(bounds.B * scale);
    
  return NSMakeRect(x, y, x2 - x, y2 - y);
}

// System Version

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

int IGraphicsReaper::GetUserOSVersion() { return (int) GetSystemVersion(); }

// Position

void IGraphicsReaper::PointToScreen(float& x, float& y) const
{
  if (GetWindow())
  {
    x *= GetDrawScale();
    y *= GetDrawScale();
    NSWindow* pWindow = [(NSView*) GetWindow() window];
    NSPoint wndpt = [(NSView*) GetWindow() convertPoint:NSMakePoint(x, y) toView:nil];
    NSPoint pt = [pWindow convertRectToScreen: NSMakeRect(wndpt.x, wndpt.y, 0.0, 0.0)].origin;
      
    x = pt.x;
    y = pt.y;
  }
}

void IGraphicsReaper::ScreenToPoint(float& x, float& y) const
{
  if (GetWindow())
  {
    NSWindow* pWindow = [(NSView*) GetWindow() window];
    NSPoint wndpt = [pWindow convertRectFromScreen: NSMakeRect(x, y, 0.0, 0.0)].origin;
    NSPoint pt = [(NSView*) GetWindow() convertPoint:NSMakePoint(wndpt.x, wndpt.y) fromView:nil];

    x = pt.x / GetDrawScale();
    y = pt.y / GetDrawScale();
  }
}

// Mouse

void IGraphicsReaper::HideMouseCursor(bool hide, bool lock)
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

void IGraphicsReaper::MoveMouseCursor(float x, float y)
{
  if (mTabletInput)
    return;
    
  PointToScreen(x, y);
  RepositionCursor(CGPoint{x, y});
  StoreCursorPosition();
}

ECursor IGraphicsReaper::SetMouseCursor(ECursor cursorType)
{
  return IGraphics::SetMouseCursor(cursorType);
}

void IGraphicsReaper::DoCursorLock(float x, float y, float& prevX, float& prevY)
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

void IGraphicsReaper::RepositionCursor(CGPoint point)
{
  point = CGPoint{point.x, CGDisplayPixelsHigh(CGMainDisplayID()) - point.y};
  CGAssociateMouseAndMouseCursorPosition(false);
  CGDisplayMoveCursorToPoint(CGMainDisplayID(), point);
  CGAssociateMouseAndMouseCursorPosition(true);
}

void IGraphicsReaper::StoreCursorPosition()
{
  // Get position in screen coordinates
  NSPoint mouse = [NSEvent mouseLocation];
  mCursorX = mouse.x = std::round(mouse.x);
  mCursorY = mouse.y = std::round(mouse.y);
  mCursorLockPosition = CGPoint{mouse.x, mouse.y};
  
  // Convert to IGraphics coordinates
  ScreenToPoint(mCursorX, mCursorY);
}

void IGraphicsReaper::GetMouseLocation(float& x, float&y) const
{
  // Get position in screen coordinates
  NSPoint mouse = [NSEvent mouseLocation];
  x = mouse.x;
  y = mouse.y;
  
  // Convert to IGraphics coordinates
  ScreenToPoint(x, y);
}

// CLIPBOARD

bool IGraphicsReaper::GetTextFromClipboard(WDL_String& str)
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

bool IGraphicsReaper::SetTextInClipboard(const char* str)
{
  NSString* pTextForClipboard = [NSString stringWithUTF8String:str];
  [[NSPasteboard generalPasteboard] clearContents];
  return [[NSPasteboard generalPasteboard] setString:pTextForClipboard forType:NSStringPboardType];
}

bool IGraphicsReaper::SetFilePathInClipboard(const char* path)
{
  NSPasteboard* pPasteboard = [NSPasteboard generalPasteboard];
  [pPasteboard clearContents]; // clear pasteboard to take ownership
  NSURL* pFileURL = [NSURL fileURLWithPath: [NSString stringWithUTF8String:path]];
  BOOL success = [pPasteboard writeObjects: [NSArray arrayWithObject:pFileURL]];
  return (bool)success;
}

// FIX - no draga and drop

bool IGraphicsReaper::InitiateExternalFileDragDrop(const char* path, const IRECT& iconBounds)
{
  /*
  NSPasteboardItem* pasteboardItem = [[NSPasteboardItem alloc] init];
  NSURL* fileURL = [NSURL fileURLWithPath: [NSString stringWithUTF8String:path]];
  [pasteboardItem setString:fileURL.absoluteString forType:NSPasteboardTypeFileURL];
  
  NSDraggingItem* draggingItem = [[NSDraggingItem alloc] initWithPasteboardWriter:pasteboardItem];
  NSRect draggingFrame = ToNSRect(this, iconBounds);
  NSImage* iconImage = [[NSWorkspace sharedWorkspace] iconForFile:fileURL.path];
  [iconImage setSize:NSMakeSize(64, 64)];
  [draggingItem setDraggingFrame:draggingFrame contents: iconImage];
  
  IGRAPHICS_VIEW* view = (IGRAPHICS_VIEW*) mView;
  NSDraggingSession* draggingSession = [view beginDraggingSessionWithItems:@[draggingItem] event:[NSApp currentEvent] source: view];
  draggingSession.animatesToStartingPositionsOnCancelOrFail = YES;
  draggingSession.draggingFormation = NSDraggingFormationNone;
  
  ReleaseMouseCapture();
  
  return true;*/
  
  return false;
}

// Reveal / Prompts

bool IGraphicsReaper::RevealPathInExplorerOrFinder(WDL_String& path, bool select)
{
  BOOL success = FALSE;

  @autoreleasepool {
    
  if(path.GetLength())
  {
    NSString* pPath = [NSString stringWithUTF8String:path.Get()];

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

void IGraphicsReaper::PromptForFile(WDL_String& fileName, WDL_String& path, EFileAction action, const char* ext, IFileDialogCompletionHandlerFunc completionHandler)
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
    pDefaultFileName = [NSString stringWithUTF8String:fileName.Get()];
  else
    pDefaultFileName = @"";
  
  if (path.GetLength())
    pDefaultPath = [NSString stringWithUTF8String:path.Get()];
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
    // FIX - is this useful?
    
    NSWindow* pWindow = [(NSView*) GetWindow() window];

    [(NSSavePanel*) pPanel beginSheetModalForWindow:pWindow completionHandler:^(NSModalResponse response) {
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

void IGraphicsReaper::PromptForDirectory(WDL_String& dir, IFileDialogCompletionHandlerFunc completionHandler)
{
  NSString* defaultPath;

  if (dir.GetLength())
  {
    defaultPath = [NSString stringWithUTF8String:dir.Get()];
  }
  else
  {
    defaultPath = [NSString stringWithUTF8String:DEFAULT_PATH];
    dir.Set(DEFAULT_PATH);
  }

  NSOpenPanel* panelOpen = [NSOpenPanel openPanel];

  [panelOpen setTitle:@"Choose a Directory"];
  [panelOpen setCanChooseFiles:NO];
  [panelOpen setCanChooseDirectories:YES];
  [panelOpen setResolvesAliases:YES];
  [panelOpen setCanCreateDirectories:YES];
  [panelOpen setFloatingPanel: YES];
  [panelOpen setDirectoryURL: [NSURL fileURLWithPath: defaultPath]];
  
  auto doHandleResponse = [](NSOpenPanel* pPanel, NSModalResponse response, WDL_String& pathAsync, IFileDialogCompletionHandlerFunc completionHandler){
    if (response == NSOKButton)
    {
      NSString* fullPath = [pPanel filename] ;
      pathAsync.Set([fullPath UTF8String]);
      pathAsync.Append("/");
    }
    else
    {
      pathAsync.Set("");
    }
    
    if (completionHandler)
    {
      WDL_String fileNameAsync; // not used
      completionHandler(fileNameAsync, pathAsync);
    }
  };

  if (completionHandler)
  {
    // FIX - is this useful?
    
    NSWindow* pWindow = [(NSView*) GetWindow() window];

    [panelOpen beginSheetModalForWindow:pWindow completionHandler:^(NSModalResponse response) {
      WDL_String pathAsync;
      doHandleResponse(panelOpen, response, pathAsync, completionHandler);
    }];
  }
  else
  {
    NSModalResponse response = [panelOpen runModal];
    doHandleResponse(panelOpen, response, dir, nullptr);
  }
}

// FIX - no view

bool IGraphicsReaper::PromptForColor(IColor& color, const char* str, IColorPickerHandlerFunc func)
{
  //if (mView)
  //  return [(IGRAPHICS_VIEW*) mView promptForColor:color : func];

  return false;
}

bool IGraphicsReaper::OpenURL(const char* url, const char* msgWindowTitle, const char* confirmMsg, const char* errMsgOnFailure)
{
  #pragma REMINDER("Warning and error messages for OpenURL not implemented")
  NSURL* pNSURL = nullptr;
  if (strstr(url, "http"))
    pNSURL = [NSURL URLWithString:[NSString stringWithUTF8String:url]];
  else
    pNSURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:url]];

  if (pNSURL)
  {
    bool ok = ([[NSWorkspace sharedWorkspace] openURL:pNSURL]);
    return ok;
  }
  return true;
}

EMsgBoxResult IGraphicsReaper::ShowMessageBox(const char* str, const char* title, EMsgBoxType type, IMsgBoxCompletionHandlerFunc completionHandler)
{
  ReleaseMouseCapture();

  NSString* messageContent = @(str ? str : "");
  NSString* alertTitle = @(title ? title : "");
  
  NSAlert* alert = [[NSAlert alloc] init];
  [alert setMessageText:alertTitle];
  [alert setInformativeText:messageContent];
  
  EMsgBoxResult result = kCANCEL;
  
  switch (type)
  {
    case kMB_OK:
      [alert addButtonWithTitle:@"OK"];
      result = kOK;
      break;
    case kMB_OKCANCEL:
      [alert addButtonWithTitle:@"OK"];
      [alert addButtonWithTitle:@"Cancel"];
      result = kCANCEL;
      break;
    case kMB_YESNO:
      [alert addButtonWithTitle:@"Yes"];
      [alert addButtonWithTitle:@"No"];
      result = kNO;
      break;
    case kMB_RETRYCANCEL:
      [alert addButtonWithTitle:@"Retry"];
      [alert addButtonWithTitle:@"Cancel"];
      result = kCANCEL;
      break;
    case kMB_YESNOCANCEL:
      [alert addButtonWithTitle:@"Yes"];
      [alert addButtonWithTitle:@"No"];
      [alert addButtonWithTitle:@"Cancel"];
      result = kCANCEL;
      break;
  }
  
  NSModalResponse response = [alert runModal];
  
  switch (type)
  {
    case kMB_OK:
      result = kOK;
      break;
    case kMB_OKCANCEL:
      result = (response == NSAlertFirstButtonReturn) ? kOK : kCANCEL;
      break;
    case kMB_YESNO:
      result = (response == NSAlertFirstButtonReturn) ? kYES : kNO;
      break;
    case kMB_RETRYCANCEL:
      result = (response == NSAlertFirstButtonReturn) ? kRETRY : kCANCEL;
      break;
    case kMB_YESNOCANCEL:
      if (response == NSAlertFirstButtonReturn) result = kYES;
      else if (response == NSAlertSecondButtonReturn) result = kNO;
      else result = kCANCEL;
      break;
  }
  
  if (completionHandler)
  {
    completionHandler(result);
  }
  
  [alert release];
  
  return result;
}

// Platform Entry etc.

// FIX - these require a view

IPopupMenu* IGraphicsReaper::CreatePlatformPopupMenu(IPopupMenu& menu, const IRECT bounds, bool& isAsync)
{
  isAsync = true;
  
  dispatch_async(dispatch_get_main_queue(), ^{
    IPopupMenu* pReturnMenu = nullptr;

    if (GetWindow())
    {
      NSRect areaRect = ToNSRect(this, bounds);
      //pReturnMenu = [(IGRAPHICS_VIEW*) GetWindow() createPopupMenu: menu: areaRect];
    }

    if (pReturnMenu && pReturnMenu->GetFunction())
      pReturnMenu->ExecFunction();
    
    this->SetControlValueAfterPopupMenu(pReturnMenu);
  });

  return nullptr;
}

void IGraphicsReaper::CreatePlatformTextEntry(int paramIdx, const IText& text, const IRECT& bounds, int length, const char* str)
{
  if (GetWindow())
  {
    NSRect areaRect = ToNSRect(this, bounds);
    //[(IGRAPHICS_VIEW*) GetWindow() createTextEntry: paramIdx : text: str: length: areaRect];
  }
}

// Font Loading

extern StaticStorage<CoreTextFontDescriptor> sFontDescriptorCache;

PlatformFontPtr IGraphicsReaper::LoadPlatformFont(const char* fontID, const char* fileNameOrResID)
{
  return CoreTextHelpers::LoadPlatformFont(fontID, fileNameOrResID, GetBundleID(), GetSharedResourcesSubPath());
}

PlatformFontPtr IGraphicsReaper::LoadPlatformFont(const char* fontID, const char* fontName, ETextStyle style)
{
  return CoreTextHelpers::LoadPlatformFont(fontID, fontName, style);
}

PlatformFontPtr IGraphicsReaper::LoadPlatformFont(const char* fontID, void* pData, int dataSize)
{
  return CoreTextHelpers::LoadPlatformFont(fontID, pData, dataSize);
}

void IGraphicsReaper::CachePlatformFont(const char* fontID, const PlatformFontPtr& font)
{
  CoreTextHelpers::CachePlatformFont(fontID, font, sFontDescriptorCache);
}

// Constructor / Destructor

IGraphicsReaper::IGraphicsReaper(IGEditorDelegate& dlg, int w, int h, int fps, float scale)
: IGraphicsSkia(dlg, w, h, fps, scale, true)
{
  NSApplicationLoad();
  StaticStorage<CoreTextFontDescriptor>::Accessor storage(sFontDescriptorCache);
  storage.Retain();
}

IGraphicsReaper::~IGraphicsReaper()
{
  StaticStorage<CoreTextFontDescriptor>::Accessor storage(sFontDescriptorCache);
  storage.Release();
  
  CloseWindow();
}

// Draw Loop

int IGraphicsReaper::DrawEmbedded(REAPER_FXEMBED_IBitmap* pBitmap, REAPER_FXEMBED_DrawInfo* pInfo)
{
  // Resize or rescale if required
  
  const float screenScale = pInfo->dpi / 256.f;
  const int width = pInfo->width / screenScale;
  const int height = pInfo->height / screenScale;

  if (screenScale != GetScreenScale())
    SetScreenScale(screenScale);
  
  if (width != Width() || height != Height())
    Resize(width, height, 1.f);
  
  // Do Drawing
  
  IRECTList rects;

  if (IsDirty(rects))
  {
    SetAllControlsClean();
    Draw(rects);
  }
  else if (pInfo->flags & REAPER_FXEMBED_DRAWINFO_FLAG_PAINT_OPTIONAL)
  {
    return 0;
  }
  
  // Copy to the output bitmap
  
  if (pBitmap && mSurface)
  {
    SkPixmap pixmap;
    mSurface->peekPixels(&pixmap);
    
    SkBitmap output;
    SkImageInfo info = SkImageInfo::Make(pBitmap->getWidth(), pBitmap->getHeight(), kBGRA_8888_SkColorType, kPremul_SkAlphaType);
    
    size_t rowBytes = pBitmap->getRowSpan() * sizeof(unsigned int);
    output.installPixels(info, pBitmap->getBits(), rowBytes);
    
    // Copy to the output
    
    output.writePixels(pixmap);
  }
  
  return 1;
}

// Draw Resize

void IGraphicsReaper::DrawResize()
{
  auto w = static_cast<int>(std::ceil(static_cast<float>(WindowWidth()) * GetScreenScale()));
  auto h = static_cast<int>(std::ceil(static_cast<float>(WindowHeight()) * GetScreenScale()));
  
  mSurface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(w, h));

  if (mSurface)
  {
    mCanvas = mSurface->getCanvas();
    mCanvas->save();
  }
}

// UI Proc

IMouseInfo IGraphicsReaper::GetMouseInfo(void* pMsg)
{
  // FIX - deltas
  // FIX - modifiers
  
  auto* pInfo = reinterpret_cast<REAPER_FXEMBED_DrawInfo*>(pMsg);
  
  const float scale = GetTotalScale();

  float x = pInfo->mouse_x / scale;
  float y = pInfo->mouse_y / scale;
  float dX = mLastX >= 0.f ? x - mLastX : 0.f;
  float dY = mLastY >= 0.f ? y - mLastY : 0.f;
  bool l = pInfo->flags & REAPER_FXEMBED_DRAWINFO_FLAG_LBUTTON_CAPTURED;
  bool r = pInfo->flags & REAPER_FXEMBED_DRAWINFO_FLAG_RBUTTON_CAPTURED;
  
  int mods = (int) [NSEvent modifierFlags];
  
  mLastX = x;
  mLastY = y;
  
  IMouseMod mod(l, r, (mods & NSShiftKeyMask), (mods & NSControlKeyMask), (mods & NSAlternateKeyMask));
  
  return IMouseInfo{x, y, dX, dY, mod};
}

void DebugMouse(const char *str, int msg, int msgL, int msgR, const IMouseInfo& info)
{
  const char* tag = "";
  
  if (msg == msgL)
    tag = "Left ";
  else if (msg == msgR)
    tag = "Right ";
   
  DBGMSG("%s%s %f %f %f %F %d %d %d %d %d\n", tag, str, info.x, info.y, info.dX, info.dY, info.ms.L, info.ms.R, info.ms.A, info.ms.C, info.ms.S);
}

int IGReaperEditorDelegate::EmbeddedUIProc(int message, void* pMsg1, void* pMsg2)
{
  auto GetGraphics = [&, this]() { return static_cast<IGraphicsReaper*>(GetUI()); };
  
  switch (message)
  {
    case 0:
      return 1;
      
    case REAPER_FXEMBED_WM_CREATE:
      OpenWindow(nullptr);
      GetUI()->SetLayoutOnResize(true);
      return 1;
      
    case REAPER_FXEMBED_WM_DESTROY:
      CloseWindow();
      return 1;
      
    //case WM_SETCURSOR: return HCURSOR;
      
    case REAPER_FXEMBED_WM_MOUSEMOVE:
    {
      auto info = GetGraphics()->GetMouseInfo(pMsg2);
      std::vector<IMouseInfo> list {info};
      
      if (info.ms.L || info.ms.R)
      {
        DebugMouse("Drag", message, 0, 0, GetGraphics()->GetMouseInfo(pMsg2));
        GetUI()->OnMouseDrag(list);
      }
      else
        GetUI()->OnMouseOver(info.x, info.y, info.ms);
      
      return 0;
    }
      
    case REAPER_FXEMBED_WM_LBUTTONDOWN:
    case REAPER_FXEMBED_WM_RBUTTONDOWN:
    {
      std::vector<IMouseInfo> list { GetGraphics()->GetMouseInfo(pMsg2) };
      DebugMouse("Down", message, REAPER_FXEMBED_WM_LBUTTONDOWN, REAPER_FXEMBED_WM_RBUTTONDOWN, GetGraphics()->GetMouseInfo(pMsg2));
      GetUI()->OnMouseDown(list);
      return 0;
    }

    case REAPER_FXEMBED_WM_LBUTTONUP:
    case REAPER_FXEMBED_WM_RBUTTONUP:
    {
      std::vector<IMouseInfo> list { GetGraphics()->GetMouseInfo(pMsg2) };
      DebugMouse("Up", message, REAPER_FXEMBED_WM_LBUTTONUP, REAPER_FXEMBED_WM_RBUTTONUP, GetGraphics()->GetMouseInfo(pMsg2));
      GetUI()->OnMouseUp(list);
      return 0;
    }
      
    case REAPER_FXEMBED_WM_LBUTTONDBLCLK:
    case REAPER_FXEMBED_WM_RBUTTONDBLCLK:
    {
      auto info = GetGraphics()->GetMouseInfo(pMsg2);
      DebugMouse("Double Click", message, REAPER_FXEMBED_WM_LBUTTONDBLCLK, REAPER_FXEMBED_WM_RBUTTONDBLCLK, info);
      GetUI()->OnMouseDblClick(info.x, info.y, info.ms);
      return 0;
    }
      
    case REAPER_FXEMBED_WM_MOUSEWHEEL:
    {
      // FIX - check

      static const float wheelStep = 120.f;
      auto info = GetGraphics()->GetMouseInfo(pMsg2);
      auto* pInfo = reinterpret_cast<REAPER_FXEMBED_DrawInfo*>(pMsg2);
      GetUI()->OnMouseWheel(info.x, info.y, info.ms, pInfo->mousewheel_amt / wheelStep);
      return 0;
    }
      
    case REAPER_FXEMBED_WM_GETMINMAXINFO:
    {
      auto* pInfo = reinterpret_cast<REAPER_FXEMBED_SizeHints*>(pMsg2);
      
      pInfo->preferred_aspect = mPreferredAspect;
      pInfo->minimum_aspect = mMinimumAspect;
      pInfo->min_width = mMinWidth;
      pInfo->min_height = mMinHeight;
      pInfo->max_width = mMaxWidth;
      pInfo->max_height = mMaxHeight;
     
      return 1;
    }
      
    case REAPER_FXEMBED_WM_PAINT:
    {
      auto* pBitmap = reinterpret_cast<REAPER_FXEMBED_IBitmap*>(pMsg1);
      auto* pInfo = reinterpret_cast<REAPER_FXEMBED_DrawInfo*>(pMsg2);

      return GetGraphics()->DrawEmbedded(pBitmap, pInfo);
    }
      
    default:
      return 0;
  }
}

int GetAspect(int num, int denom)
{
  return num * 65536 / (denom ? denom : 1);
}

void IGReaperEditorDelegate::SetPreferredAspect(int num, int denom)
{
  mPreferredAspect = GetAspect(num, denom);
}

void IGReaperEditorDelegate::SetMinimumAspect(int num, int denom)
{
  mMinimumAspect = GetAspect(num, denom);
}

void IGReaperEditorDelegate::SetMinSize(int width, int height)
{
  mMinWidth = width;
  mMinHeight = height;
}

void IGReaperEditorDelegate::SetMaxSize(int width, int height)
{
  mMaxWidth = width;
  mMaxHeight = height;
}

#ifndef IGRAPHICS_SKIA
  #include "IGraphicsSkia.cpp"
#endif
