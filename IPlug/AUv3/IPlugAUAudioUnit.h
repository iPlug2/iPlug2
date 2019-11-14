#import <AudioToolbox/AudioToolbox.h>

#if defined __APPLE__
  #include <TargetConditionals.h>
  #if TARGET_OS_IPHONE
    #define OS_IOS
    #import <UIKit/UIKit.h>
    #define PLATFORM_VIEW UIView
  #else
    #define PLATFORM_VIEW NSView
  #endif
#endif

@interface IPlugAUAudioUnit : AUAudioUnit
- (void)informHostOfParamChange: (uint64_t) address : (float) realValue;
- (NSUInteger)getChannelLayoutTags: (int) dir : (AudioChannelLayoutTag*) pTags;
- (void)populateChannelCapabilitesArray: (NSMutableArray*) pArray;
- (NSInteger)width;
- (NSInteger)height;
- (PLATFORM_VIEW*)openWindow: (PLATFORM_VIEW*) pParent;
//- (void)resize: (CGRect) bounds;
- (void)closeWindow;
- (bool) sendMidiData:(int64_t) sampleTime : (NSInteger) length : (const uint8_t*) midiBytes;
@end
