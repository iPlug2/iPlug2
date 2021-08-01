/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#import "IPlugCocoaViewController.h"
#include "IPlugCocoaEditorDelegate.h"
#include "IPlugStructs.h"

using namespace iplug;

@implementation IPlugCocoaViewController

- (void) setEditorDelegate: (void*) _editorDelegate
{
  editorDelegate = _editorDelegate;
}

- (BOOL) onMessage: (NSInteger) msgTag : (NSInteger) ctrlTag : (NSData*) msg
{
  return FALSE;
}

- (void) onParamChangeUI: (NSInteger) paramIdx : (double) value
{
  //NO-OP
}

- (void) onMidiMsgUI: (UInt8) status : (UInt8) data1 : (UInt8) data2 : (NSInteger) offset
{
  //NO-OP
}

- (void) onSysexMsgUI: (NSData*) msg : (NSInteger) offset
{
  //NO-OP
}

- (void) sendControlValueFromDelegate: (NSInteger) ctrlTag : (double) normalizedValue
{
  //NO-OP
}

- (void) sendControlMsgFromDelegate: (NSInteger) ctrlTag : (NSInteger) msgTag : (NSData*) msg
{
  //NO-OP
}

- (void) sendParameterValueFromDelegate: (NSInteger) paramIdx : (double) value : (BOOL) normalized
{
  [self onParamChangeUI: paramIdx : value];
}

-(void) sendParameterValueFromUI: (NSInteger) paramIdx : (double) value
{
  ((CocoaEditorDelegate*) editorDelegate)->SendParameterValueFromUI(static_cast<int>(paramIdx), value);
}

-(void) beginInformHostOfParamChangeFromUI: (NSInteger) paramIdx
{
  ((CocoaEditorDelegate*) editorDelegate)->BeginInformHostOfParamChangeFromUI(static_cast<int>(paramIdx));
}

-(void) endInformHostOfParamChangeFromUI: (NSInteger) paramIdx
{
  ((CocoaEditorDelegate*) editorDelegate)->EndInformHostOfParamChangeFromUI(static_cast<int>(paramIdx));
}

-(void) sendArbitraryMsgFromUI: (NSInteger) msgTag : (NSInteger) ctrlTag : (NSData*) msg
{
  ((CocoaEditorDelegate*) editorDelegate)->SendArbitraryMsgFromUI(static_cast<int>(msgTag), static_cast<int>(ctrlTag), static_cast<int>([msg length]), [msg bytes]);
}

-(void)sendMidiMsgFromUI: (UInt8) status : (UInt8) data1 : (UInt8) data2 : (NSInteger) offset
{
  IMidiMsg msg { static_cast<int>(offset), status, data1, data2 };
  ((CocoaEditorDelegate*) editorDelegate)->SendMidiMsgFromUI(msg);
}

-(void)sendSysexMsgFromUI: (NSData*) msg : (NSInteger) offset
{
  ISysEx smsg { static_cast<int>(offset), reinterpret_cast<const uint8_t*>([msg bytes]), static_cast<int>([msg length]) };
  ((CocoaEditorDelegate*) editorDelegate)->SendSysexMsgFromUI(smsg);
}

- (NSInteger)parameterCount
{
  return ((CocoaEditorDelegate*) editorDelegate)->NParams();
}

- (NSString*) getParameterName: (NSInteger) paramIdx
{
  NSString* name = [NSString stringWithUTF8String:((CocoaEditorDelegate*) editorDelegate)->GetParam(static_cast<int>(paramIdx))->GetName()];
  return name;
}

- (double) getParameterDefault: (NSInteger) paramIdx
{
  return ((CocoaEditorDelegate*) editorDelegate)->GetParam(static_cast<int>(paramIdx))->GetDefault();
}

- (double) getParameterMin: (NSInteger) paramIdx
{
  return ((CocoaEditorDelegate*) editorDelegate)->GetParam(static_cast<int>(paramIdx))->GetMin();
}

- (double) getParameterMax: (NSInteger) paramIdx
{
  return ((CocoaEditorDelegate*) editorDelegate)->GetParam(static_cast<int>(paramIdx))->GetMax();
}

- (double) getParameterStep: (NSInteger) paramIdx
{
  return ((CocoaEditorDelegate*) editorDelegate)->GetParam(static_cast<int>(paramIdx))->GetStep();
}

- (NSString*) getParameterLabel: (NSInteger) paramIdx
{
  NSString* label = [NSString stringWithUTF8String:((CocoaEditorDelegate*) editorDelegate)->GetParam(static_cast<int>(paramIdx))->GetLabel()];
  return label;
}

- (NSString*) getParameterGroup: (NSInteger) paramIdx
{
  NSString* group = [NSString stringWithUTF8String:((CocoaEditorDelegate*) editorDelegate)->GetParam(static_cast<int>(paramIdx))->GetGroup()];
  return group;
}

@end
