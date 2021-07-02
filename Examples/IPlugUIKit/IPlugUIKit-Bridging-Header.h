//
//  Use this file to import your target's public headers that you would like to expose to Swift.
//

#import "IPlugCocoaViewController.h"
#import <MetalKit/MetalKit.h>
//#include "config.h"


const NSInteger kNumPresets = 3;

const NSInteger kParamGain = 0;
const NSInteger kNumParams = 1;

//tags in interfacebuilder default to 0, so avoid using that control tag
const NSInteger kCtrlTagVolumeSlider = 1;
const NSInteger kCtrlTagButton = 2;

const NSInteger kMsgTagHello = 0;
const NSInteger kMsgTagData = 1;
const NSInteger kMsgTagRestorePreset = 2;

const NSInteger kDataPacketSize = 1024; //floats

