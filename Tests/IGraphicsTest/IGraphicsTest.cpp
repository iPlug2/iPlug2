#include "IGraphicsTest.h"
#include "IPlug_include_in_plug_src.h"

#if IPLUG_EDITOR
#include "IControls.h"
#include "Test/TestControls.h"
#endif

#include <filesystem>

enum EParam
{
  kParamDummy = 0,
  kNumParams
};

enum EControlTags
{
  kCtrlTagTestControl = 0
};

IGraphicsTest::IGraphicsTest(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, 1))
{
  GetParam(kParamDummy)->InitPercentage("Dummy", 100.f);
  
#if IPLUG_EDITOR
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    
    if(pGraphics->NControls())
    {
      IRECT bounds = pGraphics->GetBounds();
      pGraphics->GetBackgroundControl()->SetRECT(bounds);
//      pGraphics->GetControlWithTag(kCtrlTagSize)->SetRECT(bounds);
      DBGMSG("SELECTED: W %i, H%i\n", pGraphics->Width(), pGraphics->Height());
      
      return;
    }

    pGraphics->EnableMouseOver(true);
    pGraphics->EnableTooltips(true);
    pGraphics->EnableMultiTouch(true);
    
    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, true);
    
    pGraphics->SetKeyHandlerFunc([&](const IKeyPress& key, bool isUp) {
      if(!isUp) {
        switch (key.VK) {
          case kVK_TAB:
            GetUI()->GetBackgroundControl()->As<IPanelControl>()->SetPattern(IColor::GetRandomColor());
            break;
            
          default:
            break;
        }
        return true;
      }
      
      return false;
    });
    
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    if (!pGraphics->LoadFont("Alternative Font", "Times New Roman", ETextStyle::Normal)) {
      // This covers cases where we can't load system fonts, or the font doesn't exist
      pGraphics->LoadFont("Alternative Font", MONTSERRAT_FN);
    }
    pGraphics->LoadFont("Montserrat-LightItalic", MONTSERRAT_FN);

    IRECT bounds = pGraphics->GetBounds().GetPadded(-20.f);
    auto testRect = bounds.GetFromTop(480.f).GetCentredInside(480.f);

    pGraphics->AttachPanelBackground(COLOR_GRAY);
    
    auto testNames = {
    "Gradient",
    "Multi-stop gradient",
    "Polygon",
    "Arcs",
    "Beziers",
    "MultiPath",
    "Text",
    "Animation",
    "Draw contexts",
    "SVG",
    "Image",
    "Layer",
    "Blend modes",
    "DropShadow",
    "Cursor",
    "Keyboard",
    "ShadowGradient",
    "Font",
    "TextOrientation",
    "TextSize",
    "MPS (NanoVG MTL only)",
    "Custom Shader (NanoVG only)",
    "Gesture Recognizers (iOS only)",
    "MultiTouch (iOS/Win/Web only)",
    "FlexBox",
    "Mask",
    "DirBrowse",
    };
    
    WDL_String resourcePath;

    #ifdef OS_MAC
    BundleResourcePath(resourcePath, BUNDLE_ID);
    #else
    namespace fs = std::filesystem;
    fs::path mainPath(__FILE__);
    fs::path imgResourcesPath = mainPath.parent_path() / "Resources" / "img";
    resourcePath.Set(imgResourcesPath.string().c_str());
    #endif

    auto chooseTestControl = [&, pGraphics, testRect, resourcePath](int idx) {
      
      IControl* pNewControl = nullptr;
      
      switch (idx) {
        case 0: pNewControl = new TestGradientControl(testRect, kParamDummy); break;
        case 1: pNewControl = new TestColorControl(testRect); break;
        case 2: pNewControl = new TestPolyControl(testRect, kParamDummy); break;
        case 3: pNewControl = new TestArcControl(testRect, kParamDummy); break;
        case 4: pNewControl = new TestBezierControl(testRect); break;
        case 5: pNewControl = new TestMultiPathControl(testRect, kParamDummy); break;
        case 6: pNewControl = new TestTextControl(testRect); break;
        case 7: pNewControl = new TestAnimationControl(testRect); break;
        case 8: pNewControl = new TestDrawContextControl(testRect); break;
        case 9: pNewControl = new TestSVGControl(testRect, pGraphics->LoadSVG(TIGER_FN)); break;
        case 10: pNewControl = new TestImageControl(testRect, pGraphics->LoadBitmap(IPLUG_FN)); break;
        case 11: pNewControl = new TestLayerControl(testRect, kParamDummy); break;
        case 12: pNewControl = new TestBlendControl(testRect, pGraphics->LoadBitmap(SRC_FN), pGraphics->LoadBitmap(DST_FN), kParamDummy); break;
        case 13: pNewControl = new TestDropShadowControl(testRect, pGraphics->LoadSVG(ORBS_FN)); break;
        case 14: pNewControl = new TestCursorControl(testRect); break;
        case 15: pNewControl = new TestKeyboardControl(testRect); break;
        case 16: pNewControl = new TestShadowGradientControl(testRect); break;
        case 17: pNewControl = new TestFontControl(testRect); break;
        case 18: pNewControl = new TestTextOrientationControl(testRect, kParamDummy); break;
        case 19: pNewControl = new TestTextSizeControl(testRect, kParamDummy); break;
        case 20: pNewControl = new TestMPSControl(testRect, pGraphics->LoadBitmap(SMILEY_FN), kParamDummy); break;
        case 21: pNewControl = new TestCustomShaderControl(testRect, kParamDummy); break;
        case 22: pNewControl = new TestGesturesControl(testRect); break;
        case 23: pNewControl = new TestMTControl(testRect); pNewControl->SetWantsMultiTouch(true); break;
        case 24: pNewControl = new TestFlexBoxControl(testRect); break;
        case 25: pNewControl = new TestMaskControl(testRect, pGraphics->LoadBitmap(SMILEY_FN)); break;
        case 26: pNewControl = new TestDirBrowseControl(testRect, "png", resourcePath.Get()); break;
      }
      
      if(pNewControl)
        pGraphics->AttachControl(pNewControl, kCtrlTagTestControl);
      
      SendCurrentParamValuesFromDelegate();
    };
    
    pGraphics->AttachControl(new IVRadioButtonControl(bounds.FracRectHorizontal(0.2f),
                                                      [pGraphics, chooseTestControl](IControl* pCaller) {
                                                        pGraphics->RemoveControlWithTag(kCtrlTagTestControl);
                                                        SplashClickActionFunc(pCaller);
                                                        int selectedTest = pCaller->As<IVRadioButtonControl>()->GetSelectedIdx();
                                                        chooseTestControl(selectedTest);
                                                      },
                                                      testNames
                                                      ));
    
    pGraphics->AttachControl(new IVSliderControl(bounds.FracRectHorizontal(0.2f, true).GetCentredInside(100, 200), kParamDummy, "Value"));

    pGraphics->AttachControl(new GFXLabelControl(bounds.GetFromTRHC(230, 50)));//.GetTranslated(25, -25)));
    
    chooseTestControl(0);
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
