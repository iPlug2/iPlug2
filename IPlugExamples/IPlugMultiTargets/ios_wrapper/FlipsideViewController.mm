#import "FlipsideViewController.h"
#import "IPlugMultiTargetsAppDelegate.h"

@implementation FlipsideViewController

@synthesize delegate;
@synthesize midi;
@synthesize samplerateSegControl;

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.view.backgroundColor = [UIColor viewFlipsideBackgroundColor];

  IPlugMultiTargetsAppDelegate* appDelegate = (IPlugMultiTargetsAppDelegate *)[[UIApplication sharedApplication] delegate];

  if([appDelegate getSR] == 44100.0)
  {
    samplerateSegControl.selectedSegmentIndex = 1;
  }
  else 
  {
    samplerateSegControl.selectedSegmentIndex = 0;
  }

//    CGRect cgRect =[[UIScreen mainScreen] bounds];
//    CGSize cgSize = cgRect.size;

//    NSMutableArray *options =[[NSMutableArray alloc]init];
//
//    [options addObject:@"None"];
//
//    for (PGMidiSource *source in midi.sources)
//    {
//      NSString *description = [NSString stringWithFormat:@"%@", source.name];
//      [options addObject:description];
//    }
//
//    midiInputList =[[MIRadioButtonGroup alloc]initWithFrame:CGRectMake(0, 200, cgSize.width, 75) andOptions:options andColumns:1];
//
//    [options removeAllObjects];
//
//    [options addObject:@"None"];
//
//    for (PGMidiSource *destination in midi.destinations)
//    {
//      NSString *description = [NSString stringWithFormat:@"%@", destination.name];
//      [options addObject:description];
//    }
//
//    midiOutputList =[[MIRadioButtonGroup alloc]initWithFrame:CGRectMake(0, 350, cgSize.width, 75) andOptions:options andColumns:1];
//
//    [options release];
//    [self.view addSubview:midiInputList];
//    [self.view addSubview:midiOutputList];
}

- (IBAction)done:(id)sender
{
  [self.delegate flipsideViewControllerDidFinish:self];
}

- (IBAction) doSRSegControl
{

  IPlugMultiTargetsAppDelegate* appDelegate = (IPlugMultiTargetsAppDelegate *)[[UIApplication sharedApplication] delegate];

  NSInteger selected = samplerateSegControl.selectedSegmentIndex;

  if (selected)
  {
    [appDelegate changeSR:44100.0];
  }
  else
  {
    [appDelegate changeSR:22050.0];
  }
}

- (void)didReceiveMemoryWarning
{
  // Releases the view if it doesn't have a superview.
  [super didReceiveMemoryWarning];

  // Release any cached data, images, etc that aren't in use.
}


- (void)viewDidUnload
{
  // Release any retained subviews of the main view.
  // e.g. self.myOutlet = nil;
}

/*
// Override to allow orientations other than the default portrait orientation.
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
	// Return YES for supported orientations
	return (interfaceOrientation == UIInterfaceOrientationPortrait);
}
*/

- (void)dealloc
{
  [super dealloc];
}


@end
