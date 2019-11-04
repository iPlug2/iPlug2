#include "IGraphicsStressTest.h"
#include "IPlug_include_in_plug_src.h"

#include "IControls.h"

IGraphicsStressTest::IGraphicsStressTest(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, 1))
{
  GetParam(0)->InitGain("Dummy");
  
#if IPLUG_EDITOR
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, 1.);
  };
#endif
}

#if IPLUG_EDITOR
void IGraphicsStressTest::LayoutUI(IGraphics* pGraphics)
{
  IRECT bounds = pGraphics->GetBounds();
  
  if(pGraphics->NControls()) {
    pGraphics->GetBackgroundControl()->SetTargetAndDrawRECTs(bounds);
    pGraphics->GetControl(1)->SetTargetAndDrawRECTs(bounds);
    pGraphics->GetControlWithTag(kCtrlTagNumThings)->SetTargetAndDrawRECTs(bounds.GetGridCell(0, 2, 1));
    pGraphics->GetControlWithTag(kCtrlTagTestNum)->SetTargetAndDrawRECTs(bounds.GetGridCell(1, 2, 1));
    
    auto bottomButtons = bounds.GetFromBRHC(400, 50).GetPadded(-10.);
    for(int button=0;button<5;button++)
      pGraphics->GetControlWithTag(kCtrlTagButton1 + button)->SetTargetAndDrawRECTs(bottomButtons.GetGridCell(button, 1, 5));
    
    return;
  }
  
  pGraphics->SetSizeConstraints(100, 100000, 100, 100000);
  pGraphics->ShowFPSDisplay(true);
  pGraphics->AttachCornerResizer(EUIResizerMode::Size, true);
  
  enum class EFunc {Next, Prev, More, Less, Set};
  
  auto DoFunc = [&](EFunc func, int thing = 0){
    switch (func) {
      case EFunc::Next: mKindOfThing++; break;
      case EFunc::Prev: mKindOfThing--; break;
      case EFunc::More: mNumberOfThings++; break;
      case EFunc::Less: mNumberOfThings--; break;
      case EFunc::Set: mKindOfThing = thing; break;
      default:
        break;
    }
    
    dynamic_cast<ITextControl*>(GetUI()->GetControlWithTag(kCtrlTagNumThings))->SetStrFmt(64, "Number of things = %i", mNumberOfThings);
    dynamic_cast<ITextControl*>(GetUI()->GetControlWithTag(kCtrlTagTestNum))->SetStrFmt(64, "Test %i/%i", mKindOfThing, 32);
    GetUI()->SetAllControlsDirty();
  };
  
  pGraphics->SetKeyHandlerFunc([DoFunc](const IKeyPress& key, bool isUp)
  {
    if(!isUp) {
      switch (key.VK) {
        case kVK_UP: DoFunc(EFunc::More); return true;
        case kVK_DOWN: DoFunc(EFunc::Less); return true;
        case kVK_TAB: key.S ? DoFunc(EFunc::Prev) : DoFunc(EFunc::Next); return true;
        default: return false;
      }
    }
    return false;
  });
  
  pGraphics->HandleMouseOver(false);
  pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
  pGraphics->AttachPanelBackground(COLOR_GRAY);
  pGraphics->AttachControl(new ILambdaControl(bounds, [&](ILambdaControl* pCaller, IGraphics& g, IRECT& r) {
    static IBitmap smiley = g.LoadBitmap(SMILEY_FN);
    static ISVG tiger = g.LoadSVG(TIGER_FN);
    
    if(mKindOfThing == 0)
      g.DrawText(IText(40), "Press tab to go to next test, up/down to change the # of things", r);
    
    //      if (!g.CheckLayer(pCaller->mLayer))
    {
      //        g.StartLayer(r);
      
      for (int i=0; i<this->mNumberOfThings; i++)
      {
        IRECT rr = r.GetRandomSubRect();
        IColor rc = IColor::GetRandomColor();
        IBlend rb = {};
        static bool dir = 0;
        static float thickness = 5.f;
        static float roundness = 5.f;
        float rrad1 = rand() % 360;
        float rrad2 = rand() % 360;
        
        switch (mKindOfThing)
        {
          case 1:  g.DrawRect(rc, rr, &rb); break;
          case 2:  g.FillRect(rc, rr, &rb); break;
          case 3:  g.DrawRoundRect(rc, rr, roundness, &rb); break;
          case 4:  g.FillRoundRect(rc, rr, roundness, &rb); break;
          case 5:  g.DrawEllipse(rc, rr, &rb); break;
          case 6:  g.FillEllipse(rc, rr, &rb); break;
          case 7:  g.DrawArc(rc, rr.MW(), rr.MH(), rr.W() > rr.H() ? rr.H() : rr.W(), rrad1, rrad2, &rb,thickness); break;
          case 8:  g.FillArc(rc, rr.MW(), rr.MH(), rr.W() > rr.H() ? rr.H() : rr.W(), rrad1, rrad2, &rb); break;
          case 9:  g.DrawLine(rc, dir == 0 ? rr.L : rr.R, rr.B, dir == 0 ? rr.R : rr.L, rr.T, &rb,thickness); break;
          case 10: g.DrawDottedLine(rc, dir == 0 ? rr.L : rr.R, rr.B, dir == 0 ? rr.R : rr.L, rr.T, &rb, thickness); break;
          case 11: g.DrawFittedBitmap(smiley, rr, &rb); break;
          case 12: g.DrawSVG(tiger, rr); break;
          default:
            break;
        }
        
        dir = !dir;
      }
      //        pCaller->mLayer = g.EndLayer();
    }
    
    //      g.DrawLayer(pCaller->mLayer);
    
  }, 10000, false, false));
  
  pGraphics->AttachControl(new ITextControl(bounds.GetGridCell(0, 2, 1), "", IText(100)), kCtrlTagNumThings);
  pGraphics->AttachControl(new ITextControl(bounds.GetGridCell(1, 2, 1), "", IText(100)), kCtrlTagTestNum);
  
  auto bottomButtons = bounds.GetFromBRHC(400, 50).GetPadded(-10.);
  int button = 0;
  for (auto buttonLabel : {"Select test", "Next test", "Previous test", "NumThings++", "NumThings--"}) {
    pGraphics->AttachControl(new IVButtonControl(bottomButtons.GetGridCell(button, 1, 5), [button, DoFunc, pGraphics](IControl* pCaller){
      SplashClickActionFunc(pCaller);
      
      switch (button) {
        case 0:
        {
          static IPopupMenu menu {"Test", {"DrawRect", "FillRect", "DrawRoundRect", "FillRoundRect", "DrawEllipse", "FillEllipse", "DrawArc", "FillArc", "DrawLine", "DrawDottedLine", "DrawFittedBitmap", "DrawSVG"},
            [DoFunc](int indexInMenu, IPopupMenu::Item* itemChosen) {
              DoFunc(EFunc::Set, indexInMenu);
            }};
          
          pGraphics->CreatePopupMenu(*pCaller, menu, pCaller->GetRECT());
        }
        case 1: DoFunc(EFunc::Next); break;
        case 2: DoFunc(EFunc::Prev); break;
        case 3: DoFunc(EFunc::More); break;
        case 4: DoFunc(EFunc::Less); break;
        default:
          break;
      }
    }, buttonLabel, DEFAULT_STYLE.WithLabelText(DEFAULT_TEXT.WithVAlign(EVAlign::Middle))), kCtrlTagButton1 + button);
    
    button++;
  }

}
#endif
