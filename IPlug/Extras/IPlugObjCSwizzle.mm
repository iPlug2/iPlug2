/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#import "IPlugObjCSwizzle.h"

#if defined(__APPLE__)

#import <Foundation/Foundation.h>
#import <objc/runtime.h>
#import <os/lock.h>
#include <unordered_map>
#include <string>

namespace {

// Thread-safe storage for the swizzle system state
struct SwizzleState
{
  os_unfair_lock lock = OS_UNFAIR_LOCK_INIT;
  bool initialized = false;
  std::string pluginSuffix;
  std::unordered_map<std::string, Class> registeredClasses;
};

SwizzleState& GetState()
{
  static SwizzleState state;
  return state;
}

// Sanitize plugin ID to be a valid Objective-C class name suffix
// Replaces invalid characters with underscores
std::string SanitizePluginId(const char* pluginId)
{
  std::string result;
  result.reserve(strlen(pluginId) + 1);

  for (const char* p = pluginId; *p; ++p)
  {
    char c = *p;
    // Valid ObjC identifier chars: a-z, A-Z, 0-9, _
    if ((c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') ||
        c == '_')
    {
      result += c;
    }
    else if (c == '.' || c == '-' || c == ' ')
    {
      result += '_';
    }
    // Skip other invalid characters
  }

  // Ensure it doesn't start with a digit
  if (!result.empty() && result[0] >= '0' && result[0] <= '9')
  {
    result = "_" + result;
  }

  return result;
}

// Copy all methods from source class to destination class
void CopyMethods(Class srcClass, Class dstClass)
{
  unsigned int methodCount = 0;
  Method* methods = class_copyMethodList(srcClass, &methodCount);

  for (unsigned int i = 0; i < methodCount; i++)
  {
    Method method = methods[i];
    SEL selector = method_getName(method);
    IMP implementation = method_getImplementation(method);
    const char* types = method_getTypeEncoding(method);

    // Add method to destination class (won't override if exists)
    class_addMethod(dstClass, selector, implementation, types);
  }

  free(methods);
}

// Copy all protocols from source class to destination class
void CopyProtocols(Class srcClass, Class dstClass)
{
  unsigned int protocolCount = 0;
  Protocol* __unsafe_unretained* protocols = class_copyProtocolList(srcClass, &protocolCount);

  for (unsigned int i = 0; i < protocolCount; i++)
  {
    class_addProtocol(dstClass, protocols[i]);
  }

  free(protocols);
}

// Copy instance variables (ivars) layout - note: must be done before registering
void CopyIvars(Class srcClass, Class dstClass)
{
  unsigned int ivarCount = 0;
  Ivar* ivars = class_copyIvarList(srcClass, &ivarCount);

  for (unsigned int i = 0; i < ivarCount; i++)
  {
    Ivar ivar = ivars[i];
    const char* name = ivar_getName(ivar);
    const char* type = ivar_getTypeEncoding(ivar);
    size_t size = 0;
    size_t alignment = 0;

    // Get size from type encoding
    NSGetSizeAndAlignment(type, &size, &alignment);

    // Add ivar to new class
    class_addIvar(dstClass, name, size, (uint8_t)alignment, type);
  }

  free(ivars);
}

} // anonymous namespace

#pragma mark - Public API

BOOL IPlugObjCSwizzleInit(const char* pluginId)
{
  if (!pluginId || !*pluginId)
  {
    NSLog(@"IPlugObjCSwizzle: Invalid plugin ID");
    return NO;
  }

  SwizzleState& state = GetState();

  os_unfair_lock_lock(&state.lock);

  if (state.initialized)
  {
    os_unfair_lock_unlock(&state.lock);
    NSLog(@"IPlugObjCSwizzle: Already initialized with suffix: %s", state.pluginSuffix.c_str());
    return NO;
  }

  state.pluginSuffix = SanitizePluginId(pluginId);
  state.initialized = true;

  os_unfair_lock_unlock(&state.lock);

  NSLog(@"IPlugObjCSwizzle: Initialized with suffix: %s", state.pluginSuffix.c_str());
  return YES;
}

Class IPlugObjCRegisterClass(Class baseClass)
{
  if (!baseClass)
  {
    NSLog(@"IPlugObjCSwizzle: Cannot register nil class");
    return nil;
  }

  SwizzleState& state = GetState();

  os_unfair_lock_lock(&state.lock);

  if (!state.initialized)
  {
    os_unfair_lock_unlock(&state.lock);
    NSLog(@"IPlugObjCSwizzle: Not initialized, returning base class %s", class_getName(baseClass));
    return baseClass;
  }

  const char* baseName = class_getName(baseClass);
  std::string baseNameStr(baseName);

  // Check if already registered
  auto it = state.registeredClasses.find(baseNameStr);
  if (it != state.registeredClasses.end())
  {
    Class existingClass = it->second;
    os_unfair_lock_unlock(&state.lock);
    return existingClass;
  }

  // Create unique class name: BaseClass_pluginsuffix
  std::string uniqueName = baseNameStr + "_" + state.pluginSuffix;

  // Check if class already exists in runtime (e.g., from a previous load)
  Class existingClass = objc_getClass(uniqueName.c_str());
  if (existingClass)
  {
    state.registeredClasses[baseNameStr] = existingClass;
    os_unfair_lock_unlock(&state.lock);
    NSLog(@"IPlugObjCSwizzle: Reusing existing class %s", uniqueName.c_str());
    return existingClass;
  }

  // Get the superclass of the base class (we want to inherit from the same parent)
  Class superClass = class_getSuperclass(baseClass);

  // Allocate a new class pair that inherits from the same superclass as baseClass
  // This creates a sibling class rather than a subclass
  Class newClass = objc_allocateClassPair(superClass, uniqueName.c_str(), 0);

  if (!newClass)
  {
    os_unfair_lock_unlock(&state.lock);
    NSLog(@"IPlugObjCSwizzle: Failed to allocate class %s", uniqueName.c_str());
    return baseClass;
  }

  // Copy ivars before registering (ivars can only be added to unregistered classes)
  CopyIvars(baseClass, newClass);

  // Register the class before copying methods
  objc_registerClassPair(newClass);

  // Copy methods from base class to new class
  CopyMethods(baseClass, newClass);

  // Copy protocols
  CopyProtocols(baseClass, newClass);

  // Also copy class methods
  Class baseMetaClass = object_getClass(baseClass);
  Class newMetaClass = object_getClass(newClass);
  CopyMethods(baseMetaClass, newMetaClass);

  // Store in registry
  state.registeredClasses[baseNameStr] = newClass;

  os_unfair_lock_unlock(&state.lock);

  NSLog(@"IPlugObjCSwizzle: Created unique class %s for %s", uniqueName.c_str(), baseName);
  return newClass;
}

Class IPlugObjCGetClass(const char* baseClassName)
{
  if (!baseClassName)
  {
    return nil;
  }

  SwizzleState& state = GetState();

  os_unfair_lock_lock(&state.lock);

  if (!state.initialized)
  {
    os_unfair_lock_unlock(&state.lock);
    // Return the original class if not initialized
    return objc_getClass(baseClassName);
  }

  std::string baseNameStr(baseClassName);
  auto it = state.registeredClasses.find(baseNameStr);

  if (it != state.registeredClasses.end())
  {
    Class result = it->second;
    os_unfair_lock_unlock(&state.lock);
    return result;
  }

  os_unfair_lock_unlock(&state.lock);

  // Class not registered, try to register it now
  Class baseClass = objc_getClass(baseClassName);
  if (baseClass)
  {
    return IPlugObjCRegisterClass(baseClass);
  }

  NSLog(@"IPlugObjCSwizzle: Class %s not found", baseClassName);
  return nil;
}

const char* IPlugObjCGetSuffix(void)
{
  SwizzleState& state = GetState();

  os_unfair_lock_lock(&state.lock);

  if (!state.initialized)
  {
    os_unfair_lock_unlock(&state.lock);
    return NULL;
  }

  // Note: This returns a pointer to internal storage, which is stable after initialization
  const char* suffix = state.pluginSuffix.c_str();

  os_unfair_lock_unlock(&state.lock);

  return suffix;
}

BOOL IPlugObjCSwizzleIsInitialized(void)
{
  SwizzleState& state = GetState();

  os_unfair_lock_lock(&state.lock);
  BOOL result = state.initialized;
  os_unfair_lock_unlock(&state.lock);

  return result;
}

void IPlugObjCSwizzleCleanup(void)
{
  SwizzleState& state = GetState();

  os_unfair_lock_lock(&state.lock);

  // Note: We cannot truly dispose of Objective-C classes once registered,
  // as instances may still exist. We just clear our tracking.
  state.registeredClasses.clear();
  state.pluginSuffix.clear();
  state.initialized = false;

  os_unfair_lock_unlock(&state.lock);

  NSLog(@"IPlugObjCSwizzle: Cleanup complete (classes remain in runtime)");
}

#endif // __APPLE__
