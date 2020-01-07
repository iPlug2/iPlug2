/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc TestGesturesControl
 */

#include "IControl.h"

 /** Control to test multi gesture recognizers
  *   @ingroup TestControls */
class TestGesturesControl : public ITextControl
{
public:
  TestGesturesControl(IRECT bounds)
   : ITextControl(bounds, "Do a gesture...")
  {
    mIgnoreMouse = false;
    #ifdef OS_IOS
    mText = IText(60.f);
    #endif
  }
  
  void Draw(IGraphics& g) override
  {
  #ifdef OS_IOS
    g.DrawDottedRect(COLOR_BLACK, mRECT);

    g.PathTransformSave();
    g.PathTransformTranslate(mRECT.MW(), mRECT.MH());
    g.PathTransformRotate(mAngle);
    g.PathTransformScale(mScale);
    g.PathRect(IRECT(-100, -100, 100, 100));
    g.PathFill(COLOR_RED);
    g.PathTransformRestore();
    
    if (mStr.GetLength())
      g.DrawText(mText, mStr.Get(), mRECT);
    #else
        g.DrawText(mText, "UNSUPPORTED", mRECT);
    #endif
  }
  
  void OnInit() override
  {
    int idx = 1; // 0 = unknown
    for(auto& type : { EGestureType::DoubleTap,
                       EGestureType::TripleTap,
                       EGestureType::LongPress1,
                       EGestureType::LongPress2,
                       EGestureType::SwipeLeft,
                       EGestureType::SwipeRight,
                       EGestureType::SwipeUp,
                       EGestureType::SwipeDown,
                       EGestureType::Pinch,
                       EGestureType::Rotate,
                       EGestureType::Pan} )
    {
      AttachGestureRecognizer(type, [&, idx](IControl* pCaller, const IGestureInfo& info) {
        SetStr(kGestureTypeStrs[idx]);
        
        if(info.type == EGestureType::Rotate)
          mAngle = info.angle;
        
        if(info.type == EGestureType::Pinch)
          mScale = info.scale;
        
        if(info.type == EGestureType::DoubleTap)
        {
          mAngle = 0.;
          mScale = 1.;
        }
        
        SetDirty();
       });
      idx++;
    }
  }
  
private:
  float mAngle = 0.f;
  float mScale = 1.f;
};
