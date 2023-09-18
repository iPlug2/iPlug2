 /*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#import <CoreAudioKit/AUViewController.h>
#import "IPlugAUAudioUnit.h"
#import "IPlugAUViewController.h"
#include "IPlugPlatform.h"
#include "IPlugLogger.h"
#include "IPlugConstants.h"

#if !__has_feature(objc_arc)
#error This file must be compiled with Arc. Use -fobjc-arc flag
#endif

@interface IPLUG_AUVIEWCONTROLLER (AUAudioUnitFactory)
@end

@implementation IPLUG_AUVIEWCONTROLLER

- (AUAudioUnit*) createAudioUnitWithComponentDescription:(AudioComponentDescription) desc error:(NSError **)error
{
  self.audioUnit = [[IPLUG_AUAUDIOUNIT alloc] initWithComponentDescription:desc error:error];

  [self audioUnitInitialized];

  return self.audioUnit;
}

#ifdef OS_IOS
- (void) viewDidLayoutSubviews
{
  if (self.audioUnit)
  {
    [(IPLUG_AUAUDIOUNIT*) self.audioUnit hostResized: self.view.frame.size];
  }
}

- (void) viewWillAppear:(BOOL) animated
{
  [super viewWillAppear:animated];
  
  if (self.audioUnit)
  {
    [(IPLUG_AUAUDIOUNIT*) self.audioUnit openWindow:self.view];
  }
}

- (void) viewDidDisappear:(BOOL) animated
{
  [super viewDidDisappear:animated];
  
  if (self.audioUnit)
  {
    [(IPLUG_AUAUDIOUNIT*) self.audioUnit closeWindow];
  }
}
#else
- (void) viewDidLayout
{
  if (self.audioUnit)
  {
    [(IPLUG_AUAUDIOUNIT*) self.audioUnit hostResized: self.view.frame.size];
  }
}

- (void) viewWillAppear
{
  [(IPLUG_AUAUDIOUNIT*) self.audioUnit openWindow:self.view];
}

- (void) viewDidDisappear
{
  [(IPLUG_AUAUDIOUNIT*) self.audioUnit closeWindow];
}

- (instancetype) initWithNibName:(NSNibName)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
  self = [super initWithNibName:nibNameOrNil bundle:[NSBundle bundleForClass:self.class]];
  
  return self;
}


#endif

- (AUAudioUnit*) getAudioUnit
{
  return self.audioUnit;
}

- (void) audioUnitInitialized
{
  dispatch_async(dispatch_get_main_queue(), ^{
    if (self.audioUnit)
    {
      int width = (int) [(IPLUG_AUAUDIOUNIT*) self.audioUnit width];
      int height = (int) [(IPLUG_AUAUDIOUNIT*) self.audioUnit height];
      self.preferredContentSize = CGSizeMake(width, height);
    }
  });
}

#ifdef OS_IOS
static int AppleKeyCodeToVK(UIKeyboardHIDUsage code)
{
  using namespace iplug;
  
  switch (code)
  {
//    case 51: return kVK_BACK;
//    case 65: return kVK_DECIMAL;
//    case 67: return kVK_MULTIPLY;
//    case 69: return kVK_ADD;
//    case 71: return kVK_NUMLOCK;
//    case 75: return kVK_DIVIDE;
//    case 76: return kVK_RETURN | 0x8000;
//    case 78: return kVK_SUBTRACT;
//    case 81: return kVK_SEPARATOR;
    case UIKeyboardHIDUsageKeypad1 : return kVK_NUMPAD0;
    case UIKeyboardHIDUsageKeypad2 : return kVK_NUMPAD1;
    case UIKeyboardHIDUsageKeypad3 : return kVK_NUMPAD2;
    case UIKeyboardHIDUsageKeypad4 : return kVK_NUMPAD3;
    case UIKeyboardHIDUsageKeypad5 : return kVK_NUMPAD4;
    case UIKeyboardHIDUsageKeypad6 : return kVK_NUMPAD5;
    case UIKeyboardHIDUsageKeypad7 : return kVK_NUMPAD6;
    case UIKeyboardHIDUsageKeypad8 : return kVK_NUMPAD7;
    case UIKeyboardHIDUsageKeypad9 : return kVK_NUMPAD8;
    case UIKeyboardHIDUsageKeypad0 : return kVK_NUMPAD9;
    case UIKeyboardHIDUsageKeyboardF1 : return kVK_F1;
    case UIKeyboardHIDUsageKeyboardF2 : return kVK_F2;
    case UIKeyboardHIDUsageKeyboardF3 : return kVK_F3;
    case UIKeyboardHIDUsageKeyboardF4 : return kVK_F4;
    case UIKeyboardHIDUsageKeyboardF5 : return kVK_F5;
    case UIKeyboardHIDUsageKeyboardF6 : return kVK_F6;
    case UIKeyboardHIDUsageKeyboardF7 : return kVK_F7;
    case UIKeyboardHIDUsageKeyboardF8 : return kVK_F8;
    case UIKeyboardHIDUsageKeyboardF9 : return kVK_F9;
    case UIKeyboardHIDUsageKeyboardF10: return kVK_F10;
    case UIKeyboardHIDUsageKeyboardF11: return kVK_F11;
    case UIKeyboardHIDUsageKeyboardF12: return kVK_F12;

//    case 114: return kVK_INSERT;
//    case 115: return kVK_HOME;
//    case 117: return kVK_DELETE;
//    case 116: return kVK_PRIOR;
//    case 118: return kVK_F4;
//    case 119: return kVK_END;
//    case 120: return kVK_F2;
//    case 121: return kVK_NEXT;
//    case 122: return kVK_F1;
    case UIKeyboardHIDUsageKeyboardLeftArrow: return kVK_LEFT;
    case UIKeyboardHIDUsageKeyboardRightArrow: return kVK_RIGHT;
    case UIKeyboardHIDUsageKeyboardDownArrow: return kVK_DOWN;
    case UIKeyboardHIDUsageKeyboardUpArrow: return kVK_UP;
//    case 0x69: return kVK_F13;
//    case 0x6B: return kVK_F14;
//    case 0x71: return kVK_F15;
//    case 0x6A: return kVK_F16;
  }
  return kVK_NONE;
}

static int UIPressToVK(UIPress* pEvent, int& flag)
{
  using namespace iplug;
  
  int code = kVK_NONE;
  
  const NSInteger mod = [[pEvent key] modifierFlags];
  
  if (mod & UIKeyModifierShift) flag |= kFSHIFT;
  if (mod & UIKeyModifierCommand) flag |= kFCONTROL;
  if (mod & UIKeyModifierAlternate) flag |= kFALT;
  if ((mod & UIKeyModifierControl) /*&& !IsRightClickEmulateEnabled()*/) flag |= kFLWIN;

  UIKeyboardHIDUsage rawcode = [[pEvent key] keyCode];
  
  code = AppleKeyCodeToVK(rawcode);
  if (code == kVK_NONE)
  {
    NSString *str = NULL;
    
    if (!str || ![str length]) str = [[pEvent key] charactersIgnoringModifiers];
    
    if (!str || ![str length])
    {
      if (!code)
      {
        code = 1024 + rawcode; // raw code
        flag |= kFVIRTKEY;
      }
    }
    else
    {
      constexpr int NSF1FunctionKey = 0xF704;
      constexpr int NSF24FunctionKey = 0xF71B;
      
      code = [str characterAtIndex:0];
      if (code >= NSF1FunctionKey && code <= NSF24FunctionKey)
      {
        flag |= kFVIRTKEY;
        code += kVK_F1 - NSF1FunctionKey;
      }
      else
      {
        if (code >= 'a' && code <= 'z') code += 'A'-'a';
        if (code == 25 && (flag & FSHIFT)) code = kVK_TAB;
        if (isalnum(code) || code==' ' || code == '\r' || code == '\n' || code ==27 || code == kVK_TAB) flag |= kFVIRTKEY;
      }
    }
  }
  else
  {
    flag |= kFVIRTKEY;
    if (code == 8) code = '\b';
  }
  
  if (!(flag & kFVIRTKEY)) flag &= ~kFSHIFT;
  
  return code;
}

- (void)pressesBegan:(NSSet<UIPress *> *)presses withEvent:(nullable UIPressesEvent *)event
{
  for (UIPress *press in presses)
  {
    int flag = 0;
    int keycode = UIPressToVK(press, flag);
    unichar c = 0;
    
    NSString *s = [[press key] charactersIgnoringModifiers];

    if ([s length] == 1)
      c = [s characterAtIndex:0];

    if (self.audioUnit)
    {
      [(IPLUG_AUAUDIOUNIT*) self.audioUnit vcKeyDown:keycode : flag : c];
    }
  }
}

- (void)pressesEnded:(NSSet<UIPress *> *)presses withEvent:(nullable UIPressesEvent *)event
{
  for (UIPress *press in presses)
  {
    int flag = 0;
    int keycode = UIPressToVK(press, flag);
    unichar c = 0;
    
    NSString *s = [[press key] charactersIgnoringModifiers];

    if ([s length] == 1)
      c = [s characterAtIndex:0];

    if (self.audioUnit)
    {
      [(IPLUG_AUAUDIOUNIT*) self.audioUnit vcKeyUp:keycode : flag : c];
    }
  }
}
#endif


@end
