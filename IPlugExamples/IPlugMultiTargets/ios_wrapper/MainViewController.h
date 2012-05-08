#import "FlipsideViewController.h"
#import "IPlugMultiTargetsAppDelegate.h"

@class PGMidi;

@interface MainViewController : UIViewController <FlipsideViewControllerDelegate>
{
  NSMutableArray *sliders;
  NSMutableArray *nameLabels;
  NSMutableArray *valueLabels;

  UIScrollView *scrollView;

  PGMidi *midi;
  IPlug *mPluginInstance;
}

@property (nonatomic,assign) PGMidi *midi;
@property (nonatomic,assign) IPlug *mPluginInstance;

- (IBAction)showInfo:(id)sender;
- (IBAction)sliderValueChanged:(UISlider *)sender;
- (IBAction)noteOnPressed:(UIButton *)sender;
- (IBAction)noteOffPressed:(UIButton *)sender;

- (void)addSlider:(int)idx;

@end
