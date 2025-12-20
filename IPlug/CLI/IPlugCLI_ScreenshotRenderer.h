/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#ifndef _IPlugCLI_ScreenshotRenderer_
#define _IPlugCLI_ScreenshotRenderer_

/**
 * @file
 * @copydoc IPlugCLI_ScreenshotRenderer
 */

#include "IPlugCLI.h"

BEGIN_IPLUG_NAMESPACE

/** Command-line interface base class for an IPlug plug-in with IGraphics support.
 *  Inherits from IPlugCLI for audio processing, adds UI screenshot generation.
 *  @ingroup APIClasses */
class IPlugCLI_ScreenshotRenderer : public IPlugCLI
{
public:
  IPlugCLI_ScreenshotRenderer(const InstanceInfo& info, const Config& config);
  virtual ~IPlugCLI_ScreenshotRenderer() = default;

  /** Render the UI and save a screenshot to a file
   * @param path Output file path (should end in .png)
   * @param scale Scale factor for rendering (1.0 = normal, 2.0 = retina)
   * @return true on success */
  bool SaveScreenshot(const char* path, float scale = 1.0f);
};

END_IPLUG_NAMESPACE

#endif
