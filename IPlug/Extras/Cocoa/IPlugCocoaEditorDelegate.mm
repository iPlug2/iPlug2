/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#include "IPlugCocoaEditorDelegate.h"
#import "IPlugCocoaViewController.h"

using namespace iplug;

CocoaEditorDelegate::CocoaEditorDelegate(int nParams)
: IEditorDelegate(nParams)
{
}

CocoaEditorDelegate::~CocoaEditorDelegate()
{
}

void* CocoaEditorDelegate::OpenWindow(void* pParent)
{
#ifdef OS_IOS
  IPlugCocoaViewController* vc = (IPlugCocoaViewController*) [(PLATFORM_VIEW*) pParent nextResponder];
  [vc setEditorDelegate: this];
  mViewController = vc;
#endif
  
  return pParent;
}

void CocoaEditorDelegate::CloseWindow()
{
  mViewController = nil;
}

bool CocoaEditorDelegate::OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData)
{
  IPlugCocoaViewController* vc = (IPlugCocoaViewController*) mViewController;
  NSData* pNSData = [NSData dataWithBytes:pData length:dataSize];
  return [vc onMessage:msgTag : ctrlTag : pNSData];
}

void CocoaEditorDelegate::OnParamChangeUI(int paramIdx, EParamSource source)
{
  IPlugCocoaViewController* vc = (IPlugCocoaViewController*) mViewController;
  
  if(vc)
    [vc onParamChangeUI:paramIdx :GetParam(paramIdx)->GetNormalized() ];
}

void CocoaEditorDelegate::OnMidiMsgUI(const IMidiMsg& msg)
{
  [(IPlugCocoaViewController*) mViewController onMidiMsgUI:msg.mStatus : msg.mData1 : msg.mData2 : msg.mOffset];
}

void CocoaEditorDelegate::OnSysexMsgUI(const ISysEx& msg)
{
  NSData* pNSData = [NSData dataWithBytes:msg.mData length:msg.mSize];

  [(IPlugCocoaViewController*) mViewController onSysexMsgUI:pNSData : msg.mOffset];
}

void CocoaEditorDelegate::SendControlValueFromDelegate(int ctrlTag, double normalizedValue)
{
  [(IPlugCocoaViewController*) mViewController sendControlValueFromDelegate:ctrlTag :normalizedValue];
}

void CocoaEditorDelegate::SendControlMsgFromDelegate(int ctrlTag, int msgTag, int dataSize, const void* pData)
{
  NSData* pNSData = [NSData dataWithBytes:pData length:dataSize];

  [(IPlugCocoaViewController*) mViewController sendControlMsgFromDelegate: ctrlTag : msgTag : pNSData];
}

void CocoaEditorDelegate::SendParameterValueFromDelegate(int paramIdx, double value, bool normalized)
{
  [(IPlugCocoaViewController*) mViewController sendParameterValueFromDelegate:paramIdx :value :normalized];
  
  IEditorDelegate::SendParameterValueFromDelegate(paramIdx, value, normalized);
}
