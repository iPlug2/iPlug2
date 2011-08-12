#import "MainViewController.h"
@implementation MainViewController

@synthesize midi;
@synthesize mPluginInstance;

- (IBAction)sliderValueChanged:(UISlider *)sender {

  int idx = 0;
  
  for (UISlider* slider in sliders)
  {
    if (slider == sender)
      break;
    
    idx++;
  }
  
  mPluginInstance->SetParameterFromGUI(idx, sender.value);
  
  char buf[256];
  mPluginInstance->GetParam(idx)->GetDisplayForHost(buf);
  strcat (buf, mPluginInstance->GetParam(idx)->GetLabelForHost());
  UILabel* label = [valueLabels objectAtIndex:idx];
  label.text = [NSString stringWithUTF8String:buf];
}

- (IBAction)noteOnPressed:(UIButton *)sender {
  
  IMidiMsg msg;
  msg.MakeNoteOnMsg(60, 127, 0);
  mPluginInstance->ProcessMidiMsg(&msg);
  
  IPlugMultiTargetsAppDelegate* appDelegate = (IPlugMultiTargetsAppDelegate *)[[UIApplication sharedApplication] delegate];
  [appDelegate sendMidiMsg:(&msg)];
}

- (IBAction)noteOffPressed:(UIButton *)sender {
  
  IMidiMsg msg;
  msg.MakeNoteOffMsg(60, 0);
  mPluginInstance->ProcessMidiMsg(&msg);
  
  IPlugMultiTargetsAppDelegate* appDelegate = (IPlugMultiTargetsAppDelegate *)[[UIApplication sharedApplication] delegate];
  [appDelegate sendMidiMsg:(&msg)];
}

- (void)viewDidLoad {
  [super viewDidLoad];

  sliders = [[NSMutableArray alloc] init];
  valueLabels = [[NSMutableArray alloc] init];
  nameLabels = [[NSMutableArray alloc] init];

}

- (void)flipsideViewControllerDidFinish:(FlipsideViewController *)controller {
    
	[self dismissModalViewControllerAnimated:YES];
}


- (IBAction)showInfo:(id)sender {    
	
	FlipsideViewController *controller = [[FlipsideViewController alloc] initWithNibName:@"FlipsideView" bundle:nil];
	controller.delegate = self;
	controller.midi = self.midi;
  
	controller.modalTransitionStyle = UIModalTransitionStyleFlipHorizontal;
	[self presentModalViewController:controller animated:YES];
	
	[controller release];
}

- (void)didReceiveMemoryWarning {
	// Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
	
	// Release any cached data, images, etc. that aren't in use.
}

/*
// Override to allow orientations other than the default portrait orientation.
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
	// Return YES for supported orientations.
	return (interfaceOrientation == UIInterfaceOrientationPortrait);
}
*/


- (void)viewDidUnload {

}

- (void)dealloc {
  
  //TODO not sure if this is nessecary
  
  for (UISlider* slider in sliders)
  {
    [slider release];
  }
  
  [sliders release];
  
  for (UILabel* label in valueLabels)
  {
    [label release];
  }
  
  for (UILabel* label in nameLabels)
  {
    [label release];
  }
  
  [valueLabels release];
  [nameLabels release];
  [super dealloc];
}

- (void)addSlider:(int)idx {
  
  CGRect cgRect =[[UIScreen mainScreen] bounds];
  CGSize cgSize = cgRect.size;
  
  UISlider *slider = [[UISlider alloc] initWithFrame:CGRectMake(10, (60 * idx) + 30, cgSize.width - 20, 30)];
  [slider addTarget:self action:@selector(sliderValueChanged:) forControlEvents:UIControlEventValueChanged];
  [slider setValue:mPluginInstance->GetParam(idx)->GetNormalized()];
  [self.view addSubview:slider];

  [sliders addObject: slider];
  
  UILabel *nameLabel = [[UILabel alloc] initWithFrame:CGRectMake(13, (60 * idx) + 12, 150, 20)];
  nameLabel.textColor = [UIColor whiteColor];
  nameLabel.backgroundColor = [UIColor clearColor];
  nameLabel.text = [NSString stringWithUTF8String:mPluginInstance->GetParam(idx)->GetNameForHost()];
  [self.view addSubview:nameLabel];
  
  [nameLabels addObject: nameLabel];
  
  UILabel *valueLabel = [[UILabel alloc] initWithFrame:CGRectMake(cgSize.width-180, (60 * idx) + 12, 150, 20)];
  valueLabel.textAlignment=UITextAlignmentRight;  
  char buf[256];
  mPluginInstance->GetParam(idx)->GetDisplayForHost(buf);
  strcat (buf, mPluginInstance->GetParam(idx)->GetLabelForHost());
  valueLabel.text = [NSString stringWithUTF8String:buf];
  valueLabel.textColor = [UIColor whiteColor];
  valueLabel.backgroundColor = [UIColor clearColor];
  [self.view addSubview:valueLabel];
  
  [valueLabels addObject: valueLabel];
}


@end
