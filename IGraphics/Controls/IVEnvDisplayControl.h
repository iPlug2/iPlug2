/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @ingroup IControls
 * @copydoc IVEnvDisplayControl
 */

#include "IControl.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** A control to display an ADSR envelope */
class IVEnvDisplayControl : public IControl, public IVectorBase, public IMultiTouchControlBase
{
public:
  IVEnvDisplayControl(const IRECT& bounds, const std::initializer_list<int>& params, const char* label, const IVStyle& style = DEFAULT_STYLE)
  : IControl(bounds, params)
  , IVectorBase(style)
  {
    assert(params.size() == 4);
    
    SetWantsMultiTouch(true);
    AttachIControl(this, label);
  }
  
  void Draw(IGraphics& g) override
  {
    DrawLabel(g);
    DrawWidget(g);
  }
  
  void DrawWidget(IGraphics& g) override
  {
    g.DrawRect(GetColor(kFR), mWidgetBounds);
    
    IRECT mEnvBounds = mWidgetBounds.GetPadded(-5.f);
    
    g.DrawGrid(GetColor(kSH), mWidgetBounds, 5.f, 5.f);

    if (!g.CheckLayer(mLayer))
    {
      g.StartLayer(this, mWidgetBounds.GetMidHPadded(mWidgetBounds.W()));
      
      const auto startX = mEnvBounds.L;
      const auto startY = mEnvBounds.B;
      const auto attackX = mEnvBounds.L + (GetValue(0) * 100.f);
      const auto attackY = mEnvBounds.T;
      const auto decayX = attackX + (GetValue(1) * 100.f);
      const auto decayY = mEnvBounds.B - (GetValue(2) * mEnvBounds.H());
      const auto sustainX = decayX + 100.f;
      const auto sustainY = decayY;
      const auto releaseX = sustainX + GetValue(3) * 100.f;
      const auto releaseY = mEnvBounds.B;
      
      IColor lineColor = GetColor(kX1);
      
      g.PathLine(startX, startY, attackX, attackY);
      g.PathLine(attackX, attackY, decayX, decayY);
      g.PathLine(decayX, decayY, sustainX, sustainY);
      g.PathLine(sustainX, sustainY, releaseX, releaseY);
      g.PathStroke(lineColor, 1.f);
      
      g.FillRect(lineColor, IRECT::MakeMidXYWH(startX, startY, 5.f, 5.f));
      g.FillRect(lineColor, IRECT::MakeMidXYWH(attackX, attackY, 5.f, 5.f));
      g.FillRect(lineColor, IRECT::MakeMidXYWH(decayX, decayY, 5.f, 5.f));
      g.FillRect(lineColor, IRECT::MakeMidXYWH(sustainX, sustainY, 5.f, 5.f));
      g.FillRect(lineColor, IRECT::MakeMidXYWH(releaseX, releaseY, 5.f, 5.f));
      
      mLayer = g.EndLayer();
    }
    
    g.DrawBitmap(mLayer->GetBitmap(), mWidgetBounds, mWidgetBounds.W() * mScrollPosition, 0);
  }

  void OnMouseWheel(float x, float y, const IMouseMod &mod, float d) override
  {
    mScrollPosition = Clip(mScrollPosition + d, 0.f, 1.f);
    SetDirty(false);
  }
  
  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    AddTouch(mod.touchID, x, y, mod.touchRadius);
    OnMouseDrag(x, y, 0., 0., mod);
  }

  void OnMouseUp(float x, float y, const IMouseMod& mod) override
  {
    ReleaseTouch(mod.touchID);
    SetDirty(true);
  }

  void OnTouchCancelled(float x, float y, const IMouseMod& mod) override
  {
    OnMouseUp(x, y, mod);
  }
  
  void OnMouseDrag(float x, float y, float dx, float dy, const IMouseMod& mod) override
  {
    mWidgetBounds.Constrain(x, y);
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
  
  void OnResize() override
  {
    SetTargetRECT(MakeRects(mRECT, true));
    SetDirty(false);
  }
  
  void SetValueFromDelegate(double value, int valIdx = 0) override
  {
    if(mLayer)
      mLayer->Invalidate();
    
    IControl::SetValueFromDelegate(value, valIdx);
  }

  
private:
  ILayerPtr mLayer;
  float mScrollPosition = 0.f;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
