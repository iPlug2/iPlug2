/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for more info.

 ==============================================================================
*/

#pragma once

#include <string>

#include "IPlugPlatform.h"

BEGIN_IPLUG_NAMESPACE

/**
 * Platform-specific utility functions
 */
namespace PlatformUtils
{
  /** Get the path to the application support directory */
  std::string GetAppSupportPath(const char* appName);

  /** Get the path to the application's resource directory */
  std::string GetResourcePath();

  /** Get screen DPI scale factor */
  float GetScreenScale(void* windowHandle);

  /** Check if app is running sandboxed (macOS) */
  bool IsAppSandboxed();

  /** Open a URL in the default browser */
  void OpenURL(const char* url);

  /** Show a native message box */
  void ShowMessageBox(const char* title, const char* message, void* parentWindow);

  /** Prevent multiple instances of the app from running */
  bool PreventMultipleInstances(const char* appName);

  /** Release the multiple instance lock */
  void ReleaseInstanceLock();
}

END_IPLUG_NAMESPACE
