#include "IGraphicsCarbon.h"
#ifndef IPLUG_NO_CARBON_SUPPORT

IRECT GetRegionRect(EventRef pEvent, int gfxW, int gfxH)
{
  RgnHandle pRgn = 0;
  if (GetEventParameter(pEvent, kEventParamRgnHandle, typeQDRgnHandle, 0, sizeof(RgnHandle), 0, &pRgn) == noErr && pRgn) {
    Rect rct;
    GetRegionBounds(pRgn, &rct);
    return IRECT(rct.left, rct.top, rct.right, rct.bottom); 
  }
  return IRECT(0, 0, gfxW, gfxH);
}

IRECT GetControlRect(EventRef pEvent, int gfxW, int gfxH)
{
  Rect rct;
  if (GetEventParameter(pEvent, kEventParamCurrentBounds, typeQDRectangle, 0, sizeof(Rect), 0, &rct) == noErr) {
    int w = rct.right - rct.left;
    int h = rct.bottom - rct.top;
    if (w > 0 && h > 0) {
      return IRECT(0, 0, w, h);
    }
  }  
  return IRECT(0, 0, gfxW, gfxH);
}

void ResizeWindow(WindowRef pWindow, int w, int h)
{
  Rect gr;  // Screen.
  GetWindowBounds(pWindow, kWindowContentRgn, &gr);
  gr.right = gr.left + w;
  gr.bottom = gr.top + h;
  SetWindowBounds(pWindow, kWindowContentRgn, &gr); 
}

IGraphicsCarbon::IGraphicsCarbon(IGraphicsMac* pGraphicsMac, WindowRef pWindow, ControlRef pParentControl)
: mGraphicsMac(pGraphicsMac)
, mWindow(pWindow)
, mView(0)
, mTimer(0)
, mControlHandler(0)
, mWindowHandler(0)
, mCGC(0)
, mTextEntryView(0)
, mTextEntryHandler(0)
, mEdControl(0)
, mEdParam(0)
, mPrevX(0)
, mPrevY(0)
, mRgn(NewRgn())
{ 
  TRACE;
  
  Rect r;   // Client.
  r.left = r.top = 0;
  r.right = pGraphicsMac->Width();
  r.bottom = pGraphicsMac->Height();   
  
  WindowAttributes winAttrs = 0;
  GetWindowAttributes(pWindow, &winAttrs);
  mIsComposited = (winAttrs & kWindowCompositingAttribute);
 
  UInt32 features =  kControlSupportsFocus | kControlHandlesTracking | kControlSupportsEmbedding;
  
  if (mIsComposited) 
  {
    features |= kHIViewIsOpaque | kHIViewFeatureDoesNotUseSpecialParts;
  }
  
  CreateUserPaneControl(pWindow, &r, features, &mView);    
  
  const EventTypeSpec controlEvents[] = 
  { 
    { kEventClassControl, kEventControlDraw },
  };
  
  InstallControlEventHandler(mView, MainEventHandler, GetEventTypeCount(controlEvents), controlEvents, this, &mControlHandler);
  
  const EventTypeSpec windowEvents[] = 
  {
    { kEventClassMouse, kEventMouseDown },
    { kEventClassMouse, kEventMouseUp },
    { kEventClassMouse, kEventMouseMoved },
    { kEventClassMouse, kEventMouseDragged },
    { kEventClassMouse, kEventMouseWheelMoved },
    
    { kEventClassKeyboard, kEventRawKeyDown },
    
    { kEventClassWindow, kEventWindowDeactivated }
  };
  
  InstallWindowEventHandler(mWindow, MainEventHandler, GetEventTypeCount(windowEvents), windowEvents, this, &mWindowHandler);  
  
  double t = kEventDurationSecond / (double) pGraphicsMac->FPS();
  
  OSStatus s = InstallEventLoopTimer(GetMainEventLoop(), 0., t, TimerHandler, this, &mTimer);
  
  if (mIsComposited) 
  {
    if (!pParentControl) 
    {
      HIViewRef hvRoot = HIViewGetRoot(pWindow);
      s = HIViewFindByID(hvRoot, kHIViewWindowContentID, &pParentControl); 
    }
    
    s = HIViewAddSubview(pParentControl, mView);
  }
  else 
  {
    if (!pParentControl) 
    {
      if (GetRootControl(pWindow, &pParentControl) != noErr) 
      {
        CreateRootControl(pWindow, &pParentControl);
      }
    }
    s = EmbedControl(mView, pParentControl); 
  }

  if (s == noErr) 
  {
    SizeControl(mView, r.right, r.bottom);  // offset?
  }
}

