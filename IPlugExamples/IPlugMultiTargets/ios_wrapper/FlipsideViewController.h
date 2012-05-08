#import <UIKit/UIKit.h>
#import "MIRadioButtonGroup/MIRadioButtonGroup.h"
#import "MIDI/PGMidi.h"

@protocol FlipsideViewControllerDelegate;

@interface FlipsideViewController : UIViewController
{
  id <FlipsideViewControllerDelegate> delegate;
//  MIRadioButtonGroup *midiInputList;
//  MIRadioButtonGroup *midiOutputList;
  PGMidi *midi;
}

@property (nonatomic, retain) IBOutlet UISegmentedControl *samplerateSegControl;
@property (nonatomic,assign) PGMidi *midi;

- (IBAction)doSRSegControl;

@property (nonatomic, assign) id <FlipsideViewControllerDelegate> delegate;
- (IBAction)done:(id)sender;
@end

@protocol FlipsideViewControllerDelegate
- (void)flipsideViewControllerDidFinish:(FlipsideViewController *)controller;
@end

