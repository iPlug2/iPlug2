#pragma once

#include "IControl.h"
#if defined IGRAPHICS_NANOVG

/**
 * @file
 * @copydoc ShaderControl
 */

using namespace iplug;
using namespace igraphics;

class ShaderControl : public IKnobControlBase
{
public:
  ShaderControl(const IRECT& bounds, int paramIdx, const IColor& backgroundColor = IColor(255, 0, 0, 0));
  
  ~ShaderControl();
  
  void OnResize() override
  {
    invalidateFBO = true;
  }

  void OnRescale() override
  {
    invalidateFBO = true;
  }
  
  void Draw(IGraphics& g) override;
  void CleanUp();

private:
  void* mFBO = nullptr;
  void* mRenderPassDescriptor = nullptr;
  bool invalidateFBO = true;
  IColor mBackgroundColor;
  
  // Static resources shared across instances
#ifdef IGRAPHICS_METAL
  static void* sRenderPipeline;
#else
  int mInitialFBO = 0;
  static unsigned int sProgram;
  unsigned int mVAO = 0;
  unsigned int mVBO = 0;
#endif
  static int sInstanceCount;
};

#else
#error "Not Supported"
#endif
