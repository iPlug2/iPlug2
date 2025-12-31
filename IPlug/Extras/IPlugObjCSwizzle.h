/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file IPlugObjCSwizzle.h
 * @brief Runtime Objective-C class creation utilities for iPlug2
 *
 * This module provides utilities to dynamically create unique Objective-C
 * class names at runtime, solving the flat namespace issue that occurs when
 * multiple plugins share the same class names in a host process.
 *
 * Instead of using preprocessor macros to prefix class names at compile time,
 * this approach creates unique subclasses at runtime based on a plugin
 * identifier (typically BUNDLE_ID or PLUG_UNIQUE_ID).
 *
 * Usage:
 *   1. Call IPlugObjCRegisterClasses() early in plugin initialization
 *   2. Use IPlugObjCGetClass("BaseClassName") to get the unique class
 *   3. Or use the convenience macros IPLUG_OBJC_CLASS() and IPLUG_OBJC_ALLOC()
 */

#if defined(__APPLE__) && (defined(__OBJC__) || defined(__cplusplus))

#import <objc/runtime.h>
#import <Foundation/Foundation.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the swizzle system with a unique plugin identifier.
 * This should be called once during plugin initialization.
 *
 * @param pluginId A unique identifier for this plugin (e.g., BUNDLE_ID or PLUG_UNIQUE_ID)
 * @return YES if initialization succeeded, NO if already initialized or failed
 */
BOOL IPlugObjCSwizzleInit(const char* pluginId);

/**
 * Register a base class to have a unique runtime subclass created.
 * The unique class will inherit from baseClass and have a name like:
 * "BaseClassName_com_mymfr_myplugin" (sanitized from pluginId)
 *
 * @param baseClass The original class to subclass
 * @return The unique runtime class, or baseClass if registration fails
 */
Class IPlugObjCRegisterClass(Class baseClass);

/**
 * Get the unique runtime class for a given base class name.
 * Must be called after IPlugObjCSwizzleInit() and IPlugObjCRegisterClass().
 *
 * @param baseClassName The name of the base class (e.g., "IGRAPHICS_VIEW")
 * @return The unique runtime class, or nil if not found
 */
Class IPlugObjCGetClass(const char* baseClassName);

/**
 * Get the unique class name suffix being used (derived from plugin ID).
 *
 * @return The sanitized suffix string, or NULL if not initialized
 */
const char* IPlugObjCGetSuffix(void);

/**
 * Check if the swizzle system has been initialized.
 *
 * @return YES if initialized, NO otherwise
 */
BOOL IPlugObjCSwizzleIsInitialized(void);

/**
 * Clean up all registered classes (typically not needed, classes persist for process lifetime).
 * Warning: Only call this if you're certain no instances of the classes exist.
 */
void IPlugObjCSwizzleCleanup(void);

#ifdef __cplusplus
}
#endif

// Convenience macros for common operations
#ifdef __OBJC__

/**
 * Get the unique runtime class for a base class.
 * Example: IPLUG_OBJC_CLASS(IGRAPHICS_VIEW) returns the unique subclass
 */
#define IPLUG_OBJC_CLASS(BaseClass) IPlugObjCGetClass(#BaseClass)

/**
 * Allocate an instance of the unique runtime class.
 * Example: IPLUG_OBJC_ALLOC(IGRAPHICS_VIEW) returns [[UniqueClass alloc]]
 */
#define IPLUG_OBJC_ALLOC(BaseClass) [IPlugObjCGetClass(#BaseClass) alloc]

/**
 * Register a class for unique subclassing and get the result.
 * Example: IPLUG_OBJC_REGISTER(IGRAPHICS_VIEW) registers and returns unique class
 */
#define IPLUG_OBJC_REGISTER(BaseClass) IPlugObjCRegisterClass([BaseClass class])

#endif // __OBJC__

#endif // __APPLE__ && (__OBJC__ || __cplusplus)
