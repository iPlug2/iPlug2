#pragma once

#include "IControl.h"
#include "modules/skottie/include/Skottie.h"

class ILottieControl : public IControl
{
public:
  ILottieControl(const IRECT& bounds)
  : IControl(bounds)
  {
    SetAnimation([&](IControl* pCaller) {
                    auto progress = pCaller->GetAnimationProgress();
                    mAnimation->seekFrameTime(pCaller->GetAnimationDuration().count() / progress, nullptr);
      
                    if(progress > 1.f)
                      pCaller->OnEndAnimation();
                 });
  }

  void Draw(IGraphics& g)
  {
    g.FillRect(COLOR_RED, mRECT);
    
    SkCanvas* pCanvas = static_cast<SkCanvas*>(g.GetDrawContext());
    
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
    SkRect rect = {0, 0, mAnimationSize.width(), mAnimationSize.height()};
    canvas->drawRect(rect, SkPaint(SkColors::kWhite));
    mAnimation->render(canvas);
  }
  
  void Load(const void* data, size_t length)
  {
    skottie::Animation::Builder builder;
    mAnimation = builder.make((const char*)data, (size_t)length);
    mSize = {0, 0};
    mAnimationSize = mAnimation ? mAnimation->size() : SkSize{0, 0};
  }
    
private:
  sk_sp<skottie::Animation> mAnimation;
  SkSize mSize;
  SkSize mAnimationSize;
  SkMatrix mMatrix;
};
