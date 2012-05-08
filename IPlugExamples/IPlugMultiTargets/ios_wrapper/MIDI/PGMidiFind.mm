//
//  PGMidiFind.mm
//  LXAB Test
//
//  Created by Pete Goodliffe on 31/01/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import "PGMidiFind.h"


@implementation PGMidi (FindingConnections)

- (PGMidiSource*) findSourceCalled:(NSString*)name
{
  for (PGMidiSource *source in self.sources)
  {
if ([source.name isEqualToString:name]) return source;
  }
  return nil;
}

- (PGMidiDestination*) findDestinationCalled:(NSString*)name
{
  for (PGMidiDestination *destination in self.destinations)
  {
if ([destination.name isEqualToString:name]) return destination;
  }
  return nil;
}

- (void) findMatchingSource:(PGMidiSource**)source
andDestination:(PGMidiDestination**)destination
avoidNames:(NSArray*)namesToAvoid
{
  *source      = nil;
  *destination = nil;

  for (PGMidiSource *s in self.sources)
  {
    if (s.isNetworkSession) continue;
if (namesToAvoid && [namesToAvoid containsObject:s.name]) continue;

PGMidiDestination *d = [self findDestinationCalled:s.name];
    if (d)
    {
      *source      = s;
      *destination = d;
      return;
    }
  }
}

- (void) findMatchingSource:(PGMidiSource**)source
andDestination:(PGMidiDestination**)destination
{
return [self findMatchingSource:source andDestination:destination avoidNames:nil];
}

@end
