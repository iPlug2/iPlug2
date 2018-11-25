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
  pGraphics->AttachPanelBackground(COLOR_GRAY);
  pGraphics->AttachCornerResizer();
//  pGraphics->EnableLiveEdit(true);
  //pGraphics->HandleMouseOver(true);
  //pGraphics->ShowControlBounds(true);
  pGraphics->LoadFont(ROBOTTO_FN);
  pGraphics->LoadFont(MONTSERRAT_FN);
  
  IRECT bounds = pGraphics->GetBounds();
  
  int rows = 4;
  int cols = 4;
  int cellIdx = 0;
  
  auto nextCell = [&](){
    return bounds.GetGridCell(cellIdx++, rows, cols).GetPadded(-5.);
  };

  pGraphics->AttachControl(new TestGradientControl(*this, nextCell()));
  pGraphics->AttachControl(new TestPolyControl(*this, nextCell()));
  pGraphics->AttachControl(new TestArcControl(*this, nextCell()));
  pGraphics->AttachControl(new TestMultiPathControl(*this, nextCell()));
  pGraphics->AttachControl(new TestTextControl(*this, nextCell()));
  
#if 1
  pGraphics->AttachControl(new ITextControl(*this, nextCell(), "One!", {12, COLOR_WHITE, "Roboto-Regular", IText::kStyleNormal, IText::kAlignNear, IText::kVAlignTop}));
  pGraphics->AttachControl(new ITextControl(*this, nextCell(), "Two!", {18, COLOR_GREEN, "Montserrat-LightItalic", IText::kStyleNormal, IText::kAlignCenter, IText::kVAlignMiddle}));
  pGraphics->AttachControl(new ITextControl(*this, nextCell(), "Three!", {24, COLOR_RED, "Roboto-Regular", IText::kStyleItalic, IText::kAlignFar, IText::kVAlignBottom}));
  pGraphics->AttachControl(new ITextControl(*this, nextCell(), "Four!", {40, COLOR_ORANGE, "Roboto-Regular", IText::kStyleNormal, IText::kAlignCenter, IText::kVAlignBottom}));
#endif
}
