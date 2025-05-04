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

IGraphicsTest::IGraphicsTest(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, 1))
{
  GetParam(kParamDummy)->InitPercentage("Dummy", 100.f);
  
#if IPLUG_EDITOR
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS);
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    IRECT bounds = pGraphics->GetBounds();
    IRECT innerBounds = bounds.GetPadded(-20.f);

    if (pGraphics->NControls())
    {
      pGraphics->GetBackgroundControl()->SetTargetAndDrawRECTs(bounds);
      pGraphics->GetControl(1)->SetTargetAndDrawRECTs(innerBounds);
      return;
    }

    pGraphics->EnableMouseOver(true);
    pGraphics->EnableTooltips(true);
    pGraphics->EnableMultiTouch(true);
    pGraphics->SetLayoutOnResize(true);
        
    pGraphics->SetKeyHandlerFunc([&](const IKeyPress& key, bool isUp) {
      if (!isUp) {
        switch (key.VK) {
          case kVK_TAB:
            // TODO: tab through pages
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
    pGraphics->AttachPanelBackground(COLOR_GRAY);

    WDL_String resourcePath;
    #ifdef OS_MAC
    BundleResourcePath(resourcePath, BUNDLE_ID);
    #else
    namespace fs = std::filesystem;
    fs::path mainPath(__FILE__);
    fs::path imgResourcesPath = mainPath.parent_path() / "Resources" / "img";
    resourcePath.Set(imgResourcesPath.string().c_str());
    #endif

#define ADD_TEST_PAGE(name, control) {name, \
    new IVTabPage([&](IVTabPage* pPage, const IRECT& r) { \
                      pPage->AddChildControl(control); \
    }, \
    [&](IContainerBase* pTab, const IRECT& r) { \
      if (pTab->NChildren() == 1) { \
        const auto dim = r.W() < r. H() ? r.W() : r.H(); \
        const auto controlBounds = r.GetCentredInside(dim).GetPadded(-10); \
        pTab->GetChild(0)->SetTargetAndDrawRECTs(controlBounds); \
      } \
    } \
    )}

    PageMap pages = {
      ADD_TEST_PAGE("Gradient", new TestGradientControl(r, kParamDummy)),
      ADD_TEST_PAGE("Multi-stop gradient", new TestColorControl(r)),
      ADD_TEST_PAGE("Polygon", new TestPolyControl(r, kParamDummy)),
      ADD_TEST_PAGE("Arcs", new TestArcControl(r, kParamDummy)),
      ADD_TEST_PAGE("Beziers", new TestBezierControl(r)),
      ADD_TEST_PAGE("MultiPath", new TestMultiPathControl(r, kParamDummy)),
      ADD_TEST_PAGE("Text", new TestTextControl(r)),
      ADD_TEST_PAGE("Animation", new TestAnimationControl(r)),
      ADD_TEST_PAGE("Draw contexts", new TestDrawContextControl(r)),
      ADD_TEST_PAGE("SVG", new TestSVGControl(r, pGraphics->LoadSVG(TIGER_FN))),
      ADD_TEST_PAGE("Image", new TestImageControl(r, pGraphics->LoadBitmap(IPLUG_FN))),
      ADD_TEST_PAGE("Layer", new TestLayerControl(r, kParamDummy)),
      ADD_TEST_PAGE("Blend modes", new TestBlendControl(r, pGraphics->LoadBitmap(SRC_FN), pGraphics->LoadBitmap(DST_FN), kParamDummy)),
      ADD_TEST_PAGE("DropShadow", new TestDropShadowControl(r, pGraphics->LoadSVG(ORBS_FN))),
      ADD_TEST_PAGE("Cursor", new TestCursorControl(r)),
      ADD_TEST_PAGE("Keyboard", new TestKeyboardControl(r)),
      ADD_TEST_PAGE("ShadowGradient", new TestShadowGradientControl(r)),
      ADD_TEST_PAGE("Font", new TestFontControl(r)),
      ADD_TEST_PAGE("TextOrientation", new TestTextOrientationControl(r, kParamDummy)),
      ADD_TEST_PAGE("TextSize", new TestTextSizeControl(r, kParamDummy)),
      ADD_TEST_PAGE("MPS", new TestMPSControl(r, pGraphics->LoadBitmap(SMILEY_FN), kParamDummy)),
      // TODO: fix crash
//      ADD_TEST_PAGE("Custom Shader", new TestCustomShaderControl(r, kParamDummy)),
      ADD_TEST_PAGE("Gesture Recognizers", new TestGesturesControl(r)),
      {"MultiTouch", new IVTabPage([&](IVTabPage* pPage, const IRECT& r) {
        auto* pControl = new TestMTControl(r);
        pControl->SetWantsMultiTouch(true);
        pPage->AddChildControl(pControl);
      })},
      ADD_TEST_PAGE("FlexBox", new TestFlexBoxControl(r)),
      ADD_TEST_PAGE("Mask", new TestMaskControl(r, pGraphics->LoadBitmap(SMILEY_FN))),
      ADD_TEST_PAGE("DirBrowse", new TestDirBrowseControl(r, "png", resourcePath.Get())),
      ADD_TEST_PAGE("DragNDrop", new TestDragAndDropControl(r))
    };

    #undef ADD_TEST_PAGE

    pGraphics->AttachControl(new IVTabbedPagesControl(innerBounds, pages, "Test Controls", DEFAULT_STYLE.WithShowLabel(false).WithValueText(DEFAULT_LABEL_TEXT), 50.0f, 1.f, EAlign::Near, EVAlign::Bottom));
  };
  
#endif
}

