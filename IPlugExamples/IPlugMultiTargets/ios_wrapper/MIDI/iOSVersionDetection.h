/*
 *  iOSVersionDetection.h
 *  MidiMonitor
 *
 *  Created by Pete Goodliffe on 9/22/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

// Adapted from http://cocoawithlove.com/2010/07/tips-tricks-for-conditional-ios3-ios32.html

#ifndef kCFCoreFoundationVersionNumber_iPhoneOS_4_0
#define kCFCoreFoundationVersionNumber_iPhoneOS_4_0 550.32
#endif

#ifndef kCFCoreFoundationVersionNumber_iPhoneOS_4_1
#define kCFCoreFoundationVersionNumber_iPhoneOS_4_1 550.38
#endif

#ifndef kCFCoreFoundationVersionNumber_iPhoneOS_4_2
// NOTE: THIS IS NOT THE FINAL NUMBER
// 4.2 is not out of beta yet, so took the beta 1 build number
#define kCFCoreFoundationVersionNumber_iPhoneOS_4_2 550.47
#endif

#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 40000
#define IF_IOS4_OR_GREATER(...) \
    if (kCFCoreFoundationVersionNumber >= kCFCoreFoundationVersionNumber_iPhoneOS_4_0) \
    { \
        __VA_ARGS__ \
    }
#else
#define IF_IOS4_OR_GREATER(...) \
    if (false) \
    { \
    }
#endif

#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 40200
#define IF_IOS4_2_OR_GREATER(...) \
    if (kCFCoreFoundationVersionNumber >= kCFCoreFoundationVersionNumber_iPhoneOS_4_2) \
    { \
        __VA_ARGS__ \
    }
#else
#define IF_IOS4_2_OR_GREATER(...) \
    if (false) \
    { \
    }
#endif

#define IF_IOS_HAS_COREMIDI(...) IF_IOS4_2_OR_GREATER(__VA_ARGS__)
