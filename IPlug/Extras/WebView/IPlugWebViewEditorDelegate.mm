 /*
 ==============================================================================
 
  MIT License

  iPlug2 WebView Library
  Copyright (c) 2024 Oliver Larkin

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
 
 ==============================================================================
*/

#if __has_feature(objc_arc)
#error This file must be compiled without Arc. Don't use -fobjc-arc flag!
#endif

#include "IPlugWebViewEditorDelegate.h"

#ifdef OS_MAC
#import <AppKit/AppKit.h>
#elif defined(OS_IOS)
#import <UIKit/UIKit.h>
#endif

#if defined OS_MAC
  #define PLATFORM_VIEW NSView
#elif defined OS_IOS
  #define PLATFORM_VIEW UIView
#endif

using namespace iplug;

@interface IPLUG_WKWEBVIEW_EDITOR_HELPER : PLATFORM_VIEW
{
  WebViewEditorDelegate* mDelegate;
}
- (void) removeFromSuperview;
- (id) initWithEditorDelegate: (WebViewEditorDelegate*) pDelegate;
@end

@implementation IPLUG_WKWEBVIEW_EDITOR_HELPER
{
}

- (id) initWithEditorDelegate: (WebViewEditorDelegate*) pDelegate;
{
  mDelegate = pDelegate;
  CGFloat w = pDelegate->GetEditorWidth();
  CGFloat h = pDelegate->GetEditorHeight();
  CGRect r = CGRectMake(0, 0, w, h);
  self = [super initWithFrame:r];
  
  void* pWebView = pDelegate->OpenWebView(self, 0, 0, w, h, 1.0f);

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
    
  IPLUG_WKWEBVIEW_EDITOR_HELPER* pHelperView = [[IPLUG_WKWEBVIEW_EDITOR_HELPER alloc] initWithEditorDelegate: this];
  mHelperView = (void*) pHelperView;

  if (pParentView)
  {
    [pParentView addSubview: pHelperView];
  }
  
  if (mEditorInitFunc)
  {
    mEditorInitFunc();
  }
  
  return mHelperView;
}

void WebViewEditorDelegate::Resize(int width, int height)
{
  ResizeWebViewAndHelper(width, height);
  EditorResizeFromUI(width, height, true);
}

void WebViewEditorDelegate::OnParentWindowResize(int width, int height)
{
  ResizeWebViewAndHelper(width, height);
  EditorResizeFromUI(width, height, false);
}

void WebViewEditorDelegate::ResizeWebViewAndHelper(float width, float height)
{
  CGFloat w = static_cast<float>(width);
  CGFloat h = static_cast<float>(height);
  IPLUG_WKWEBVIEW_EDITOR_HELPER* pHelperView = (IPLUG_WKWEBVIEW_EDITOR_HELPER*) mHelperView;
  [pHelperView setFrame:CGRectMake(0, 0, w, h)];
  SetWebViewBounds(0, 0, w, h);
}
