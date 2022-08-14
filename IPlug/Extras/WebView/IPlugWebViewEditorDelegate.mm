 /*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#if __has_feature(objc_arc)
#error This file must be compiled without Arc. Don't use -fobjc-arc flag!
#endif

#include "IPlugWebViewEditorDelegate.h"

#ifdef OS_IOS
#import <UIKit/UIKit.h>
#endif

using namespace iplug;

@interface HELPER_VIEW : PLATFORM_VIEW
{
  WebViewEditorDelegate* mDelegate;
}
- (void) removeFromSuperview;
- (id) initWithEditorDelegate: (WebViewEditorDelegate*) pDelegate;
@end

@implementation HELPER_VIEW
{
}

- (id) initWithEditorDelegate: (WebViewEditorDelegate*) pDelegate;
{
  mDelegate = pDelegate;
  CGFloat w = pDelegate->GetEditorWidth();
  CGFloat h = pDelegate->GetEditorHeight();
  CGRect r = CGRectMake(0, 0, w, h);
  self = [super initWithFrame:r];
  
  void* pWebView = pDelegate->OpenWebView(self, 0, 0, w, h);

  [self addSubview: (PLATFORM_VIEW*) pWebView];

  return self;
}

- (void) removeFromSuperview
{
#ifdef AU_API
  //For AUv2 this is where we know about the window being closed, close via delegate
  mDelegate->CloseWindow();
#endif
  [super removeFromSuperview];
}

@end

WebViewEditorDelegate::WebViewEditorDelegate(int nParams)
: IEditorDelegate(nParams)
, IWebView()
{
}

WebViewEditorDelegate::~WebViewEditorDelegate()
{
  CloseWindow();
  
  PLATFORM_VIEW* pHelperView = (PLATFORM_VIEW*) mHelperView;
  [pHelperView release];
  mHelperView = nullptr;
}

void* WebViewEditorDelegate::OpenWindow(void* pParent)
{
  PLATFORM_VIEW* pParentView = (PLATFORM_VIEW*) pParent;
    
  HELPER_VIEW* pHelperView = [[HELPER_VIEW alloc] initWithEditorDelegate: this];
  mHelperView = (void*) pHelperView;

  if(pParentView) {
    [pParentView addSubview: pHelperView];
  }
  
  if(mEditorInitFunc)
    mEditorInitFunc();

  return mHelperView;
}

void WebViewEditorDelegate::Resize(int width, int height)
{
  CGFloat w = static_cast<float>(width);
  CGFloat h = static_cast<float>(height);
  HELPER_VIEW* pHelperView = (HELPER_VIEW*) mHelperView;
  [pHelperView setFrame:CGRectMake(0, 0, w, h)];
  SetWebViewBounds(0, 0, w, h);
  EditorResizeFromUI(width, height, false);
}
