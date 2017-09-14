#include "IGraphicsCarbon.h"
#ifndef IPLUG_NO_CARBON_SUPPORT

IRECT GetRegionRect(EventRef pEvent, int gfxW, int gfxH)
{
#if MAC_OS_X_VERSION_MAX_ALLOWED <= 1060
  RgnHandle pRgn = 0;
  if (GetEventParameter(pEvent, kEventParamRgnHandle, typeQDRgnHandle, 0, sizeof(RgnHandle), 0, &pRgn) == noErr && pRgn)
  {
    Rect rct;
    GetRegionBounds(pRgn, &rct);
    return IRECT(rct.left, rct.top, rct.right, rct.bottom);
  }
#endif
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

#if MAC_OS_X_VERSION_MAX_ALLOWED <= 1040
typedef UInt32 URefCon;
#endif

IGraphicsCarbon::IGraphicsCarbon(IGraphicsMac* pGraphicsMac,
                                 WindowRef pWindow,
                                 ControlRef pParentControl,
                                 short leftOffset,
                                 short topOffset)
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
  , mLeftOffset(leftOffset)
  , mTopOffset(topOffset)
  , mShowingTooltip(false)
  , mTooltipIdx(-1)
  , mTooltipTimer(0)
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

    #if USE_MLTE
    TXNFocus(mTextEntryView, false);
    TXNClear(mTextEntryView);
    TXNDeleteObject(mTextEntryView);
    #else
    HIViewRemoveFromSuperview(mTextEntryView);
    #endif

    mTextEntryView = 0;
    mEdControl = 0;
    mEdParam = 0;
  }

  HideTooltip();
  RemoveEventLoopTimer(mTimer);
  RemoveEventHandler(mControlHandler);
  RemoveEventHandler(mWindowHandler);
  mTimer = 0;
  mView = 0;
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

