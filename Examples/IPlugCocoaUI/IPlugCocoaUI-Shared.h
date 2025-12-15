#ifndef IPlugCocoaUI_Shared_h
#define IPlugCocoaUI_Shared_h

const NSInteger kNumPresets = 3;

static const NSInteger kParamGain = 0;
static const NSInteger kNumParams = 1;

//tags in interface builder default to 0, so avoid using that control tag
static const NSInteger kCtrlTagVolumeSlider = 1;
static const NSInteger kCtrlTagButton = 2;
static const NSInteger kCtrlTagVUMeter = 3;

static const NSInteger kUpdateMessage = 0;
static const NSInteger kMsgTagHello = 1;
static const NSInteger kMsgTagRestorePreset = 2;

static const NSInteger kDataPacketSize = 1024; //floats


#endif