IGraphicsCarbon::~IGraphicsCarbon()
{
  if (mTextEntryView)
  {
    RemoveEventHandler(mTextEntryHandler);
    mTextEntryHandler = 0;
    
    TXNFocus(mTextEntryView, false);
    TXNClear(mTextEntryView);
    TXNDeleteObject(mTextEntryView);
    
    mTextEntryView = 0;
    mEdControl = 0;
    mEdParam = 0;
  }
  
  RemoveEventLoopTimer(mTimer);
  RemoveEventHandler(mControlHandler);
  RemoveEventHandler(mWindowHandler);
  mTimer = 0;
  mView = 0;
  DisposeRgn(mRgn);
}

bool IGraphicsCarbon::Resize(int w, int h)
{
  if (mWindow && mView) 
  {
    ResizeWindow(mWindow, w, h);
    CGRect tmp = CGRectMake(0, 0, w, h);
    return (HIViewSetFrame(mView, &tmp) == noErr);
  }
  return false;
}

void IGraphicsCarbon::EndUserInput(bool commit)
{
  if (mTextEntryHandler) 
  {
    RemoveEventHandler(mTextEntryHandler);
    mTextEntryHandler = 0;
  }
  else {
    return;
  }
  
  if (commit) 
  {
    // Get the text
    CharsHandle textHandle;
    TXNGetDataEncoded(mTextEntryView, kTXNStartOffset, kTXNEndOffset, &textHandle, kTXNTextData);
    
    // Check that we have some worthwhile data
    if (textHandle != NULL && GetHandleSize(textHandle) > 0)
    {
      const long textLength = GetHandleSize(textHandle);
      char txt[257];
      strncpy(txt, *textHandle, (textLength > 255) ? 255 : textLength);
      txt[(textLength > 255) ? 255 : textLength] = '\0';
      
      if (mEdParam) 
        mGraphicsMac->SetFromStringAfterPrompt(mEdControl, mEdParam, txt);
      else
        mEdControl->TextFromTextEntry(txt);
    }
  }
  
  if (mTextEntryView)
  {
    TXNFocus(mTextEntryView, false);
    TXNClear(mTextEntryView);
    TXNDeleteObject(mTextEntryView);
    mTextEntryView = 0;
  }
  
  if (mIsComposited)
  {
    HIViewSetNeedsDisplay(mView, true);
  }
  else
  {
    mEdControl->SetDirty(false);
    mEdControl->Redraw();
  }
  
  SetThemeCursor(kThemeArrowCursor);
  SetUserFocusWindow(kUserFocusAuto);

  mEdControl = 0;
  mEdParam = 0;
}

