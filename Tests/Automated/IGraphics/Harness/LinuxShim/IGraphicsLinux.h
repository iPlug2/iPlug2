/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @brief Test-only stand-in for IGraphics/Platforms/IGraphicsLinux.h
 *
 * On Linux, IGEditorDelegate talks directly to IGraphicsLinux (static_cast from IGraphics*) for host scaling,
 * host resizing and deferred window closing. The automated tests have no X11 window, so this header shadows the real
 * one on the include path and provides just the part of the IGraphicsLinux interface that IGEditorDelegate uses.
 * IGraphicsHeadless derives from it on Linux so those casts stay valid.
 */

#include "IGraphics.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

class IGraphicsLinux : public IGraphics
{
public:
  using IGraphics::IGraphics;

  void RequestWindowClose() { mCloseRequested = true; }
  bool WindowCloseRequested() const { return mCloseRequested; }
  bool IsInPlatformCallback() const { return mDispatchDepth != 0; }

  void SetScreenScaleFromHost(float scale) { SetScreenScale(scale); }

  void ResizeFromHost(int width, int height)
  {
    const float scale = GetScreenScale();
    Resize(static_cast<int>(width / scale), static_cast<int>(height / scale), GetDrawScale(), false);
  }

protected:
  bool mCloseRequested = false;
  int mDispatchDepth = 0;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
