/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#ifndef IGRAPHICS_SKIA
#error This IControl only works with the Skia graphics backend
#endif

/**
 * @file
 * @copydoc ISkLottieControl
 */

#ifndef IGRAPHICS_SKIA
#error ISkLottieControl requires the IGRAPHICS_SKIA backend
#endif

#include "IControl.h"
#include "SkMatrix.h"
#include "modules/skottie/include/Skottie.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** A control that hosts a Lottie animation, via Skia's Skottie module
 * https://skia.org/user/modules/skottie
 * @ingroup IControls */
class ISkLottieControl : public IControl
{
public:
  ISkLottieControl(const IRECT& bounds)
  : IControl(bounds)
  {
  }

  void Draw(IGraphics& g) override
  {    
    SkCanvas* pCanvas = static_cast<SkCanvas*>(g.GetDrawContext());
    
    if(mAnimation && GetAnimationFunction())
      DoDraw(SkSize::Make(mRECT.W(), mRECT.H()), pCanvas);
  }
  
  void DoDraw(SkSize size, SkCanvas* canvas)
  {
    if (size.width() != mSize.width() || size.height() != mSize.height())
    {
      // Cache the current matrix; change only if size changes.
      if (mAnimationSize.width() > 0 && mAnimationSize.height() > 0)
      {
        float scale = std::min(size.width() / mAnimationSize.width(),
                               size.height() / mAnimationSize.height());
        mMatrix.setScaleTranslate( scale, scale,
                (size.width()  - mAnimationSize.width()  * scale) * 0.5f,
                (size.height() - mAnimationSize.height() * scale) * 0.5f);
      }
      else
      {
        mMatrix = SkMatrix();
      }
      mSize = size;
    }
    canvas->concat(mMatrix);
    mAnimation->render(canvas);
  }
  
  void LoadFile(const char* path)
  {
    skottie::Animation::Builder builder;
    mAnimation = builder.makeFromFile(path);
    mSize = {0, 0};
  
    if(mAnimation) {
      mAnimationSize = mAnimation->size();
    
      SetActionFunction([&](IControl* pCaller) {
        if(mAnimation) {
          SetAnimation([&](IControl* pCaller) {
                        auto progress = pCaller->GetAnimationProgress();
                        mAnimation->seekFrameTime((pCaller->GetAnimationDuration().count() / 1000.) * progress, nullptr);
          
                        if(progress > 1.f)
                          pCaller->OnEndAnimation();
                     }, mAnimation->duration() * 1000.);
        }
        });
    }
    else {
      mAnimationSize = {0, 0};
    }
  }
  
  void LoadData(const void* data, size_t length)
  {
    skottie::Animation::Builder builder;
    mAnimation = builder.make((const char*) data, (size_t)length);
    mSize = {0, 0};
    mAnimationSize = mAnimation ? mAnimation->size() : SkSize{0, 0};

    SetActionFunction([&](IControl* pCaller) {
        SetAnimation([&](IControl* pCaller) {
                      auto progress = pCaller->GetAnimationProgress();
                      mAnimation->seekFrameTime((pCaller->GetAnimationDuration().count() / 1000.) * progress, nullptr);

                      if(progress > 1.f)
                        pCaller->OnEndAnimation();
                   }, mAnimation->duration() * 1000.);
      });
  }
  
  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    if (mod.S)
    {
      WDL_String fileName, path {"~/Desktop"};

      GetUI()->PromptForFile(fileName, path, EFileAction::Open, "svg", 
      [this](const WDL_String& fileName, const WDL_String& path) {
        if (fileName.GetLength())
          LoadFile(fileName.Get());
          SetDirty(false);
      });
    }
    else
      SetDirty(true);
  }
    
private:
  sk_sp<skottie::Animation> mAnimation;
  SkSize mSize;
  SkSize mAnimationSize;
  SkMatrix mMatrix;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