IPopupMenu* IGraphicsCarbon::CreateIPopupMenu(IPopupMenu* pMenu, IRECT* pAreaRect)
{
  MenuRef menuRef = 0;
  ResID menuID = UniqueID ('MENU');

  int numItems = pMenu->GetNItems();
  
  if (numItems && CreateNewMenu(menuID, kMenuAttrCondenseSeparators, &menuRef) == noErr)
  {
//    bool multipleCheck = menu->getStyle () & (kMultipleCheckStyle & ~kCheckStyle);

    for (int i = 0; i < numItems; ++i)
    { 
      IPopupMenuItem* menuItem = pMenu->GetItem(i);
      
      if (menuItem->GetIsSeparator())
        AppendMenuItemTextWithCFString(menuRef, CFSTR(""), kMenuItemAttrSeparator, 0, NULL);
      else
      {
        CFStringRef itemString = CFStringCreateWithCString (NULL, menuItem->GetText(), kCFStringEncodingUTF8);
        
        if (pMenu->GetPrefix())
        {
          CFStringRef prefixString = 0;
          
          switch (pMenu->GetPrefix())
          {
            case 0: 
              prefixString = CFStringCreateWithFormat (NULL, 0, CFSTR(""),i+1); break;
            case 1:
              prefixString = CFStringCreateWithFormat (NULL, 0, CFSTR("%1d: "),i+1); break;
            case 2:
              prefixString = CFStringCreateWithFormat (NULL, 0, CFSTR("%02d: "),i+1); break;
            case 3:
              prefixString = CFStringCreateWithFormat (NULL, 0, CFSTR("%03d: "),i+1); break;
          }
          
          CFMutableStringRef newItemString = CFStringCreateMutable (0, 0);
          CFStringAppend (newItemString, prefixString);
          CFStringAppend (newItemString, itemString);
          CFRelease (itemString);
          CFRelease (prefixString);
          itemString = newItemString;
        }
        
        if (itemString == 0)
          continue;
        
        MenuItemAttributes itemAttribs = kMenuItemAttrIgnoreMeta;
        if (!menuItem->GetEnabled())
          itemAttribs |= kMenuItemAttrDisabled;
        if (menuItem->GetIsTitle())
          itemAttribs |= kMenuItemAttrSectionHeader;
        
        InsertMenuItemTextWithCFString(menuRef, itemString, i, itemAttribs, 0);
                
        if (menuItem->GetChecked())
        {
          //MacCheckMenuItem(menuRef, i, true);
          CheckMenuItem(menuRef, i+1, true);
        }
//        if (menuItem->GetChecked() && multipleCheck)
//          CheckMenuItem (menuRef, i, true);
        
//        if (menuItem->GetSubmenu())
//        {
//          MenuRef submenu = createMenu (menuItem->GetSubmenu());
//          if (submenu)
//          {
//            SetMenuItemHierarchicalMenu (menuRef, i, submenu);
//            CFRelease (submenu);
//          }
//        }
                
        CFRelease (itemString);
      }
    }
    
  //  if (pMenu->getStyle() & kCheckStyle && !multipleCheck)
  //    CheckMenuItem (menuRef, pMenu->getCurrentIndex (true) + 1, true);
  //  SetMenuItemRefCon(menuRef, 0, (int32_t)menu);
  //  InsertMenu(menuRef, kInsertHierarchicalMenu);
  }

  if (menuRef)
  {
    CalcMenuSize(menuRef);
  //  SInt16 menuWidth = GetMenuWidth(menuRef);
  //  if (menuWidth < optionMenu->getViewSize().getWidth())
  //    SetMenuWidth(menuRef, optionMenu->getViewSize().getWidth());
  //  int32_t popUpItem = 1;
  //  int32_t PopUpMenuItem = PopUpMenuItem = PopUpMenuSelect(menuRef, gy, gx, popUpItem);
    
    // Get the plugin gui frame rect within the host's window
    HIRect rct;
    HIViewGetFrame(this->mView, &rct);
    
    // Get the host's window rect within the screen
    Rect wrct;
    GetWindowBounds(this->mWindow, kWindowContentRgn, &wrct);

    HIViewRef contentView;
    HIViewFindByID (HIViewGetRoot(this->mWindow), kHIViewWindowContentID, &contentView);
    HIViewConvertRect(&rct, HIViewGetSuperview((HIViewRef)this->mView), contentView);
    
    int xpos = wrct.left + rct.origin.x + pAreaRect->L;
    int ypos = wrct.top + rct.origin.y + pAreaRect->B + 5;
    
    int32_t PopUpMenuItem = PopUpMenuSelect (menuRef, ypos, xpos, 0);//popUpItem);

    short result = LoWord(PopUpMenuItem) - 1; 
    short menuIDResult = HiWord(PopUpMenuItem);
    IPopupMenu* resultMenu = 0;
    
    if (menuIDResult != 0)
    {
      //MenuRef usedMenuRef = GetMenuHandle(menuIDResult);
      
      //if (usedMenuRef)
      //{
      //  if (GetMenuItemRefCon(usedMenuRef, 0, (URefCon*)&resultMenu) == noErr)
      //  {
        //  popupResult.menu = resultMenu;
        //  popupResult.index = result;
          
          //printf("result = %i", result);
          
          resultMenu = pMenu;
          resultMenu->SetChosenItemIdx(result);
      //  }
      //}
    }
    
    CFRelease(menuRef);
    
    return resultMenu;
  }
  else {
    return 0;
  }
}

