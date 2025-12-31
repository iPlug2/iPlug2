/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file IPlugObjCSwizzle_IGraphics.h
 * @brief Integration of ObjC swizzling with IGraphics classes
 *
 * Include this header in your IGraphics platform implementation to enable
 * runtime unique class creation for all IGraphics Objective-C classes.
 *
 * This solves the flat namespace collision issue when multiple plugins
 * with IGraphics are loaded into the same host process.
 *
 * Usage:
 *   1. In your plugin's early initialization, call:
 *      IPlugObjCSwizzleInitWithConfig();
 *
 *   2. Before creating any IGraphics views, call:
 *      IPlugObjCRegisterIGraphicsClasses();
 *
 *   3. Use IPLUG_IGRAPHICS_VIEW_CLASS instead of IGRAPHICS_VIEW
 *      (or call IPlugObjCGetIGraphicsViewClass())
 */

#if defined(__APPLE__) && defined(__OBJC__)

#import "IPlugObjCSwizzle.h"

// Forward declarations of IGraphics classes (the originals)
@class IGRAPHICS_VIEW;
@class IGRAPHICS_MENU;
@class IGRAPHICS_MENU_RCVR;
@class IGRAPHICS_FORMATTER;
@class IGRAPHICS_TEXTFIELD;
@class IGRAPHICS_TEXTFIELDCELL;

#if TARGET_OS_IOS
@class IGRAPHICS_UITABLEVC;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the swizzle system using the plugin's BUNDLE_ID from config.h
 * This macro version automatically uses the BUNDLE_ID defined in your plugin's config.
 *
 * Call this early in plugin initialization (e.g., in constructor or factory function).
 */
#if defined(BUNDLE_ID) && BUNDLE_ID[0] != '\0'
#define IPlugObjCSwizzleInitWithConfig() IPlugObjCSwizzleInit(BUNDLE_ID)
#elif defined(PLUG_UNIQUE_ID)
// Fallback to PLUG_UNIQUE_ID as a four-char code string
#define IPlugObjCSwizzleInitWithConfig() IPlugObjCSwizzleInit(#PLUG_UNIQUE_ID)
#else
#define IPlugObjCSwizzleInitWithConfig() ((void)0)
#endif

/**
 * Register all standard IGraphics Objective-C classes for unique naming.
 * Call this after IPlugObjCSwizzleInit() and before creating any views.
 */
static inline void IPlugObjCRegisterIGraphicsClasses(void)
{
  // Register the main view class
  IPlugObjCRegisterClass([IGRAPHICS_VIEW class]);

#if TARGET_OS_OSX
  // Register macOS-specific classes
  IPlugObjCRegisterClass([IGRAPHICS_MENU class]);
  IPlugObjCRegisterClass([IGRAPHICS_MENU_RCVR class]);
  IPlugObjCRegisterClass([IGRAPHICS_FORMATTER class]);
  IPlugObjCRegisterClass([IGRAPHICS_TEXTFIELD class]);
  IPlugObjCRegisterClass([IGRAPHICS_TEXTFIELDCELL class]);
#endif

#if TARGET_OS_IOS
  // Register iOS-specific classes
  IPlugObjCRegisterClass([IGRAPHICS_UITABLEVC class]);
#endif
}

/**
 * Get the unique view class for this plugin.
 * Returns the swizzled class if initialized, otherwise the original IGRAPHICS_VIEW.
 */
static inline Class IPlugObjCGetIGraphicsViewClass(void)
{
  Class cls = IPlugObjCGetClass("IGRAPHICS_VIEW");
  return cls ? cls : [IGRAPHICS_VIEW class];
}

/**
 * Get the unique menu class for this plugin (macOS only).
 */
#if TARGET_OS_OSX
static inline Class IPlugObjCGetIGraphicsMenuClass(void)
{
  Class cls = IPlugObjCGetClass("IGRAPHICS_MENU");
  return cls ? cls : [IGRAPHICS_MENU class];
}
#endif

#ifdef __cplusplus
}
#endif

// Convenience macro: use this instead of IGRAPHICS_VIEW in allocations
#define IPLUG_IGRAPHICS_VIEW_CLASS IPlugObjCGetIGraphicsViewClass()

// Example usage in IGraphicsMac.mm:
//
// Before (causes flat namespace collision):
//   IGRAPHICS_VIEW* pView = [[IGRAPHICS_VIEW alloc] initWithIGraphics: this];
//
// After (each plugin gets unique class):
//   IGRAPHICS_VIEW* pView = [[IPLUG_IGRAPHICS_VIEW_CLASS alloc] initWithIGraphics: this];
//
// Or equivalently:
//   IGRAPHICS_VIEW* pView = [IPLUG_OBJC_ALLOC(IGRAPHICS_VIEW) initWithIGraphics: this];

#endif // __APPLE__ && __OBJC__
