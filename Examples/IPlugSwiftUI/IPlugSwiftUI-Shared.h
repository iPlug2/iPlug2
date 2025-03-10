#ifndef IPlugSwiftUI_Shared_h
#define IPlugSwiftUI_Shared_h

const int kNumPresets = 1;

const NSInteger kParamGain = 0;
const NSInteger kNumParams = 1;

const NSInteger kCtrlTagScope = 1;
const NSInteger kCtrlTagButton = 2;

const NSInteger kUpdateMessage = 0; // must match ISender::kUpdateMessage
const NSInteger kMsgTagHello = 1;
const NSInteger kMsgTagRestorePreset = 2;

const NSInteger kDataPacketSize = 1024;
extern const NSInteger kScopeBufferSize;

#endif /* IPlugSwiftUI_Shared_h */
