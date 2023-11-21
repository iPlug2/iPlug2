 /*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#if !__has_feature(objc_arc)
#error This file must be compiled with Arc. Use -fobjc-arc flag
#endif

#include "IPlugPlatformView.h"
#include "IPlugPaths.h"

using namespace iplug;

IPlatformView::IPlatformView(bool opaque)
: mOpaque(opaque)
{
}

IPlatformView::~IPlatformView()
{
  RemovePlatformView();
}

void* IPlatformView::CreatePlatformView(void* pParent, float x, float y, float w, float h, float scale)
{
  @autoreleasepool {
    PLATFORM_VIEW* platformView = [[PLATFORM_VIEW alloc] initWithFrame:MAKERECT(x,y,w,h)];
    mPlatformView = (__bridge_retained void*) platformView;
  }

  return mPlatformView;
}

void IPlatformView::RemovePlatformView()
{
  @autoreleasepool {
    PLATFORM_VIEW* platformView = (__bridge_transfer PLATFORM_VIEW*) mPlatformView;
    [platformView removeFromSuperview];
  }
  
  mPlatformView = nullptr;
}

void IPlatformView::SetChildViewBounds(float x, float y, float w, float h, float scale)
{
  [(__bridge PLATFORM_VIEW*) mPlatformView setFrame: MAKERECT(x, y, w, h) ];
}
