#include "IGraphicsTest.h"
#include "IPlug_include_in_plug_src.h"

#include "Test/TestControls.h"

IGraphicsTest::IGraphicsTest(IPlugInstanceInfo instanceInfo)
: IPLUG_CTOR(1, 1, instanceInfo)
{
}

IGraphics* IGraphicsTest::CreateGraphics()
{
  return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, 60);
}

void IGraphicsTest::LayoutUI(IGraphics* pGraphics)
{
  pGraphics->AttachCornerResizer();
//  pGraphics->EnableLiveEdit(true);
  //pGraphics->HandleMouseOver(true);
//  pGraphics->ShowControlBounds(true);
  pGraphics->LoadFont(ROBOTTO_FN);
  pGraphics->LoadFont(MONTSERRAT_FN);
  
  IRECT bounds = pGraphics->GetBounds();
  
  int cellIdx = 0;
  
  auto nextCell = [&](){
    return bounds.GetGridCell(cellIdx++, 4, 4).GetPadded(-5.);
  };

  pGraphics->AttachPanelBackground(COLOR_GRAY);

  pGraphics->AttachControl(new ILambdaControl(*this, nextCell(), [](IControl* pCaller, IGraphics& g, IRECT& r, IMouseInfo&, double) {

    static constexpr float width = 5.f;
    static constexpr float radius = 50.f;
    static constexpr float cornerSize = 10.f;

//    g.FillRect(COLOR_WHITE, r);
//    g.FillCircle(COLOR_WHITE, r.MW(), r.MH(), radius);
//    g.FillArc(COLOR_WHITE, r.MW(), r.MH(), radius, 0, 90);
//    g.FillRoundRect(COLOR_WHITE, r, cornerSize);

//    g.DrawRect(COLOR_WHITE, r, nullptr, width);
//    g.DrawCircle(COLOR_WHITE, r.MW(), r.MH(), radius, nullptr, width);
//    g.DrawArc(COLOR_WHITE, r.MW(), r.MH(), radius, 0, 90);
//    g.DrawRoundRect(COLOR_BLUE, r, cornerSize, nullptr, width);
  }));
  
  pGraphics->AttachControl(new TestGradientControl(*this, nextCell()));
  pGraphics->AttachControl(new TestPolyControl(*this, nextCell()));
  pGraphics->AttachControl(new TestArcControl(*this, nextCell()));
  pGraphics->AttachControl(new TestMultiPathControl(*this, nextCell()));
  pGraphics->AttachControl(new TestTextControl(*this, nextCell()));

#if 0
  pGraphics->AttachControl(new ITextControl(*this, nextCell(), "One!", {12, COLOR_WHITE, "Roboto-Regular", IText::kStyleNormal, IText::kAlignNear, IText::kVAlignTop}));
  pGraphics->AttachControl(new ITextControl(*this, nextCell(), "Two!", {18, COLOR_GREEN, "Montserrat-LightItalic", IText::kStyleNormal, IText::kAlignCenter, IText::kVAlignMiddle}));
  pGraphics->AttachControl(new ITextControl(*this, nextCell(), "Three!", {24, COLOR_RED, "Roboto-Regular", IText::kStyleItalic, IText::kAlignFar, IText::kVAlignBottom}));
  pGraphics->AttachControl(new ITextControl(*this, nextCell(), "Four!", {40, COLOR_ORANGE, "Roboto-Regular", IText::kStyleNormal, IText::kAlignCenter, IText::kVAlignBottom}));
#endif
}