void IGraphicsCarbon::CreateTextEntry(IControl* pControl, IText* pText, IRECT* pTextRect, const char* pString, IParam* pParam)
{
  if (!pControl || mTextEntryView || !mIsComposited) return; // Only composited carbon supports text entry
  
  WindowRef window = mWindow;
  TXNFrameOptions txnFrameOptions = kTXNMonostyledTextMask | kTXNDisableDragAndDropMask | kTXNSingleLineOnlyMask;
  TXNObject txnObject = 0;
  TXNFrameID frameID = 0;
  TXNObjectRefcon txnObjectRefCon = 0;

  HIRect rct;
  HIViewGetFrame(this->mView, &rct);

  HIViewRef contentView;
  HIViewFindByID (HIViewGetRoot(this->mWindow), kHIViewWindowContentID, &contentView);
  HIViewConvertRect(&rct, HIViewGetSuperview((HIViewRef)this->mView), contentView);
  
  Rect rect = { rct.origin.y + pTextRect->T, 
                rct.origin.x + pTextRect->L, 
                rct.origin.y + pTextRect->B + 1, 
                rct.origin.x + pTextRect->R + 1};
  
  if (TXNNewObject(NULL, 
                   window, 
                   &rect, 
                   txnFrameOptions, 
                   kTXNTextEditStyleFrameType, 
                   kTXNSingleStylePerTextDocumentResType, 
                   kTXNMacOSEncoding, 
                   &txnObject, 
                   &frameID, 
                   txnObjectRefCon) == noErr)
  {
    TXNSetFrameBounds(txnObject, rect.top, rect.left, rect.bottom, rect.right, frameID);
    mTextEntryView = txnObject;
    
    // Set the text to display by defualt
    TXNSetData(mTextEntryView, kTXNTextData, pString, strlen(pString)/*+1*/, kTXNStartOffset, kTXNEndOffset); // center aligned text has problems with uneven string lengths
        
    RGBColor tc;
    tc.red = pText->mTextEntryFGColor.R * 257;
    tc.green = pText->mTextEntryFGColor.G * 257;
    tc.blue = pText->mTextEntryFGColor.B * 257;
        
    TXNBackground bg;
    bg.bgType         = kTXNBackgroundTypeRGB;
    bg.bg.color.red   = pText->mTextEntryBGColor.R * 257;
    bg.bg.color.green = pText->mTextEntryBGColor.G * 257;
    bg.bg.color.blue  = pText->mTextEntryBGColor.B * 257;
    
    TXNSetBackground(mTextEntryView, &bg);
    
    // Set justification
    SInt16 justification;
    Fract flushness;
    
    switch ( pText->mAlign ) 
    {
      case IText::kAlignCenter:
        justification = kTXNCenter;  // seems to be buggy wrt dragging and alignement with uneven string lengths
        flushness = kATSUCenterAlignment;
        break;
      case IText::kAlignFar:
        justification = kTXNFlushRight;
        flushness = kATSUEndAlignment;
        break;
      case IText::kAlignNear:
      default:
        justification = kTXNFlushLeft;  
        flushness = kATSUStartAlignment;
        break;
    }
        
    TXNControlTag controlTag[1];
    TXNControlData controlData[1];
    controlTag[0] = kTXNJustificationTag;
    controlData[0].sValue = justification;
    TXNSetTXNObjectControls(mTextEntryView, false, 1, controlTag, controlData);
    
    ATSUFontID fontid = kATSUInvalidFontID;
    
    if (pText->mFont && pText->mFont[0])
    {
      ATSUFindFontFromName(pText->mFont, strlen(pText->mFont), 
                           kFontFullName /* kFontFamilyName? */ ,
                           (FontPlatformCode)kFontNoPlatform,
                           kFontNoScriptCode,
                           kFontNoLanguageCode,
                           &fontid);
    }
    
    // font
    TXNTypeAttributes attributes[3];
    attributes[0].tag = kATSUFontTag;
    attributes[0].size = sizeof(ATSUFontID);
    attributes[0].data.dataPtr = &fontid;
    // size
    attributes[1].tag = kTXNQDFontSizeAttribute;
    attributes[1].size = kTXNFontSizeAttributeSize;
    attributes[1].data.dataValue = pText->mSize << 16;
    // color
    attributes[2].tag = kTXNQDFontColorAttribute;
    attributes[2].size = kTXNQDFontColorAttributeSize;
    attributes[2].data.dataPtr = &tc;
    
    // Finally set the attributes
    TXNSetTypeAttributes(mTextEntryView, 3, attributes, kTXNStartOffset, kTXNEndOffset);
    
    // Ensure focus remains consistent
    SetUserFocusWindow(window);
    AdvanceKeyboardFocus(window);
    
    // Set the focus to the edit window
    TXNFocus(txnObject, true);
    TXNSelectAll(mTextEntryView);
    TXNShowSelection(mTextEntryView, true);
    
    // The event types
    const static EventTypeSpec eventTypes[] = 
    { 
      { kEventClassMouse,    kEventMouseMoved }, 
      { kEventClassMouse,    kEventMouseDown }, 
      { kEventClassMouse,    kEventMouseUp }, 
      { kEventClassMouse,    kEventMouseWheelMoved },
      { kEventClassWindow,   kEventWindowClosed },
      { kEventClassWindow,   kEventWindowDeactivated }, 
      { kEventClassWindow,   kEventWindowFocusRelinquish },
      { kEventClassKeyboard, kEventRawKeyDown }, 
      { kEventClassKeyboard, kEventRawKeyRepeat }
    };
    
    // Install the event handler
    InstallWindowEventHandler(window, TextEntryHandler, GetEventTypeCount(eventTypes), eventTypes, this, &mTextEntryHandler);
    
    mEdControl = pControl;
    mEdParam = pParam;
    mTextEntryRect = *pTextRect;
  }
}

