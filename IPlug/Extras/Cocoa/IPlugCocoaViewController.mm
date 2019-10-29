#import "IPlugCocoaViewController.h"
#include "IPlugCocoaEditorDelegate.h"
#include "IPlugStructs.h"

using namespace iplug;

@implementation IPlugCocoaViewController

- (void)setEditorDelegate: (void*) _editorDelegate
{
  editorDelegate = _editorDelegate;
}

- (BOOL) onMessage: (int) msgTag : (int) ctrlTag : (NSData*) msg
{
  return FALSE;
}

- (void) onParamChangeUI: (int) paramIdx : (double) value
{
  //NO-OP
}

- (void) onMidiMsgUI: (UInt8) status : (UInt8) data1 : (UInt8) data2 : (int) offset
{
  //NO-OP
}

- (void) onSysexMsgUI: (NSData*) msg : (int) offset
{
  //NO-OP
}

- (void) sendControlValueFromDelegate: (int) controlTag : (double) normalizedValue
{
  //NO-OP
}

- (void) sendControlMsgFromDelegate: (int) controlTag : (int) msgTag : (int) dataSize : (const void*) pData
{
  //NO-OP
}

- (void) sendParameterValueFromDelegate: (int) paramIdx : (double) value : (BOOL) normalized
{
  [self onParamChangeUI: paramIdx : value];
}

-(void) sendParameterValueFromUI: (int) paramIdx : (double) value
{
  ((CocoaEditorDelegate*) editorDelegate)->SendParameterValueFromUI(paramIdx, value);
}

-(void) beginInformHostOfParamChangeFromUI: (int) paramIdx
{
  ((CocoaEditorDelegate*) editorDelegate)->BeginInformHostOfParamChangeFromUI(paramIdx);
}

-(void) endInformHostOfParamChangeFromUI: (int) paramIdx
{
  ((CocoaEditorDelegate*) editorDelegate)->EndInformHostOfParamChangeFromUI(paramIdx);
}

-(void) sendArbitraryMsgFromUI: (int) msgTag : (int) ctrlTag : (NSData*) msg
{
  ((CocoaEditorDelegate*) editorDelegate)->SendArbitraryMsgFromUI(msgTag, ctrlTag, (int) [msg length], [msg bytes]);
}

-(void)sendMidiMsgFromUI: (UInt8) status : (UInt8) data1 : (UInt8) data2 : (int) offset
{
  IMidiMsg msg { offset, status, data1, data2 };
  ((CocoaEditorDelegate*) editorDelegate)->SendMidiMsgFromUI(msg);
}

-(void)sendSysexMsgFromUI: (NSData*) msg : (int) offset
{
  ISysEx smsg { offset, reinterpret_cast<const uint8_t*>([msg bytes]), (int) [msg length] };
  ((CocoaEditorDelegate*) editorDelegate)->SendSysexMsgFromUI(smsg);
}

@end
