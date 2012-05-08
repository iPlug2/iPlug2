//
//  PGMidiFind.h
//  LXAB Test
//
//  Created by Pete Goodliffe on 31/01/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import "PGMidi.h"

@interface PGMidi (FindingConnections)

- (PGMidiSource*)      findSourceCalled:(NSString*)name;

- (PGMidiDestination*) findDestinationCalled:(NSString*)name;

- (void)               findMatchingSource:(PGMidiSource**)source
andDestination:(PGMidiDestination**)destination;

- (void)               findMatchingSource:(PGMidiSource**)source
andDestination:(PGMidiDestination**)destination
avoidNames:(NSArray*)namesToAvoid;

@end
