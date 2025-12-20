/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include "IPlugCLI_ScreenshotRenderer.h"
#include "IGraphics.h"
#include "IGraphicsHeadless.h"

using namespace iplug;
using namespace igraphics;

IPlugCLI_ScreenshotRenderer::IPlugCLI_ScreenshotRenderer(const InstanceInfo& info, const Config& config)
: IPlugCLI(info, config)
{
}

bool IPlugCLI_ScreenshotRenderer::SaveScreenshot(const char* path, float scale)
{
  if (!mMakeGraphicsFunc)
  {
    fprintf(stderr, "Error: No graphics function defined (plugin has no UI)\n");
    return false;
  }

  // Use IGEditorDelegate::OpenWindow which properly sets mGraphics and calls LayoutUI
  void* pWindow = IGEditorDelegate::OpenWindow(nullptr);
  IGraphics* pGraphics = GetUI();

  if (!pGraphics)
  {
    fprintf(stderr, "Error: Failed to create graphics instance\n");
    return false;
  }

  // Cast to headless graphics for RenderUI and SaveScreenshot
  IGraphicsHeadless* pHeadless = dynamic_cast<IGraphicsHeadless*>(pGraphics);
  if (!pHeadless)
  {
    fprintf(stderr, "Error: Graphics is not headless, cannot save screenshot\n");
    IGEditorDelegate::CloseWindow();
    return false;
  }

  // Apply the requested scale (default 1.0, use 2.0 for retina-quality)
  if (scale != 1.0f)
  {
    pHeadless->SetScale(scale);
  }

  // Render the UI
  pHeadless->RenderUI();

  // Save the screenshot
  bool success = pHeadless->SaveScreenshot(path);

  // Clean up using the delegate's CloseWindow
  IGEditorDelegate::CloseWindow();

  if (success)
  {
    fprintf(stderr, "Saved screenshot to %s\n", path);
  }
  else
  {
    fprintf(stderr, "Error: Failed to save screenshot to %s\n", path);
  }

  return success;
}
