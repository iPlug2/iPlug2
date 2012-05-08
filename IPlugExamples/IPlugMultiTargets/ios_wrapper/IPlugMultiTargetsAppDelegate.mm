#import "IPlugMultiTargetsAppDelegate.h"
#import "MainViewController.h"
#import "MIDI/PGMidi.h"


@implementation IPlugMultiTargetsAppDelegate


@synthesize window;
@synthesize mainViewController;

#pragma mark -
#pragma mark Application lifecycle

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
  mLink = new IOSLink(self);
  mPluginInstance = MakePlug(NULL, NULL, mLink);

  [self allocateAudio:44100.0];

  // Set the main view controller as the window's root view controller
  self.window.rootViewController = self.mainViewController;

  midi = [[PGMidi alloc] init];
  [midi enableNetwork:YES];
  midi.delegate = self;
  [midi init]; // TODO: why init again? bad? see comments http://goodliffe.blogspot.com/2011/02/pgmidi-updated.html

  mainViewController.midi = midi;
  mainViewController.mPluginInstance = mPluginInstance;

  //display
  [self.window makeKeyAndVisible];
  [mIOSAudio play];

  return YES;
}

- (void) allocateAudio:(Float64) sr
{

  if (mIOSAudio != nil)
    [mIOSAudio dealloc];

  int ticksPerBuffer = 64;

#if TARGET_IPHONE_SIMULATOR // otherwise glitchy audio on simulator 
  //int simulatorBufferSize = 512;
  ticksPerBuffer = 512;
  //ticksPerBuffer = simulatorBufferSize / IOS_BLOCK_SIZE;
#endif

  mIOSAudio = [[IPlugIOSAudio alloc] initWithIPlugInstance:mPluginInstance
  andSampleRate:sr
  andTicksPerBuffer:ticksPerBuffer
  andNumberOfInputChannels:IOS_INCHANS
  andNumberOfOutputChannels:IOS_OUTCHANS];
}

- (void)applicationWillResignActive:(UIApplication *)application
{
  /*
   Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
   Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
   */
}


- (void)applicationDidEnterBackground:(UIApplication *)application
{
  /*
   Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
   If your application supports background execution, called instead of applicationWillTerminate: when the user quits.
   */
}


- (void)applicationWillEnterForeground:(UIApplication *)application
{
  /*
   Called as part of  transition from the background to the inactive state: here you can undo many of the changes made on entering the background.
   */
}


- (void)applicationDidBecomeActive:(UIApplication *)application
{
  /*
   Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
   */
}


- (void)applicationWillTerminate:(UIApplication *)application
{
  /*
   Called when the application is about to terminate.
   See also applicationDidEnterBackground:.
   */
}


#pragma mark -
#pragma mark Memory management

- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application
{
  /*
   Free up as much memory as possible by purging cached data objects that can be recreated (or reloaded from disk) later.
   */
}


- (void)dealloc
{
  [mainViewController release];
  [window release];
  [mIOSAudio release];
  delete mLink;
  delete mPluginInstance;
  [super dealloc];
}

- (void)sendMidiMsg:(IMidiMsg*) msg
{
  const UInt8 rawMsg[]  = { msg->mStatus, msg->mData1, msg->mData2 };

  // TODO: maybe this should be deferred to low priority
  [midi sendBytes:rawMsg size:3];
}

- (void) changeSR:(Float64) sr
{
  if (sr != mIOSAudio.sampleRate)
  {
    [self allocateAudio:sr];
    [mIOSAudio play];
  }
}

- (Float64) getSR
{
  return mIOSAudio.sampleRate;
}

- (void) midi:(PGMidi*)midi sourceAdded:(PGMidiSource *)source
{
  source.delegate = self;
}

- (void) midi:(PGMidi*)midi sourceRemoved:(PGMidiSource *)source
{
  // TODO handle
}

- (void) midi:(PGMidi*)midi destinationAdded:(PGMidiDestination *)destination
{
  // TODO handle
}

- (void) midi:(PGMidi*)midi destinationRemoved:(PGMidiDestination *)destination
{
  // TODO handle
}

- (void) midiSource:(PGMidiSource*)midi midiReceived:(const MIDIPacketList *)packetList
{
  const MIDIPacket *packet = &packetList->packet[0];

  for (int i = 0; i < packetList->numPackets; ++i)
  {
    for (int j = 0; j < packet->length; j += 3 )
    {
      Byte status = packet->data[j + 0];
      Byte data1 = packet->data[j + 1];
      Byte data2 = packet->data[j + 2];

      IMidiMsg msg(0, status, data1, data2);
      mPluginInstance->ProcessMidiMsg(&msg);
    }

    packet = MIDIPacketNext(packet);
  }
}

@end
