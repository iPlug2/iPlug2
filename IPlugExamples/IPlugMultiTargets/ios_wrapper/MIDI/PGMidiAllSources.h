//
//  PGMidiAllSources.h
//  LXAB Test
//
//  Created by Pete Goodliffe on 31/01/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>

@class PGMidi;
@protocol PGMidiSourceDelegate;

@interface PGMidiAllSources : NSObject
{
  PGMidi                  *midi;
  id<PGMidiSourceDelegate> delegate;
}

@property (nonatomic,assign) PGMidi *midi;
@property (nonatomic,assign) id<PGMidiSourceDelegate> delegate;

@end
