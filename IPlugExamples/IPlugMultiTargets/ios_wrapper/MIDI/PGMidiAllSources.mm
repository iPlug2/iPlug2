//
//  PGMidiAllSources.mm
//  LXAB Test
//
//  Created by Pete Goodliffe on 31/01/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import "PGMidiAllSources.h"

#import "PGMidi.h"

@interface PGMidiAllSources () <PGMidiDelegate, PGMidiSourceDelegate>
@end

@implementation PGMidiAllSources

- (void) dealloc
{
  self.midi = nil;
  [super dealloc];
}

@synthesize midi, delegate;

- (void) setMidi:(PGMidi *)newMidi
{
  midi.delegate = nil;
  for (PGMidiSource *source in midi.sources) source.delegate = nil;

  midi = newMidi;

  midi.delegate = self;
  for (PGMidiSource *source in midi.sources) source.delegate = self;
}

#pragma mark PGMidiDelegate

- (void) midi:(PGMidi*)midi sourceAdded:(PGMidiSource *)source
{
  source.delegate = self;
}

- (void) midi:(PGMidi*)midi sourceRemoved:(PGMidiSource *)source {}
- (void) midi:(PGMidi*)midi destinationAdded:(PGMidiDestination *)destination {}
- (void) midi:(PGMidi*)midi destinationRemoved:(PGMidiDestination *)destination {}

#pragma mark PGMidiSourceDelegate

- (void) midiSource:(PGMidiSource*)input midiReceived:(const MIDIPacketList *)packetList
{
[delegate midiSource:input midiReceived:packetList];
}

@end
