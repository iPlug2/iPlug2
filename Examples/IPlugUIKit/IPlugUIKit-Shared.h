#ifndef IPlugUIKit_Shared_h
#define IPlugUIKit_Shared_h

const NSInteger kNumPresets = 3;

const NSInteger kParamGain = 0;
const NSInteger kNumParams = 1;

//tags in interface builder default to 0, so avoid using that control tag
const NSInteger kCtrlTagVolumeSlider = 1;
const NSInteger kCtrlTagButton = 2;
const NSInteger kCtrlTagVUMeter = 3;

const NSInteger kUpdateMessage = 0;
const NSInteger kMsgTagHello = 1;
const NSInteger kMsgTagRestorePreset = 2;

const NSInteger kDataPacketSize = 1024; //floats


#endif
