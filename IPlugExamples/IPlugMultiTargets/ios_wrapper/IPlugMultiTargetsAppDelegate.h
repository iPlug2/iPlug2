#import <UIKit/UIKit.h>
#import <CoreMIDI/CoreMIDI.h>

#import "IPlugIOSAudio.h"
#import "PGMidi.h"

#include "IOSLink.h"

@class MainViewController;
@class PGMidi;

@interface IPlugMultiTargetsAppDelegate : NSObject <UIApplicationDelegate, PGMidiDelegate, PGMidiSourceDelegate>
{
  UIWindow *window;
  MainViewController *mainViewController;
  IPlugIOSAudio *mIOSAudio;
  IPlug *mPluginInstance;
  PGMidi *midi;
  IOSLink *mLink;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) IBOutlet MainViewController *mainViewController;

- (void) sendMidiMsg:(IMidiMsg*) msg;
- (void) allocateAudio:(Float64) sr;
- (void) changeSR:(Float64) sr;
- (Float64) getSR;

@end

