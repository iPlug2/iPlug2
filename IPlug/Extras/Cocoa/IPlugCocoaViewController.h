//Avoid any C++ includes in this file for swift interop

#include <TargetConditionals.h>

#if defined TARGET_OS_MAC
  #if TARGET_OS_IPHONE
    #import <UIKit/UIKit.h>
    #define PLATFORM_VIEW UIView
    #define PLATFORM_VC UIViewController
    #define MAKERECT CGRectMake
  #else
    #import <Cocoa/Cocoa.h>
    #define PLATFORM_VIEW NSView
    #define PLATFORM_VC NSViewController
    #define MAKERECT NSMakeRect
  #endif
#endif

/** An objc view controller base which reproduces some functionality from EditorDelegate in objc */
@interface IPlugCocoaViewController : PLATFORM_VC
{
  void* editorDelegate;
}

- (void)setEditorDelegate: (void*) editorDelegate;

- (BOOL) onMessage: (NSInteger) msgTag : (NSInteger) ctrlTag : (NSData*) msg;
- (void) onParamChangeUI: (NSInteger) paramIdx : (double) value;
- (void) onMidiMsgUI: (UInt8) status : (UInt8) data1 : (UInt8) data2 : (NSInteger) offset;
- (void) onSysexMsgUI: (NSData*) msg : (NSInteger) offset;
- (void) sendControlValueFromDelegate: (NSInteger) ctrlTag : (double) normalizedValue NS_SWIFT_NAME(sendControlValueFromDelegate(ctrlTag:normalizedValue:));
- (void) sendControlMsgFromDelegate: (NSInteger) ctrlTag : (NSInteger) msgTag : (NSData*) msg NS_SWIFT_NAME(sendControlMsgFromDelegate(ctrlTag:msgTag:msg:));
- (void) sendParameterValueFromDelegate: (NSInteger) paramIdx : (double) value : (BOOL) normalized NS_SWIFT_NAME(sendParameterValueFromDelegate(paramIdx:value:isNormalized:));

- (void) sendParameterValueFromUI: (NSInteger) paramIdx : (double) normalizedValue NS_SWIFT_NAME(sendParameterValueFromUI(paramIdx:normalizedValue:));
- (void) beginInformHostOfParamChangeFromUI: (NSInteger) paramIdx NS_SWIFT_NAME(beginInformHostOfParamChangeFromUI(paramIdx:));
- (void) endInformHostOfParamChangeFromUI: (NSInteger) paramIdx NS_SWIFT_NAME(endInformHostOfParamChangeFromUI(paramIdx:));

- (void) sendMidiMsgFromUI: (UInt8) status : (UInt8) data1 : (UInt8) data2 : (NSInteger) offset NS_SWIFT_NAME(sendMidiMsgFromUI(status:data1:data2:offset:));
- (void) sendSysexMsgFromUI: (NSData*) msg : (NSInteger) offset NS_SWIFT_NAME(sendSysexMsgFromUI(msg:offset:));
- (void) sendArbitraryMsgFromUI: (NSInteger) msgTag : (NSInteger) ctrlTag : (NSData*) msg NS_SWIFT_NAME(sendArbitraryMsgFromUI(msgTag:ctrlTag:msg:));

- (NSInteger) parameterCount NS_SWIFT_NAME(parameterCount());
- (NSString*) getParameterName: (NSInteger) paramIdx NS_SWIFT_NAME(getParameterName(paramIdx:));

@end
