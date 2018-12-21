#include "IGraphicsTest.h"
#include "IPlug_include_in_plug_src.h"

#include "Test/TestControls.h"

enum EControlTags
{
  kSizeControl = 0,
};

IGraphicsTest::IGraphicsTest(IPlugInstanceInfo instanceInfo)
: IPLUG_CTOR(1, 1, instanceInfo)
{
  
#if IPLUG_EDITOR
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, 60, 1.);
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    
    if(pGraphics->NControls())
    {
      IRECT bounds = pGraphics->GetBounds();
      pGraphics->GetControl(0)->SetRECT(bounds);
      pGraphics->GetControlWithTag(kSizeControl)->SetRECT(bounds);
      DBGMSG("SELECTED: W %i, H%i\n", pGraphics->Width(), pGraphics->Height());
      
      return;
    }
    
    pGraphics->AttachCornerResizer(EUIResizerMode::kUIResizerScale, true);
    pGraphics->HandleMouseOver(true);
    //  pGraphics->EnableLiveEdit(true);
    //  pGraphics->ShowControlBounds(true);
    pGraphics->LoadFont(ROBOTTO_FN);
    pGraphics->LoadFont(MONTSERRAT_FN);
    ISVG tiger = pGraphics->LoadSVG(TIGER_FN);

    IRECT bounds = pGraphics->GetBounds();
    
    int cellIdx = 0;
    
    auto nextCell = [&](){
      return bounds.GetGridCell(cellIdx++, 4, 4).GetPadded(-5.);
    };
    
    pGraphics->AttachPanelBackground(COLOR_GRAY);
     
    pGraphics->AttachControl(new ILambdaControl(*this, nextCell(), [](IControl* pCaller, IGraphics& g, IRECT& r, IMouseInfo&, double t) {
      
//      static constexpr float width = 5.f;
      static constexpr float radius = 50.f;
//      static constexpr float cornerSize = 10.f;
      
      //    g.FillRect(COLOR_WHITE, r);
      //    g.FillCircle(COLOR_WHITE, r.MW(), r.MH(), radius);
      //    g.FillArc(COLOR_WHITE, r.MW(), r.MH(), radius, 0, 90);
      //    g.FillRoundRect(COLOR_WHITE, r, cornerSize);
      
      g.DrawDottedLine(COLOR_WHITE, r.L, r.T, r.R, r.MH());
      //    g.DrawRect(COLOR_WHITE, r, nullptr, width);
      //    g.DrawCircle(COLOR_WHITE, r.MW(), r.MH(), radius, nullptr, width);
          g.DrawArc(COLOR_WHITE, r.MW(), r.MH(), radius, 0, 90);
      //    g.DrawRoundRect(COLOR_BLUE, r, cornerSize, nullptr, width);
    }, 1000, false));
    
    pGraphics->AttachControl(new TestGradientControl(*this, nextCell()));
    pGraphics->AttachControl(new TestPolyControl(*this, nextCell()));
    pGraphics->AttachControl(new TestArcControl(*this, nextCell()));
    pGraphics->AttachControl(new TestMultiPathControl(*this, nextCell()));
    pGraphics->AttachControl(new TestTextControl(*this, nextCell()));
    pGraphics->AttachControl(new TestAnimationControl(*this, nextCell()));
    pGraphics->AttachControl(new TestDrawContextControl(*this, nextCell()));
    pGraphics->AttachControl(new TestSVGControl(*this, nextCell(), tiger));
    pGraphics->AttachControl(new TestSizeControl(*this, bounds), kSizeControl);
    pGraphics->AttachControl(new TestLayerControl(*this, nextCell()));

#if 1
    pGraphics->AttachControl(new ITextControl(*this, nextCell(), "Hello World!", {24, COLOR_WHITE, "Roboto-Regular", IText::kStyleNormal, IText::kAlignNear, IText::kVAlignTop, 90}));
    pGraphics->AttachControl(new ITextControl(*this, nextCell(), "Two!", {18, COLOR_GREEN, "Montserrat-LightItalic", IText::kStyleItalic, IText::kAlignCenter, IText::kVAlignMiddle, 45}));
    pGraphics->AttachControl(new ITextControl(*this, nextCell(), "Three!", {24, COLOR_RED, "Roboto-Regular", IText::kStyleNormal, IText::kAlignFar, IText::kVAlignBottom}));
    pGraphics->AttachControl(new ITextControl(*this, nextCell(), "Four!", {40, COLOR_ORANGE, "Roboto-Regular", IText::kStyleNormal, IText::kAlignCenter, IText::kVAlignBottom}));
#endif
      
  };
  
#endif
}

void IGraphicsTest::OnHostSelectedViewConfiguration(int width, int height)
{
  DBGMSG("SELECTED: W %i, H%i\n", width, height);
//  const float scale = (float) height / (float) PLUG_HEIGHT;
  
//  if(GetUI())
//    GetUI()->Resize(width, height, 1);
}

bool IGraphicsTest::OnHostRequestingSupportedViewConfiguration(int width, int height)
{
  DBGMSG("SUPPORTED: W %i, H%i\n", width, height); return true;
}
