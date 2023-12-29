#include "MyPlatformControl.h"

#ifdef FRAMEWORK_BUILD
#import <AUv3Framework/IPlugEmbedPlatformView-Swift.h>
#else
#import <IPlugEmbedPlatformView-Swift.h>
#endif
#ifdef OS_IOS
#import <UIView+PinEdges.h>
#else
#import <NSView+PinEdges.h>
#endif

void MyPlatformControl::AttachSubViews(void* pPlatformView)
{
  PLATFORM_VIEW* pParent = (PLATFORM_VIEW*) pPlatformView;
  // TODO: this will leak
  SwiftViewController* vc = [[SwiftViewController alloc] init];
  vc.view.frame = [pParent bounds];
  [pParent addSubview:vc.view];
  [vc.view pinToSuperviewEdges];
}

