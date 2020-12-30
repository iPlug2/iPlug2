 /*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#if !__has_feature(objc_arc)
#error This file must be compiled with Arc. Use -fobjc-arc flag
#endif

#include "IPlugWebViewEditorDelegate.h"

#ifdef OS_IOS
#import <UIKit/UIKit.h>
#endif

using namespace iplug;

WebViewEditorDelegate::WebViewEditorDelegate(int nParams)
: IEditorDelegate(nParams)
, IWebView()
{
}

WebViewEditorDelegate::~WebViewEditorDelegate()
{
  CloseWindow();
}

void* WebViewEditorDelegate::OpenWindow(void* pParent)
{
  PLATFORM_VIEW* parentView = (__bridge PLATFORM_VIEW*) pParent;
  
  void* pView = OpenWebView(pParent, 0, 0, GetEditorWidth(), GetEditorHeight());
  
  if(mEditorInitFunc)
    mEditorInitFunc();
  
  [parentView addSubview: (__bridge PLATFORM_VIEW*) pView];
    
  return pView;
}
