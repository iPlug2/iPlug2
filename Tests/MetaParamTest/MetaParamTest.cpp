#include "MetaParamTest.h"
#include "IPlug_include_in_plug_src.h"

#if IPLUG_EDITOR
#include "IControls.h"
#endif

MetaParamTest::MetaParamTest(IPlugInstanceInfo instanceInfo)
: IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo)
{
  GetParam(kParamLeftX)->InitDouble("X1", 100., 0., 100.0, 0.01, "%");
  GetParam(kParamLeftY)->InitDouble("Y1", 100., 0., 100.0, 0.01, "%");
  GetParam(kParamRightX)->InitDouble("X2", 100., 0., 100.0, 0.01, "%");
  GetParam(kParamRightY)->InitDouble("Y2", 100., 0., 100.0, 0.01, "%");
  GetParam(kParamLink)->InitBool("link", false);
  
#if IPLUG_EDITOR // All UI methods and member variables should be within an IPLUG_EDITOR guard, should you want distributed UI
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, 1.);
  };
  
  auto updatePeersFunc =

  mLayoutFunc = [&](IGraphics* pGraphics) {
    pGraphics->AttachCornerResizer(kUIResizerScale, false);
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->LoadFont(ROBOTTO_FN);
    pGraphics->ShowFPSDisplay(true);
    const IRECT b = pGraphics->GetBounds();
    pGraphics->AttachControl(new IVXYPadControl(b.GetGridCell(0, 2, 2), {kParamLeftX, kParamLeftY}), kCtrlLeftXYPad, "mux");
    pGraphics->AttachControl(new IVXYPadControl(b.GetGridCell(1, 2, 2), {kParamRightX, kParamRightY}), kCtrlRightXYPad, "mux");
    pGraphics->AttachControl(new IVKnobControl(b.GetGridCell(2, 2, 2).FracRectHorizontal(0.5), kParamLeftX), kCtrlLeftXKnob, "mux");
    pGraphics->AttachControl(new IVKnobControl(b.GetGridCell(2, 2, 2).FracRectHorizontal(0.5, true), kParamLeftY), kCtrlLeftYKnob, "mux");
    pGraphics->AttachControl(new IVKnobControl(b.GetGridCell(3, 2, 2).FracRectHorizontal(0.5), kParamRightX), kCtrlRightXKnob, "mux");
    pGraphics->AttachControl(new IVKnobControl(b.GetGridCell(3, 2, 2).FracRectHorizontal(0.5, true), kParamRightY), kCtrlRightYKnob, "mux");
    pGraphics->AttachControl(new IVSwitchControl(b.GetCentredInside(30), kParamLink));
    
    pGraphics->ForControlInGroup("mux", [&](IControl& control)
    {
      control.SetActionFunction([&](IControl* pCaller)
      {
        if(GetParam(kParamLink)->Bool())
        {
          if(pCaller->GetTag() == kCtrlLeftXKnob || pCaller->GetTag() == kCtrlLeftYKnob || pCaller->GetTag() == kCtrlLeftXYPad)
          {
            double x = GetParam(kParamLeftX)->GetNormalized();
            double y = GetParam(kParamLeftY)->GetNormalized();
            SetParameterValue(kParamRightX, x);
            SetParameterValue(kParamRightY, y);
            GetUI()->GetControlWithTag(kCtrlRightXKnob)->SetValueFromDelegate(x);
            GetUI()->GetControlWithTag(kCtrlRightYKnob)->SetValueFromDelegate(y);
            GetUI()->GetControlWithTag(kCtrlRightXYPad)->SetValueFromDelegate(x, 0);
            GetUI()->GetControlWithTag(kCtrlRightXYPad)->SetValueFromDelegate(y, 1);
          }
          else if(pCaller->GetTag() == kCtrlRightXKnob || pCaller->GetTag() == kCtrlRightYKnob || pCaller->GetTag() == kCtrlRightXYPad)
          {
            double x = GetParam(kParamRightX)->GetNormalized();
            double y = GetParam(kParamRightY)->GetNormalized();
            SetParameterValue(kParamLeftX, x);
            SetParameterValue(kParamLeftY, y);
            GetUI()->GetControlWithTag(kCtrlLeftXKnob)->SetValueFromDelegate(x);
            GetUI()->GetControlWithTag(kCtrlLeftYKnob)->SetValueFromDelegate(y);
            GetUI()->GetControlWithTag(kCtrlLeftXYPad)->SetValueFromDelegate(x, 0);
            GetUI()->GetControlWithTag(kCtrlLeftXYPad)->SetValueFromDelegate(y, 1);
          }
        }
      });
    });

    
  };
#endif
}

#if IPLUG_EDITOR
#endif

#if IPLUG_DSP
void MetaParamTest::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gainX = GetParam(kParamLeftX)->Value() / 100.;
  const double gainY = GetParam(kParamLeftY)->Value() / 100.;
  
  for (int s = 0; s < nFrames; s++) {
    outputs[0][s] = inputs[0][s] * gainX;
    outputs[1][s] = inputs[1][s] * gainY;
  }
}
#endif
