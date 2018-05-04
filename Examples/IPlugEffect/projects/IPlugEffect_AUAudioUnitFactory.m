#import "IPlugEffect_AUAudioUnitFactory.h"

@implementation IPlugViewController (AUAudioUnitFactory)

- (IPlugAUAudioUnit*) createAudioUnitWithComponentDescription:(AudioComponentDescription) desc error:(NSError **)error
{
  self.audioUnit = [[[IPlugAUAudioUnit alloc] initWithComponentDescription:desc error:error] retain];
  return self.audioUnit;
}

@end