// static 
pascal OSStatus IGraphicsCarbon::MainEventHandler(EventHandlerCallRef pHandlerCall, EventRef pEvent, void* pGraphicsCarbon)
{
  IGraphicsCarbon* _this = (IGraphicsCarbon*) pGraphicsCarbon;
  IGraphicsMac* pGraphicsMac = _this->mGraphicsMac;
  UInt32 eventClass = GetEventClass(pEvent);
  UInt32 eventKind = GetEventKind(pEvent);
  
  switch (eventClass) {
    case kEventClassKeyboard:
    { 
      switch (eventKind) { 
        case kEventRawKeyDown:{
          
          if (_this->mTextEntryView)
            return eventNotHandledErr;
          
          bool handle = true;
          int key;     
          UInt32 k;
          GetEventParameter(pEvent, kEventParamKeyCode, typeUInt32, NULL, sizeof(UInt32), NULL, &k);
          
          char c;
          GetEventParameter(pEvent, kEventParamKeyMacCharCodes, typeChar, NULL, sizeof(char), NULL, &c);
          
          if (k == 49) key = KEY_SPACE;
          else if (k == 125) key = KEY_DOWNARROW;
          else if (k == 126) key = KEY_UPARROW;
          else if (k == 123) key = KEY_LEFTARROW;
          else if (k == 124) key = KEY_RIGHTARROW;
          else if (c >= '0' && c <= '9') key = KEY_DIGIT_0+c-'0';
          else if (c >= 'A' && c <= 'Z') key = KEY_ALPHA_A+c-'A';
          else if (c >= 'a' && c <= 'z') key = KEY_ALPHA_A+c-'a';
          else handle = false;
          
          if(handle)
            handle = pGraphicsMac->OnKeyDown(_this->mPrevX, _this->mPrevY, key);
          
          if(handle)
            return noErr;
          else
            return eventNotHandledErr;
          
        } 
      }
    }
    case kEventClassControl: 
    {
      switch (eventKind) 
      {          
        case kEventControlDraw: 
        {
          int gfxW = pGraphicsMac->Width(), gfxH = pGraphicsMac->Height();
          
          IRECT r = GetRegionRect(pEvent, gfxW, gfxH);  
          
          CGrafPtr port = 0;
          
          if (_this->mIsComposited) 
          {
            GetEventParameter(pEvent, kEventParamCGContextRef, typeCGContextRef, 0, sizeof(CGContextRef), 0, &(_this->mCGC));         
            CGContextTranslateCTM(_this->mCGC, 0, gfxH);
            CGContextScaleCTM(_this->mCGC, 1.0, -1.0);     
            pGraphicsMac->Draw(&r);
          }
          else
          {
            GetEventParameter(pEvent, kEventParamGrafPort, typeGrafPtr, 0, sizeof(CGrafPtr), 0, &port);
            QDBeginCGContext(port, &(_this->mCGC));
            
            RgnHandle clipRegion = NewRgn();
            GetPortClipRegion(port, clipRegion);
            
            Rect portBounds;
            GetPortBounds(port, &portBounds);
            
            int offsetW = 0;
            
            if ((portBounds.right - portBounds.left) >= gfxW)
            {
              offsetW = 0.5 * ((portBounds.right - portBounds.left) - gfxW);
            }
            
            CGContextTranslateCTM(_this->mCGC, portBounds.left + offsetW, -portBounds.top);
            
            r.L = r.T = r.R = r.B = 0;
            pGraphicsMac->IsDirty(&r);
            pGraphicsMac->Draw(&r);
            
            //CGContextFlush(_this->mCGC);
            QDEndCGContext(port, &(_this->mCGC));           
          }      
          return noErr;
        }
      }
      break;
    }
    case kEventClassMouse: 
    {
      HIPoint hp;
      GetEventParameter(pEvent, kEventParamWindowMouseLocation, typeHIPoint, 0, sizeof(HIPoint), 0, &hp);
      HIPointConvert(&hp, kHICoordSpaceWindow, _this->mWindow, kHICoordSpaceView, _this->mView);
      int x = (int) hp.x - 2;
      int y = (int) hp.y - 3;
      
      UInt32 mods;
      GetEventParameter(pEvent, kEventParamKeyModifiers, typeUInt32, 0, sizeof(UInt32), 0, &mods);
      EventMouseButton button;
      GetEventParameter(pEvent, kEventParamMouseButton, typeMouseButton, 0, sizeof(EventMouseButton), 0, &button);
      if (button == kEventMouseButtonPrimary && (mods & cmdKey)) button = kEventMouseButtonSecondary;
      IMouseMod mmod(true, button == kEventMouseButtonSecondary, (mods & shiftKey), (mods & controlKey), (mods & optionKey));
      
      switch (eventKind) 
      {
        case kEventMouseDown: 
        {
          if (_this->mTextEntryView)
          {
            _this->EndUserInput(true);
          }
          
          #ifdef RTAS_API // RTAS triple click
          if (mmod.L && mmod.R && mmod.C && (pGraphicsMac->GetParamIdxForPTAutomation(x, y) > -1)) 
          {
            return CallNextEventHandler(pHandlerCall, pEvent);
          }
          #endif
          
          CallNextEventHandler(pHandlerCall, pEvent);
          
          UInt32 clickCount = 0;
          GetEventParameter(pEvent, kEventParamClickCount, typeUInt32, 0, sizeof(UInt32), 0, &clickCount);
          
          if (clickCount > 1) 
          {
            pGraphicsMac->OnMouseDblClick(x, y, &mmod);
          }
          else
          {        
            pGraphicsMac->OnMouseDown(x, y, &mmod);
          }
          
          return noErr;
        }
          
        case kEventMouseUp: 
        {
          pGraphicsMac->OnMouseUp(x, y, &mmod);
          return noErr;
        }
          
        case kEventMouseMoved: 
        {
          _this->mPrevX = x;
          _this->mPrevY = y;
          pGraphicsMac->OnMouseOver(x, y, &mmod);
          return noErr;
        }
          
        case kEventMouseDragged: 
        {
          if (!_this->mTextEntryView)
            pGraphicsMac->OnMouseDrag(x, y, &mmod);
          return noErr; 
        }
          
        case kEventMouseWheelMoved: 
        {
          EventMouseWheelAxis axis;
          GetEventParameter(pEvent, kEventParamMouseWheelAxis, typeMouseWheelAxis, 0, sizeof(EventMouseWheelAxis), 0, &axis);
          
          if (axis == kEventMouseWheelAxisY) 
          {
            int d;
            GetEventParameter(pEvent, kEventParamMouseWheelDelta, typeSInt32, 0, sizeof(SInt32), 0, &d);
            
            if (_this->mTextEntryView) _this->EndUserInput(false);
            
            pGraphicsMac->OnMouseWheel(x, y, &mmod, d);
            return noErr;
          }
        }   
      }
      
      break;    
    }
      
    case kEventClassWindow:
    {
      WindowRef window;
      
      if (GetEventParameter (pEvent, kEventParamDirectObject, typeWindowRef, NULL, sizeof (WindowRef), NULL, &window) != noErr)
        break;
      
      switch (eventKind)
      {
        case kEventWindowDeactivated:
        {          
          if (_this->mTextEntryView) 
            _this->EndUserInput(false);          
          break;
        }
      }
      break;
    }
  }
  
  return eventNotHandledErr;
}    

