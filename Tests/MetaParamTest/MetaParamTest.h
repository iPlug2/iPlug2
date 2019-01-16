#pragma once

#include "IPlug_include_in_plug_hdr.h"

const int kNumPrograms = 1;

enum EParams
{
  kParamLeftX = 0,
  kParamLeftY,
  kParamRightX,
  kParamRightY,
  kParamLink,
  kNumParams
};

enum ECtrlTags
{
  kCtrlLeftXYPad = 0,
  kCtrlRightXYPad,
  kCtrlLeftXKnob,
  kCtrlLeftYKnob,
  kCtrlRightXKnob,
  kCtrlRightYKnob
};

#if IPLUG_EDITOR
#include "IControls.h"

class IXYPad : public IControl
{
private:
  ILayerPtr mLayer;
  
public:
  IXYPad(IRECT r, const std::initializer_list<int>& params)
  : IControl(r, params)
  {
  }
  
  void Draw(IGraphics& g) override
  {
    float xpos = GetValue(0) * mRECT.W();
    float ypos = GetValue(1) * mRECT.H();
    
    g.DrawVerticalLine(mHandleColor, mRECT, 0.5);
    g.DrawHorizontalLine(mHandleColor, mRECT, 0.5);
    g.FillCircle(mHandleColor, mRECT.L + xpos, mRECT.B - ypos, mHandleRadius);
  }
  
  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    mHandleColor = COLOR_BLACK;
    OnMouseDrag(x, y, 0., 0., mod);
  }
  
  void OnMouseUp(float x, float y, const IMouseMod& mod) override
  {
    mHandleColor = COLOR_WHITE;
    SetDirty(false);
  }
  
  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override
  {
    mRECT.Constrain(x, y);
    float xn = (x - mRECT.L) / mRECT.W();
    float yn = 1.f-((y - mRECT.T) / mRECT.H());
    SetValue(xn, 0);
    SetValue(yn, 1);
    SetDirty(true);
  }
  
private:
  float mHandleRadius = 10.;
  IColor mHandleColor = COLOR_BLACK;
};
#endif

class MetaParamTest : public IPlug
{
public:
  MetaParamTest(IPlugInstanceInfo instanceInfo);

#if IPLUG_DSP // All DSP methods and member variables should be within an IPLUG_DSP guard, should you want distributed UI
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
#endif
};
