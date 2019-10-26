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
- (void)sendControlValueFromDelegate: (int) ctrlTag : (double) normalizedValue NS_SWIFT_NAME(sendControlValueFromDelegate(ctrlTag:normalizedValue:));
- (void)sendControlMsgFromDelegate: (int) ctrlTag : (int) msgTag : (int) dataSize : (const void*) pData NS_SWIFT_NAME(sendControlMsgFromDelegate(ctrlTag:msgTag:dataSize:data:));
- (void)sendParameterValueFromDelegate: (int) paramIdx : (double) value : (BOOL) normalized NS_SWIFT_NAME(sendParameterValueFromDelegate(paramIdx:value:isNormalized:));

- (void)sendParameterValueFromUI: (int) paramIdx : (double) normalizedValue NS_SWIFT_NAME(sendParameterValueFromUI(paramIdx:normalizedValue:));
- (void)beginInformHostOfParamChangeFromUI: (int) paramIdx NS_SWIFT_NAME(beginInformHostOfParamChangeFromUI(paramIdx:));
- (void)endInformHostOfParamChangeFromUI: (int) paramIdx NS_SWIFT_NAME(endInformHostOfParamChangeFromUI(paramIdx:));

- (void)sendMidiMsgFromUI: (UInt8) status : (UInt8) data1 : (UInt8) data2 : (int) offset NS_SWIFT_NAME(sendMidiMsgFromUI(status:data1:data2:offset:));
- (void)sendSysexMsgFromUI: (NSData*) msg : (int) offset NS_SWIFT_NAME(sendSysexMsgFromUI(msg:offset:));
- (void)sendArbitraryMsgFromUI: (int) msgTag : (int) ctrlTag : (NSData*) msg NS_SWIFT_NAME(sendArbitraryMsgFromUI(msgTag:ctrlTag:msg:));
@end
