//Avoid any C++ includes in this file for swift interop

#include <TargetConditionals.h>

#if defined TARGET_OS_MAC
  #if TARGET_OS_IPHONE
    #import <UIKit/UIKit.h>
    #define VIEW UIView
    #define MAKERECT CGRectMake
  #else
    #define VIEW NSView
    #define MAKERECT NSMakeRect
    #import <Cocoa/Cocoa.h>
  #endif
#endif

#if defined AUv3_API
  #import "IPlugAUViewController.h"
  #define VIEW_CONTROLLER IPlugAUViewController
#else
  #if defined TARGET_OS_MAC
    #define VIEW_CONTROLLER NSViewController
  #elif defined TARGET_OS_IPHONE
    #define VIEW_CONTROLLER UIViewController
  #endif
#endif

/** An objc view controller base which reproduces some functionality from EditorDelegate in objc */
@interface IPlugCocoaViewController : VIEW_CONTROLLER
{
  void* editorDelegate;
}

- (void)setEditorDelegate: (void*) editorDelegate;

- (BOOL)onMessage: (int) msgTag : (int) ctrlTag : (NSData*) msg;
- (void)onParamChangeUI: (int) paramIdx : (double) value;
- (void)onMidiMsgUI: (UInt8) status : (UInt8) data1 : (UInt8) data2 : (int) offset;
- (void)onSysexMsgUI: (NSData*) msg : (int) offset;
- (void)sendControlValueFromDelegate: (int) ctrlTag : (double) normalizedValue;
- (void)sendControlMsgFromDelegate: (int) ctrlTag : (int) msgTag : (int) dataSize : (const void*) pData;
- (void)sendParameterValueFromDelegate: (int) paramIdx : (double) value : (BOOL) normalized;

- (void)sendParameterValueFromUI: (int) paramIdx : (double) normalizedValue;
- (void)beginInformHostOfParamChangeFromUI: (int) paramIdx;
- (void)endInformHostOfParamChangeFromUI: (int) paramIdx;
- (void)sendMidiMsgFromUI: (UInt8) status : (UInt8) data1 : (UInt8) data2 : (int) offset;
- (void)sendSysexMsgFromUI: (NSData*) msg : (int) offset;
- (void)sendArbitraryMsgFromUI: (int) msgTag : (int) ctrlTag : (NSData*) msg;
@end