// static 
pascal void IGraphicsCarbon::TimerHandler(EventLoopTimerRef pTimer, void* pGraphicsCarbon)
{
  IGraphicsCarbon* _this = (IGraphicsCarbon*) pGraphicsCarbon;
  
  IRECT r;
  
  if (_this->mGraphicsMac->IsDirty(&r)) 
  {
    if (_this->mIsComposited) 
    {
      CGRect tmp = CGRectMake(r.L, r.T, r.W(), r.H());
      HIViewSetNeedsDisplayInRect(_this->mView, &tmp , true); // invalidate everything that is set dirty
      
      if (_this->mTextEntryView) // validate the text entry rect, otherwise, flicker
      {
        tmp = CGRectMake(_this->mTextEntryRect.L, 
                                _this->mTextEntryRect.T, 
                                _this->mTextEntryRect.W() + 1, 
                                _this->mTextEntryRect.H() + 1);
        HIViewSetNeedsDisplayInRect(_this->mView, &tmp , false);
      }
    }
    else 
    { 
      // TODO: make this more efficient and able to handle the text entry
      UpdateControls(_this->mWindow, 0);
    }
  } 
}

// static
pascal OSStatus IGraphicsCarbon::TextEntryHandler(EventHandlerCallRef pHandlerCall, EventRef pEvent, void* pGraphicsCarbon)
{
  IGraphicsCarbon* _this = (IGraphicsCarbon*) pGraphicsCarbon;
  UInt32 eventClass = GetEventClass(pEvent);
  UInt32 eventKind = GetEventKind(pEvent);

  switch (eventClass)
  {
    case kEventClassKeyboard:
      switch (eventKind)
      {
        case kEventRawKeyDown:
        case kEventRawKeyRepeat:
        {
          // Get the keys and modifiers
          char c;
          UInt32 k;
          UInt32 modifiers;
          GetEventParameter(pEvent, kEventParamKeyMacCharCodes, typeChar,   NULL, sizeof(char),   NULL, &c);
          GetEventParameter(pEvent, kEventParamKeyCode,         typeUInt32, NULL, sizeof(UInt32), NULL, &k);
          GetEventParameter(pEvent, kEventParamKeyModifiers,    typeUInt32, NULL, sizeof(UInt32), NULL, &modifiers);
          
          // paste
          if (c == 118 && modifiers == 256) 
          {
            if (TXNIsScrapPastable()) 
            {
              TXNPaste(_this->mTextEntryView);
              
              return eventNotHandledErr;
            }
          }
          
          // trap enter keys
          if (c == 3 || c == 13) {
            _this->EndUserInput(true);
            return noErr;
          }
          
          // trap escape key
          if (c == 27) {
            _this->EndUserInput(false);
            return noErr;
          }
          
          // pass arrow keys
          if (k == 125 || k == 126 || k == 123 || k == 124) 
            return eventNotHandledErr;
          
          // pass delete keys
          if (c == 8 || c == 127)
            return eventNotHandledErr;
          
          if (_this->mEdParam) 
          {
            switch ( _this->mEdParam->Type() ) 
            {
              case IParam::kTypeEnum:
              case IParam::kTypeInt:
              case IParam::kTypeBool:
                if (c >= '0' && c <= '9') break;
                else if (c == '-') break;
                else if (c == '+') break;
                else return noErr;
              case IParam::kTypeDouble:
                if (c >= '0' && c <= '9') break;
                else if (c == '.') break;
                else if (c == '-') break;
                else if (c == '+') break;
                else return noErr;
              default:
                break;
            }
          }
          
          // Get the text
          CharsHandle textHandle;
          long textLength = 0;
          TXNGetDataEncoded(_this->mTextEntryView, kTXNStartOffset, kTXNEndOffset, &textHandle, kTXNTextData);
          
          // Check that we have some worthwhile data
          if (textHandle != NULL && GetHandleSize(textHandle) > 0)
          {
            textLength = GetHandleSize(textHandle);
          }
          
          if(textLength >= _this->mEdControl->GetTextEntryLength())
          {
            return noErr;
          }
          else 
          {
            EventRecord eventRecord;
            
            if (ConvertEventRefToEventRecord(pEvent, &eventRecord))
            {
              TXNKeyDown(_this->mTextEntryView, &eventRecord);
              return noErr;
            }
          }
        }
        break;
      }
      break;
    case kEventClassMouse:
    {
      switch (eventKind)
      {
        case kEventMouseDown:
        case kEventMouseUp:
        {
          // Get the window handle
          WindowRef window;
          GetEventParameter(pEvent, kEventParamWindowRef, typeWindowRef, NULL, sizeof(WindowRef), NULL, &window);
          
          // Determine the point
          HIPoint p;
          GetEventParameter(pEvent, kEventParamMouseLocation, typeHIPoint, NULL, sizeof(HIPoint), NULL, &p);
          Point point = { (short)p.y, (short)p.x };
          QDGlobalToLocalPoint(GetWindowPort (window), &point);
          
          // Get the viewable area
          Rect rect;
          TXNGetViewRect (_this->mTextEntryView, &rect);
          
          //swell collision
          #undef PtInRect
          #define MacPtInRect PtInRect          
          // Handle the click as necessary
          if (PtInRect(point, &rect))
          {
            #define PtInRect(r,p) SWELL_PtInRect(r,p)
            EventRecord eventRecord;
            if (eventKind == kEventMouseDown && ConvertEventRefToEventRecord(pEvent, &eventRecord))
            {
              TXNClick(_this->mTextEntryView, &eventRecord);
            }
            return noErr;
          }
          else
          {
            CallNextEventHandler(pHandlerCall, pEvent);
            ClearKeyboardFocus(window);
            _this->EndUserInput(false);
            return noErr;
          }
        }
        break;
        case kEventMouseMoved:
          TXNAdjustCursor(_this->mTextEntryView, NULL);
          return noErr;
        case kEventMouseWheelMoved:
          return noErr;
      }
      break;
    }
    case kEventClassWindow:
    {
      WindowRef window;
      GetEventParameter (pEvent, kEventParamDirectObject, typeWindowRef, NULL, sizeof(WindowRef), NULL, &window);
      switch (eventKind)
      {
        case kEventWindowFocusRelinquish:
        case kEventWindowClosed:
        case kEventWindowDeactivated:
          CallNextEventHandler(pHandlerCall, pEvent);
          ClearKeyboardFocus(window);
          _this->EndUserInput(false);
          return noErr;
      }
      break;
    }
  }
  
  return eventNotHandledErr;
}

#endif // IPLUG_NO_CARBON_SUPPORT
