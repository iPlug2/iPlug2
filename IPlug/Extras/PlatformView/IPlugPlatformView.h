 /*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

#include "IPlugPlatform.h"
#include "wdlstring.h"
#include <functional>

#if defined OS_MAC
  #define PLATFORM_VIEW NSView
  #define PLATFORM_RECT NSRect
  #define MAKERECT NSMakeRect
#elif defined OS_IOS
  #define PLATFORM_VIEW UIView
  #define PLATFORM_RECT CGRect
  #define MAKERECT CGRectMake
#elif defined OS_WIN
  #define PLATFORM_VIEW HWND
#endif

BEGIN_IPLUG_NAMESPACE

/** IPlatformView is a base interface for hosting a platform view inside an IPlug plug-in's UI */
class IPlatformView
{
public:
  IPlatformView(bool opaque = true);
  virtual ~IPlatformView();

  void* CreatePlatformView(void* pParent, float x, float y, float w, float h, float scale = 1.);
  void RemovePlatformView();

  void SetChildViewBounds(float x, float y, float w, float h, float scale = 1.);
  
private:
  bool mOpaque = true;
  void* mPlatformView = nullptr;
};

END_IPLUG_NAMESPACE