MenuRef IGraphicsCarbon::CreateMenu(IPopupMenu* pMenu)
{
  MenuRef menuRef = 0;
  ResID menuID = UniqueID ('MENU');

  int numItems = pMenu->GetNItems();

  if (numItems && CreateNewMenu(menuID, kMenuAttrCondenseSeparators, &menuRef) == noErr)
  {
    for (int i = 0; i < numItems; ++i)
    {
      IPopupMenuItem* menuItem = pMenu->GetItem(i);

      if (menuItem->GetIsSeparator())
      {
        AppendMenuItemTextWithCFString(menuRef, CFSTR(""), kMenuItemAttrSeparator, 0, NULL);
      }
      else
      {
        CFStringRef itemString = CFStringCreateWithCString(NULL, menuItem->GetText(), kCFStringEncodingUTF8);

        if (pMenu->GetPrefix())
        {
          CFStringRef prefixString = 0;

          switch (pMenu->GetPrefix())
          {
            case 0:
              prefixString = CFStringCreateWithCString(NULL, "", kCFStringEncodingUTF8); break;
            case 1:
              prefixString = CFStringCreateWithFormat(NULL, 0, CFSTR("%1d: "),i+1); break;
            case 2:
              prefixString = CFStringCreateWithFormat(NULL, 0, CFSTR("%02d: "),i+1); break;
            case 3:
              prefixString = CFStringCreateWithFormat(NULL, 0, CFSTR("%03d: "),i+1); break;
          }

          CFMutableStringRef newItemString = CFStringCreateMutable(0, 0);
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
        {
          itemAttribs |= kMenuItemAttrDisabled;
        }

        if (menuItem->GetIsTitle())
        {
          itemAttribs |= kMenuItemAttrSectionHeader;
        }

        InsertMenuItemTextWithCFString(menuRef, itemString, i, itemAttribs, 0);

        if (menuItem->GetChecked())
        {
          CheckMenuItem(menuRef, i+1, true);
        }

        if (menuItem->GetSubmenu())
        {
          MenuRef submenu = CreateMenu(menuItem->GetSubmenu());

          if (submenu)
          {
            SetMenuItemHierarchicalMenu(menuRef, i+1, submenu);
            CFRelease (submenu);
          }
        }

        CFRelease (itemString);
      }
    }

    //  if (pMenu->getStyle() & kCheckStyle && !multipleCheck)
    //    CheckMenuItem (menuRef, pMenu->getCurrentIndex (true) + 1, true);
    SetMenuItemRefCon(menuRef, 0, (int32_t)pMenu);
    //swell collision
    #undef InsertMenu
    InsertMenu(menuRef, kInsertHierarchicalMenu);
    #define InsertMenu SWELL_InsertMenu
  }

  return menuRef;
}

IPopupMenu* IGraphicsCarbon::CreateIPopupMenu(IPopupMenu* pMenu, IRECT* pAreaRect)
{
  // Get the plugin gui frame rect within the host's window
  HIRect rct;
  HIViewGetFrame(this->mView, &rct);

  // Get the host's window rect within the screen
  Rect wrct;
  GetWindowBounds(this->mWindow, kWindowContentRgn, &wrct);

  #ifdef RTAS_API
  int xpos = wrct.left + this->GetLeftOffset() + pAreaRect->L;
  int ypos = wrct.top + this->GetTopOffset() + pAreaRect->B + 5;
  #else
  HIViewRef contentView;
  HIViewFindByID(HIViewGetRoot(this->mWindow), kHIViewWindowContentID, &contentView);
  HIViewConvertRect(&rct, HIViewGetSuperview((HIViewRef)this->mView), contentView);

  int xpos = wrct.left + rct.origin.x + pAreaRect->L;
  int ypos = wrct.top + rct.origin.y + pAreaRect->B + 5;
  #endif

  MenuRef menuRef = CreateMenu(pMenu);

  if (menuRef)
  {
    int32_t popUpItem = 1;
    int32_t PopUpMenuItem = PopUpMenuSelect(menuRef, ypos, xpos, popUpItem);

    short result = LoWord(PopUpMenuItem) - 1;
    short menuIDResult = HiWord(PopUpMenuItem);
    IPopupMenu* resultMenu = 0;

    if (menuIDResult != 0)
    {
      MenuRef usedMenuRef = GetMenuHandle(menuIDResult);

      if (usedMenuRef)
      {
        if (GetMenuItemRefCon(usedMenuRef, 0, (URefCon*)&resultMenu) == noErr)
        {
          resultMenu->SetChosenItemIdx(result);
        }
      }
    }

    CFRelease(menuRef);

    return resultMenu;
  }
  else
  {
    return 0;
  }
}

// static
pascal OSStatus IGraphicsCarbon::MainEventHandler(EventHandlerCallRef pHandlerCall, EventRef pEvent, void* pGraphicsCarbon)
{
  IGraphicsCarbon* _this = (IGraphicsCarbon*) pGraphicsCarbon;
  IGraphicsMac* pGraphicsMac = _this->mGraphicsMac;
  UInt32 eventClass = GetEventClass(pEvent);
  UInt32 eventKind = GetEventKind(pEvent);

  switch (eventClass)
  {
    case kEventClassKeyboard:
    {
      switch (eventKind)
      {
        case kEventRawKeyDown:
        {
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

          if (_this->mIsComposited)
          {
            GetEventParameter(pEvent, kEventParamCGContextRef, typeCGContextRef, 0, sizeof(CGContextRef), 0, &(_this->mCGC));
            CGContextTranslateCTM(_this->mCGC, 0, gfxH);
            CGContextScaleCTM(_this->mCGC, 1.0, -1.0);
            pGraphicsMac->Draw(&r);
          }
#if MAC_OS_X_VERSION_MAX_ALLOWED <= 1060
          else
          {
            CGrafPtr port = 0;
            
            GetEventParameter(pEvent, kEventParamGrafPort, typeGrafPtr, 0, sizeof(CGrafPtr), 0, &port);
            QDBeginCGContext(port, &(_this->mCGC));
            
            Rect portBounds;
            GetPortBounds(port, &portBounds);

            int offsetW = 0;
            int offsetH = -portBounds.top;
            
            if ((portBounds.right - portBounds.left) >= gfxW)
            {
              offsetW = 0.5 * ((portBounds.right - portBounds.left) - gfxW);
            }
            
            CGContextTranslateCTM(_this->mCGC, portBounds.left + offsetW, offsetH);
            
            r = IRECT(0, 0, pGraphicsMac->Width(), pGraphicsMac->Height());
            pGraphicsMac->Draw(&r); // Carbon non-composited will redraw everything, the IRECT passed here is the entire plugin-gui
            
            QDEndCGContext(port, &(_this->mCGC));
          }
#endif
          return noErr;
        }
      }
      break;
    }
    case kEventClassMouse:
    {
      HIPoint hp;
      GetEventParameter(pEvent, kEventParamWindowMouseLocation, typeHIPoint, 0, sizeof(HIPoint), 0, &hp);

      #ifdef RTAS_API
      // Header offset
      hp.x -= _this->GetLeftOffset();
      hp.y -= _this->GetTopOffset();

      Rect bounds;
      GetWindowBounds(_this->mWindow, kWindowTitleBarRgn, &bounds);

      // adjust x mouse coord if the gui is less wide than the window
//      int windowWidth = (bounds.right - bounds.left);
//
//      if (windowWidth > pGraphicsMac->Width())
//      {
//        hp.x -= (int) floor((windowWidth - pGraphicsMac->Width()) / 2.);
//      }

      // Title bar Y offset
      hp.y -= bounds.bottom - bounds.top;

      int x = (int) hp.x;
      int y = (int) hp.y;

      #else // NOT RTAS
      HIPointConvert(&hp, kHICoordSpaceWindow, _this->mWindow, kHICoordSpaceView, _this->mView);
      int x = (int) hp.x - 2;
      int y = (int) hp.y - 3;
      #endif

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
           _this->HideTooltip();
          
          if (_this->mTextEntryView)
          {
            #if !(USE_MLTE)
            HIViewRef view;
            HIViewGetViewForMouseEvent(_this->mView, pEvent, &view);
            if (view == _this->mTextEntryView) break;
            #endif
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
          
          if (pGraphicsMac->TooltipsEnabled()) 
          {
            int c = pGraphicsMac->GetMouseOver();
            if (c != _this->mTooltipIdx) 
            {
              _this->mTooltipIdx = c;
              _this->HideTooltip();
              const char* tooltip = c >= 0 ? pGraphicsMac->GetControl(c)->GetTooltip() : NULL;
              if (CSTR_NOT_EMPTY(tooltip)) 
              {
                _this->mTooltip = tooltip;
                _this->mTooltipTimer = pGraphicsMac->FPS() * 3 / 2;  //TODO: remove FPS link
              }
            }
          }          
          
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

      if (GetEventParameter(pEvent, kEventParamDirectObject, typeWindowRef, NULL, sizeof (WindowRef), NULL, &window) != noErr)
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

      #if USE_MTLE
      if (_this->mTextEntryView) // validate the text entry rect, otherwise, flicker
      {
        tmp = CGRectMake(_this->mTextEntryRect.L,
                         _this->mTextEntryRect.T,
                         _this->mTextEntryRect.W() + 1,
                         _this->mTextEntryRect.H() + 1);
        HIViewSetNeedsDisplayInRect(_this->mView, &tmp , false);
      }
      #endif
    }
    else
    {
//      int h = _this->mGraphicsMac->Height();
//      SetRectRgn(_this->mRgn, r.L, h - r.B, r.R, h - r.T);
//      UpdateControls(_this->mWindow, _this->mRgn);
      UpdateControls(_this->mWindow, 0);
    }
  }
  
  if (_this->mTooltipTimer) 
  {    
    if (!(--_this->mTooltipTimer)) 
    {
      if (!_this->mShowingTooltip) 
      {
        _this->ShowTooltip();
        _this->mTooltipTimer = _this->mGraphicsMac->FPS() * 10; // TODO: remove FPS link
      }
      else 
      {
        _this->HideTooltip();
      }
    }
  }
}

#pragma mark -
#pragma mark MLTE text entry methods

#if USE_MLTE

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
                rct.origin.x + pTextRect->R + 1
              };

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

    // font (NOT working)
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

void IGraphicsCarbon::EndUserInput(bool commit)
{
  if (mTextEntryHandler)
  {
    RemoveEventHandler(mTextEntryHandler);
    mTextEntryHandler = 0;
  }
  else
  {
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
          if (c == 3 || c == 13)
          {
            _this->EndUserInput(true);
            return noErr;
          }

          // trap escape key
          if (c == 27)
          {
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

#else

#pragma mark -
#pragma mark HIView text entry methods

void IGraphicsCarbon::CreateTextEntry(IControl* pControl, IText* pText, IRECT* pTextRect, const char* pString, IParam* pParam)
{
  ControlRef control = 0;

  if (!pControl || mTextEntryView || !mIsComposited) return;

  Rect r = { static_cast<short>(pTextRect->T), static_cast<short>(pTextRect->L), static_cast<short>(pTextRect->B), static_cast<short>(pTextRect->R) };

  // these adjustments should make it the same as the cocoa one, i.e. the same size as the pTextRect, but with the extra blue rim often this is too small
  //Rect r = { pTextRect->T+4, pTextRect->L+3, pTextRect->B-3, pTextRect->R -3 };

  if (CreateEditUnicodeTextControl(NULL, &r, NULL, false, NULL, &control) != noErr) return;

  HIViewAddSubview(mView, control);

  const EventTypeSpec events[] =
  {
    { kEventClassKeyboard, kEventRawKeyDown },
    { kEventClassKeyboard, kEventRawKeyRepeat }
  };

  InstallControlEventHandler(control, TextEntryHandler, GetEventTypeCount(events), events, this, &mTextEntryHandler);
  mTextEntryView = control;

  if (pString[0] != '\0')
  {
    CFStringRef str = CFStringCreateWithCString(NULL, pString, kCFStringEncodingUTF8);

    if (str)
    {
      SetControlData(mTextEntryView, kControlEditTextPart, kControlEditTextCFStringTag, sizeof(str), &str);
      CFRelease(str);
    }

    ControlEditTextSelectionRec sel;
    sel.selStart = 0;
    sel.selEnd = strlen(pString);
    SetControlData(mTextEntryView, kControlEditTextPart, kControlEditTextSelectionTag, sizeof(sel), &sel);
  }

  int just = 0;

  switch ( pText->mAlign )
  {
    case IText::kAlignNear:
      just = teJustLeft;
      break;
    case IText::kAlignCenter:
      just = teCenter;
      break;
    case IText::kAlignFar:
      just = teJustRight;
      break;
    default:
      just = teCenter;
      break;
  }

  ControlFontStyleRec font = { kControlUseJustMask | kControlUseSizeMask | kControlUseFontMask, 0, static_cast<SInt16>(pText->mSize), 0, 0, static_cast<SInt16>(just), 0, 0 };
  CFStringRef str = CFStringCreateWithCString(NULL, pText->mFont, kCFStringEncodingUTF8);
  font.font = ATSFontFamilyFindFromName(str, kATSOptionFlagsDefault);

  SetControlData(mTextEntryView, kControlEditTextPart, kControlFontStyleTag, sizeof(font), &font);
  CFRelease(str);

  Boolean singleLineStyle = true;
  SetControlData(mTextEntryView, kControlEditTextPart, kControlEditTextSingleLineTag, sizeof (Boolean), &singleLineStyle);

  HIViewSetVisible(mTextEntryView, true);
  HIViewAdvanceFocus(mTextEntryView, 0);
  SetKeyboardFocus(mWindow, mTextEntryView, kControlEditTextPart);
  SetUserFocusWindow(mWindow);

  mEdControl = pControl;
  mEdParam = pParam;
}

void IGraphicsCarbon::EndUserInput(bool commit)
{
  RemoveEventHandler(mTextEntryHandler);
  mTextEntryHandler = 0;

  if (commit)
  {
    CFStringRef str;
    if (GetControlData(mTextEntryView, kControlEditTextPart, kControlEditTextCFStringTag, sizeof(str), &str, NULL) == noErr)
    {
      char txt[MAX_PARAM_LEN];
      CFStringGetCString(str, txt, MAX_PARAM_LEN, kCFStringEncodingUTF8);
      CFRelease(str);

      if (mEdParam)
        mGraphicsMac->SetFromStringAfterPrompt(mEdControl, mEdParam, txt);
      else
        mEdControl->TextFromTextEntry(txt);
    }
  }

  HIViewSetVisible(mTextEntryView, false);
  HIViewRemoveFromSuperview(mTextEntryView);

  if (mIsComposited)
  {
    //IRECT* pR = mEdControl->GetRECT();
    //HIViewSetNeedsDisplayInRect(mView, &CGRectMake(pR->L, pR->T, pR->W(), pR->H()), true);
    HIViewSetNeedsDisplay(mView, true);
  }
  else
  {
    if (mEdControl) 
    {
      mEdControl->SetDirty(false);
      mEdControl->Redraw();
    }
  }
  
  SetThemeCursor(kThemeArrowCursor);
  SetUserFocusWindow(kUserFocusAuto);

  mTextEntryView = 0;
  mEdControl = 0;
  mEdParam = 0;
}

pascal OSStatus IGraphicsCarbon::TextEntryHandler(EventHandlerCallRef pHandlerCall, EventRef pEvent, void* pGraphicsCarbon)
{
  IGraphicsCarbon* _this = (IGraphicsCarbon*) pGraphicsCarbon;
  UInt32 eventClass = GetEventClass(pEvent);
  UInt32 eventKind = GetEventKind(pEvent);

  switch (eventClass)
  {
    case kEventClassKeyboard:
    {
      switch (eventKind)
      {
        case kEventRawKeyDown:
        case kEventRawKeyRepeat:
        {
          char c;
          GetEventParameter(pEvent, kEventParamKeyMacCharCodes, typeChar, NULL, sizeof(c), NULL, &c);
          UInt32 k;
          GetEventParameter(pEvent, kEventParamKeyCode, typeUInt32, NULL, sizeof(UInt32), NULL, &k);

          // trap enter key
          if (c == 3 || c == 13)
          {
            _this->EndUserInput(true);
            return noErr;
          }

          // pass arrow keys
          if (k == 125 || k == 126 || k == 123 || k == 124)
            break;

          // pass delete keys
          if (c == 8 || c == 127)
            break;

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

          // check the length of the text
          Str31  theText;
          Size   textLength;

          GetControlData(_this->mTextEntryView,
                         kControlNoPart,
                         kControlEditTextTextTag,
                         sizeof(theText) -1,
                         (Ptr) &theText[1],
                         &textLength);

          if(textLength >= _this->mEdControl->GetTextEntryLength())
            return noErr;

          break;
        }
      }
      break;
    }
  }
  return eventNotHandledErr;
}

#endif // USE_MLTE

void IGraphicsCarbon::ShowTooltip()
{
  HMHelpContentRec helpTag;
  helpTag.version = kMacHelpVersion;

  helpTag.tagSide = kHMInsideTopLeftCorner;
  HIRect r = CGRectMake(mGraphicsMac->GetMouseX(), mGraphicsMac->GetMouseY() + 23, 1, 1);
  HIRectConvert(&r, kHICoordSpaceView, mView, kHICoordSpaceScreenPixel, NULL);
  helpTag.absHotRect.top = (int)r.origin.y;
  helpTag.absHotRect.left = (int)r.origin.x;
  helpTag.absHotRect.bottom = helpTag.absHotRect.top + (int)r.size.height;
  helpTag.absHotRect.right = helpTag.absHotRect.left + (int)r.size.width;
  
  helpTag.content[kHMMinimumContentIndex].contentType = kHMCFStringLocalizedContent;
  CFStringRef str = CFStringCreateWithCString(NULL, mTooltip, kCFStringEncodingUTF8);
  helpTag.content[kHMMinimumContentIndex].u.tagCFString = str;
  helpTag.content[kHMMaximumContentIndex].contentType = kHMNoContent;
  HMDisplayTag(&helpTag);
  CFRelease(str);
  mShowingTooltip = true;
}

void IGraphicsCarbon::HideTooltip()
{
  mTooltipTimer = 0;
  if (mShowingTooltip) 
  {
    HMHideTag();
    mShowingTooltip = false;
  }
}

#endif // IPLUG_NO_CARBON_SUPPORT
