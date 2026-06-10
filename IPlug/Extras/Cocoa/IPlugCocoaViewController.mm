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

- (id) initWithEditorDelegateAndBundleID: (void*) _editorDelegate : (const char*) _bundleID
{
  if (self = [super init]) {
    editorDelegate = _editorDelegate;
    bundleID = [NSString stringWithUTF8String:_bundleID];
  }
  return self;
}

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

-(void) sendMidiMsgFromUI: (UInt8) status : (UInt8) data1 : (UInt8) data2 : (NSInteger) offset
{
  IMidiMsg msg { static_cast<int>(offset), status, data1, data2 };
  ((CocoaEditorDelegate*) editorDelegate)->SendMidiMsgFromUI(msg);
}

-(void) sendSysexMsgFromUI: (NSData*) msg : (NSInteger) offset
{
  ISysEx smsg { static_cast<int>(offset), reinterpret_cast<const uint8_t*>([msg bytes]), static_cast<int>([msg length]) };
  ((CocoaEditorDelegate*) editorDelegate)->SendSysexMsgFromUI(smsg);
}

- (NSInteger) parameterCount
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

- (NSInteger) getParameterNumDisplayTexts:(NSInteger)paramIdx
{
  if (paramIdx < 0 || paramIdx >= [self parameterCount]) {
    return 0;
  }
  
  IParam* pParam = ((CocoaEditorDelegate*) editorDelegate)->GetParam(static_cast<int>(paramIdx));
  return pParam->NDisplayTexts();
}

- (NSString*) getParameterDisplayText:(NSInteger)paramIdx
                                index:(NSInteger)displayIndex
{
  if (paramIdx < 0 || paramIdx >= [self parameterCount]) {
    return @"";
  }
  
  IParam* pParam = ((CocoaEditorDelegate*) editorDelegate)->GetParam(static_cast<int>(paramIdx));
  
  // We don’t care about the numeric value here,
  // but we have to call GetDisplayTextAtIdx:
  double dummyValue = 0.0;
  const char* text = pParam->GetDisplayTextAtIdx(static_cast<int>(displayIndex), &dummyValue);
  
  if (!text) {
    return @"";
  }
  return [NSString stringWithUTF8String:text];
}

- (double) getParameterDisplayTextValue:(NSInteger)paramIdx
                                  index:(NSInteger)displayIndex
{
  if (paramIdx < 0 || paramIdx >= [self parameterCount]) {
    return 0.0;
  }
  
  IParam* pParam = ((CocoaEditorDelegate*) editorDelegate)->GetParam(static_cast<int>(paramIdx));
  
  double outVal = 0.0;
  pParam->GetDisplayTextAtIdx(static_cast<int>(displayIndex), &outVal);
  return outVal;
}

- (int) getParameterFlags:(NSInteger) paramIdx
{
  if (paramIdx < 0 || paramIdx >= [self parameterCount]) {
    return 0;
  }
  
  IParam* pParam = ((CocoaEditorDelegate*) editorDelegate)->GetParam(static_cast<int>(paramIdx));
  return pParam->GetFlags();
}

- (int) getParameterShapeID:(NSInteger)paramIdx
{
  if (paramIdx < 0 || paramIdx >= [self parameterCount]) {
    return -1;
  }
  IParam* pParam = ((CocoaEditorDelegate*) editorDelegate)->GetParam(static_cast<int>(paramIdx));
  
  return pParam->GetShapeID();
}

- (double) getParameterShapeValue:(NSInteger)paramIdx
{
  if (paramIdx < 0 || paramIdx >= [self parameterCount]) {
    return 0.0;
  }
  IParam* pParam = ((CocoaEditorDelegate*) editorDelegate)->GetParam(static_cast<int>(paramIdx));

  return pParam->GetShapeValue();
}


- (void) setBundleID: (const char *) _bundleID
{
  bundleID = [NSString stringWithUTF8String:_bundleID];
}

- (NSString*) getBundleID
{
  return bundleID;
}

@end
