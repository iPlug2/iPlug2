#include "IGraphicsTest.h"
#include "IPlug_include_in_plug_src.h"

#include "Test/TestControls.h"

enum EParam
{
  kParamDummy = 0,
  kNumParams
};

enum EControlTags
{
  kCtrlTagSize = 0
};

IGraphicsTest::IGraphicsTest(IPlugInstanceInfo instanceInfo)
: IPLUG_CTOR(kNumParams, 1, instanceInfo)
{
  
#if IPLUG_EDITOR
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, 1.);
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    
    if(pGraphics->NControls())
    {
      IRECT bounds = pGraphics->GetBounds();
      pGraphics->GetBackgroundControl()->SetRECT(bounds);
      pGraphics->GetControlWithTag(kCtrlTagSize)->SetRECT(bounds);
      DBGMSG("SELECTED: W %i, H%i\n", pGraphics->Width(), pGraphics->Height());
      
      return;
    }
    
    pGraphics->AttachCornerResizer(EUIResizerMode::kUIResizerScale, true);
    pGraphics->HandleMouseOver(true);
    pGraphics->EnableTooltips(true);
    
    pGraphics->SetKeyHandlerFunc([&](const IKeyPress& key)
    {
      switch (key.VK) {
        case kVK_TAB:
          dynamic_cast<IPanelControl*>(GetUI()->GetBackgroundControl())->SetPattern(IColor::GetRandomColor());
          break;
          
        default:
          break;
      }
      return true;
    });
    
    pGraphics->LoadFont(ROBOTTO_FN);
    pGraphics->LoadFont(MONTSERRAT_FN);
    ISVG tiger = pGraphics->LoadSVG(TIGER_FN);
    IBitmap smiley = pGraphics->LoadBitmap(SMILEY_FN);
    IBitmap base = pGraphics->LoadBitmap(BASE_FN);
    IBitmap mask = pGraphics->LoadBitmap(MASK_FN);
    IBitmap top = pGraphics->LoadBitmap(TOP_FN);

    IRECT bounds = pGraphics->GetBounds();
    
    int cellIdx = 0;
    
    auto nextCell = [&](){
      return bounds.GetPadded(-10).GetGridCell(cellIdx++, 4, 6).GetPadded(-5.);
    };
    
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->AttachControl(new TestSizeControl(bounds), kCtrlTagSize);

    pGraphics->AttachControl(new ILambdaControl(nextCell(), [](ILambdaControl* pCaller, IGraphics& g, IRECT& r) {
      
//      const float width = 5.f;
       const float radius = r.W();
//      const float cornerSize = 10.f;
      
      //    g.FillRect(COLOR_WHITE, r);
      //    g.FillCircle(COLOR_WHITE, r.MW(), r.MH(), radius);
      //    g.FillArc(COLOR_WHITE, r.MW(), r.MH(), radius, 0, 90);
      //    g.FillRoundRect(COLOR_WHITE, r, cornerSize);

      //    g.DrawDottedLine(COLOR_WHITE, r.L, r.T, r.R, r.MH());
      //    g.DrawRect(COLOR_WHITE, r, nullptr, width);
      //    g.DrawCircle(COLOR_WHITE, r.MW(), r.MH(), radius, nullptr, width);
      //    g.DrawArc(COLOR_WHITE, r.MW(), r.MH(), radius, 0, 90);
      //    g.DrawRoundRect(COLOR_BLUE, r, cornerSize, nullptr, width);
      
      const float x = r.MW();
      const float y = r.MH();
      const float rotate = pCaller->GetAnimationProgress() * PI;
      
      for(int index = 0, limit = 40; index < limit; ++index)
      {
        float firstAngle = (index * 2 * PI) / limit;
        float secondAngle = ((index + 1) * 2 * PI) / limit;
        
        g.PathTriangle(x, y,
                       x + std::sin(firstAngle + rotate) * radius, y + std::cos(firstAngle + rotate) * radius,
                       x + std::sin(secondAngle + rotate) * radius, y + std::cos(secondAngle + rotate) * radius);
        
        if(index % 2)
          g.PathFill(COLOR_RED);
        else
          g.PathFill(COLOR_BLUE);
      }
      
      
    }, 1000, false));
    
    pGraphics->AttachControl(new TestGradientControl(nextCell(), kParamDummy));
    pGraphics->AttachControl(new TestColorControl(nextCell()));
    pGraphics->AttachControl(new TestPolyControl(nextCell(), kParamDummy));
    pGraphics->AttachControl(new TestArcControl(nextCell(), kParamDummy));
    pGraphics->AttachControl(new TestMultiPathControl(nextCell(), kParamDummy));
    pGraphics->AttachControl(new TestTextControl(nextCell()));
    pGraphics->AttachControl(new TestAnimationControl(nextCell()));
    pGraphics->AttachControl(new TestDrawContextControl(nextCell()));
    pGraphics->AttachControl(new TestSVGControl(nextCell(), tiger));
    pGraphics->AttachControl(new TestImageControl(nextCell()));
    pGraphics->AttachControl(new TestLayerControl(nextCell()));
    pGraphics->AttachControl(new TestBlendControl(nextCell(), smiley));
    pGraphics->AttachControl(new TestDropShadowControl(nextCell(), tiger));
    pGraphics->AttachControl(new TestCursorControl(nextCell()));
    pGraphics->AttachControl(new TestKeyboardControl(nextCell()));
    pGraphics->AttachControl(new TestShadowGradientControl(nextCell()));

    WDL_String path;
    //    DesktopPath(path);
    path.Set(__FILE__);
    path.remove_filepart();
#ifdef OS_WIN
    path.Append("\\resources\\img\\");
#else
    path.Append("/resources/img/");
#endif
    pGraphics->AttachControl(new TestDirBrowseControl(nextCell(), "png", path.Get()));

    IRECT r = nextCell();
    pGraphics->AttachControl(new TestRotatingMaskControl(r.L, r.T, base, mask, top));

#if 0
    pGraphics->AttachControl(new ITextControl(nextCell(), "Hello World!", {24, COLOR_WHITE, "Roboto-Regular", IText::kStyleNormal, IText::kAlignNear, IText::kVAlignTop, 90}));
    pGraphics->AttachControl(new ITextControl(nextCell(), "Two!", {18, COLOR_GREEN, "Montserrat-LightItalic", IText::kStyleItalic, IText::kAlignCenter, IText::kVAlignMiddle, 45}));
    pGraphics->AttachControl(new ITextControl(nextCell(), "Three!", {24, COLOR_RED, "Roboto-Regular", IText::kStyleNormal, IText::kAlignFar, IText::kVAlignBottom}));
    pGraphics->AttachControl(new ITextControl(nextCell(), "Four!", {40, COLOR_ORANGE, "Roboto-Regular", IText::kStyleNormal, IText::kAlignCenter, IText::kVAlignBottom}));
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
