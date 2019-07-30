#include "IPlugCocoaEditorDelegate.h"
#import "IPlugCocoaViewController.h"

#if defined OS_MAC
#define VIEW NSView
#define MAKERECT NSMakeRect
#elif defined OS_IOS
#import <UIKit/UIKit.h>
#define VIEW UIView
#define MAKERECT CGRectMake
#endif

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
  IPlugCocoaViewController* viewController = (IPlugCocoaViewController*) [(VIEW*) pParent nextResponder];
  [viewController setEditorDelegate: this];
  mViewController = viewController;
#endif
  
  return pParent;
}

void CocoaEditorDelegate::CloseWindow()
{
}

bool CocoaEditorDelegate::OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData)
{
  IPlugCocoaViewController* viewController = (IPlugCocoaViewController*) mViewController;
  NSData* pNSData = [NSData dataWithBytes:pData length:dataSize];
  return [viewController onMessage:msgTag : ctrlTag : pNSData];
}

void CocoaEditorDelegate::OnParamChangeUI(int paramIdx, EParamSource source)
{
  IPlugCocoaViewController* viewController = (IPlugCocoaViewController*) mViewController;
  
  if(viewController)
    [viewController onParamChangeUI:paramIdx :GetParam(paramIdx)->GetNormalized() ];
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
  [(IPlugCocoaViewController*) mViewController sendControlMsgFromDelegate: ctrlTag : msgTag : dataSize : pData];
}

void CocoaEditorDelegate::SendParameterValueFromDelegate(int paramIdx, double value, bool normalized)
{
  [(IPlugCocoaViewController*) mViewController sendParameterValueFromDelegate:paramIdx :value :normalized];
  
  IEditorDelegate::SendParameterValueFromDelegate(paramIdx, value, normalized);
}
