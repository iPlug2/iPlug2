#include "MetaParamTest.h"
#include "IPlug_include_in_plug_src.h"

#if IPLUG_EDITOR
#include "IControls.h"
#endif

#if IPLUG_EDITOR
struct FourValues : public IControl
{
  FourValues(const IRECT& bounds, int paramIdx1, int paramIdx2, int paramIdx3, int paramIdx4)
  : IControl(bounds, {paramIdx1, paramIdx2, paramIdx3, paramIdx4})
  {
    mText.mAlign = EAlign::Center;
    mText.mVAlign = EVAlign::Middle;
    mDisablePrompt = false;
  }
  
  int GetValIdxForPos(float x, float y) const override
  {
    int a = x > mRECT.MW();
    int b = y > mRECT.MH();
    
    return b * 2 + a;
  }
  
  IRECT GetRect(int num)
  {
    return mRECT.GetGridCell(num / 2, num % 2, 2, 2).GetPadded(-120, -5, -120, -5);
  }
  
  void OnMouseDblClick(float x, float y, const IMouseMod&) override
  {
    PromptUserInput(GetRect(GetValIdxForPos(x, y)), GetValIdxForPos(x, y));
  }
  
  void Draw(IGraphics& g) override
  {
    auto drawVal = [this, &g](int num)
    {
      WDL_String str;
      IRECT r = GetRect(num);
      GetParam(num)->GetDisplay(str);
      g.DrawText(mText, str.Get(), r);
    };
    
    for (int i = 0; i < 4; i++)
      drawVal(i);
  }
};
#endif

MetaParamTest::MetaParamTest(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kParamLeftX)->InitDouble("X1", 100., 0., 100.0, 0.01, "%");
  GetParam(kParamLeftY)->InitDouble("Y1", 100., 0., 100.0, 0.01, "%");
  GetParam(kParamRightX)->InitDouble("X2", 100., 0., 100.0, 0.01, "%");
  GetParam(kParamRightY)->InitDouble("Y2", 100., 0., 100.0, 0.01, "%");
  GetParam(kParamLink)->InitBool("link", false);
    
  GetParam(kParamEnum1)->InitEnum("Enum 1", 0, 4);
  GetParam(kParamEnum1)->SetDisplayText(0, "Apple");
  GetParam(kParamEnum1)->SetDisplayText(1, "Orange");
  GetParam(kParamEnum1)->SetDisplayText(2, "Pear");
  GetParam(kParamEnum1)->SetDisplayText(3, "Banana");
  GetParam(kParamEnum2)->InitEnum("Enum 2", 0, 4);
  GetParam(kParamEnum2)->SetDisplayText(0, "Spades");
  GetParam(kParamEnum2)->SetDisplayText(1, "Diamonds");
  GetParam(kParamEnum2)->SetDisplayText(2, "Hearts");
  GetParam(kParamEnum2)->SetDisplayText(3, "Clubs");
  
#if IPLUG_EDITOR
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_HEIGHT));
  };
  
  auto updatePeersFunc =

  mLayoutFunc = [&](IGraphics* pGraphics) {

    pGraphics->EnableMultiTouch(true);
    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    pGraphics->ShowFPSDisplay(true);
    const IRECT b = pGraphics->GetBounds();
    pGraphics->AttachControl(new IVXYPadControl(b.GetGridCell(0, 2, 2), {kParamLeftX, kParamLeftY}), kCtrlLeftXYPad, "mux");
    pGraphics->AttachControl(new IVXYPadControl(b.GetGridCell(1, 2, 2), {kParamRightX, kParamRightY}), kCtrlRightXYPad, "mux");
    pGraphics->AttachControl(new IVKnobControl(b.GetGridCell(2, 2, 2).FracRectHorizontal(0.5), kParamLeftX), kCtrlLeftXKnob, "mux");
    pGraphics->AttachControl(new IVKnobControl(b.GetGridCell(2, 2, 2).FracRectHorizontal(0.5, true), kParamLeftY), kCtrlLeftYKnob, "mux");
    pGraphics->AttachControl(new IVKnobControl(b.GetGridCell(3, 2, 2).FracRectHorizontal(0.5), kParamRightX), kCtrlRightXKnob, "mux");
    pGraphics->AttachControl(new IVKnobControl(b.GetGridCell(3, 2, 2).FracRectHorizontal(0.5, true), kParamRightY), kCtrlRightYKnob, "mux");
    pGraphics->AttachControl(new IVSwitchControl(b.GetCentredInside(50), kParamLink));
//    pGraphics->AttachControl(new FourValues(b.GetFromBottom(100).GetPadded(-20), kParamEnum1, kParamLeftX, kParamLeftY, kParamEnum2));
    
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
