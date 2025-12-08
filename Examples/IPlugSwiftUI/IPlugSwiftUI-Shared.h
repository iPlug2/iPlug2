#ifndef IPlugSwiftUI_Shared_h
#define IPlugSwiftUI_Shared_h

static const int kNumPresets = 1;

static const NSInteger kParamGain = 0;
static const NSInteger kNumParams = 1;

static const NSInteger kCtrlTagScope = 1;
static const NSInteger kCtrlTagButton = 2;

static const NSInteger kUpdateMessage = 0; // must match ISender::kUpdateMessage
static const NSInteger kMsgTagHello = 1;
static const NSInteger kMsgTagRestorePreset = 2;

static const NSInteger kDataPacketSize = 1024;
extern const NSInteger kScopeBufferSize;

#endif /* IPlugSwiftUI_Shared_h */
