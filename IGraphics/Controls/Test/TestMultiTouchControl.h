/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc TestMTControl
 */

#include "IControl.h"
#include <map>

 /** Control to test multi touch
  *   @ingroup TestControls */
class TestMTControl : public IControl
                    , public IMultiTouchControlBase
{
public:
  TestMTControl(const IRECT& bounds)
   : IControl(bounds)
  {
    SetWantsMultiTouch(true);
  }
  
  void Draw(IGraphics& g) override
  {
    std::vector<ITouchID> touches;
    g.GetTouches(this, touches);
    WDL_String str;
    str.SetFormatted(32, "NUM TOUCHES: %i", static_cast<int>(touches.size()));
    g.DrawText(IText(20), str.Get(), mRECT);

    g.DrawRect(COLOR_BLACK, mRECT);
    
    if (g.CheckLayer(mLayer))
    {
      g.ResumeLayer(mLayer);
    
      WDL_String str;

      for (auto& touchPair : mTrackedTouches)
      {
        TrackedTouch* pTouch = &touchPair.second;
        int t = pTouch->index;
        float dim = pTouch->radius > 0.f ? pTouch->radius : 50.f;
        IRECT r {pTouch->x-dim,pTouch->y-dim,pTouch->x+dim, pTouch->y+dim};
        g.FillEllipse(GetRainbow(t), r);
        g.DrawEllipse(COLOR_BLACK, r);
        Milliseconds duration =  std::chrono::high_resolution_clock::now() - pTouch->startTime;
        str.SetFormatted(32, "%i: %i", t, static_cast<int>(duration.count()));
        g.DrawText(IText(20.f), str.Get(), r);
      }
    }
    else
    {
      g.StartLayer(this, mRECT);
    }
    
    mLayer = g.EndLayer();
    g.DrawLayer(mLayer);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    AddTouch(mod.touchID, x, y, mod.touchRadius);
  }

  void OnMouseUp(float x, float y, const IMouseMod& mod) override
  {
    ReleaseTouch(mod.touchID);
    
    if(NTrackedTouches() == 0)
    {
      mLayer->Invalidate();
    }
    
    SetDirty(true);
  }

  void OnTouchCancelled(float x, float y, const IMouseMod& mod) override
  {
    OnMouseUp(x, y, mod);
  }
  
  void OnMouseDrag(float x, float y, float dx, float dy, const IMouseMod& mod) override
  {
    UpdateTouch(mod.touchID, x, y, mod.touchRadius);
    SetDirty(true);
  }

  bool IsDirty() override
  {
    if(NTrackedTouches())
      return true;
    else
      return IControl::IsDirty();
  }
  
public:
  ILayerPtr mLayer;
};
