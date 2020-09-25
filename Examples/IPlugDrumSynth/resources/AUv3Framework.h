
#include <TargetConditionals.h>
#if TARGET_OS_IOS == 1
#import <UIKit/UIKit.h>
#else
#import <Cocoa/Cocoa.h>
#endif

//! Project version number for AUv3Framework.
FOUNDATION_EXPORT double AUv3FrameworkVersionNumber;

//! Project version string for AUv3Framework.
FOUNDATION_EXPORT const unsigned char AUv3FrameworkVersionString[];

// In this header, you should import all the public headers of your framework using statements like #import <AUv3Framework/PublicHeader.h>
@class IPlugAUViewController;
