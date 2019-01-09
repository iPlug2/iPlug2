#include "IGraphicsStressTest.h"
#include "IPlug_include_in_plug_src.h"

#include "Test/TestControls.h"

enum EParam
{
  kParamDummy = 0,
  kNumParams
};

enum EControlTags
{
  kCtrlTagLabel = 0
};

IGraphicsStressTest::IGraphicsStressTest(IPlugInstanceInfo instanceInfo)
: IPLUG_CTOR(kNumParams, 1, instanceInfo)
{
#if IPLUG_EDITOR
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, 1.);
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    IRECT bounds = pGraphics->GetBounds();
    pGraphics->SetSizeConstraints(0, 100000, 0, 100000);

    if(pGraphics->NControls()) {
      pGraphics->GetBackgroundControl()->SetTargetAndDrawRECTs(bounds);
      return;
    }
    
    pGraphics->AttachCornerResizer(EUIResizerMode::kUIResizerScale, true);
    pGraphics->HandleMouseOver(true);
    pGraphics->EnableTooltips(true);
    
    pGraphics->SetKeyHandlerFunc([&](const IKeyPress& key)
    {
      switch (key.VK) {
        case VK_UP: this->mNumberOfThings++; break;
        case VK_DOWN: this->mNumberOfThings--; break;
        case VK_TAB:
        {
          if(key.S) this->mKindOfThing--;
          else this->mKindOfThing++;
          break;
        }
        default:
          break;
      }
      WDL_String str;
      str.SetFormatted(64, "Number of things = %i", this->mNumberOfThings);
      dynamic_cast<ITextControl*>(this->GetUI()->GetControlWithTag(kCtrlTagLabel))->SetStr(str.Get());
      this->GetUI()->SetAllControlsDirty();
      return true;
    });
    
    pGraphics->HandleMouseOver(false);
    pGraphics->LoadFont(ROBOTTO_FN);
//    ISVG tiger = pGraphics->LoadSVG(TIGER_FN);
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->AttachControl(new ILambdaControl(*this, bounds, [&](ILambdaControl* pCaller, IGraphics& g, IRECT& r)
    {
      
      static IBitmap smiley = g.LoadBitmap(SMILEY_FN);
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

          switch (this->mKindOfThing)
          {
            case 0: g.DrawRect(rc, rr, &rb); break;
            case 1: g.FillRect(rc, rr, &rb); break;
            case 2: g.DrawRoundRect(rc, rr, roundness, &rb); break;
            case 3: g.FillRoundRect(rc, rr, roundness, &rb); break;
            case 4: g.DrawEllipse(rc, rr, &rb); break;
            case 5: g.FillEllipse(rc, rr, &rb); break;
            case 6: g.DrawArc(rc, rr.MW(), rr.MH(), rr.W() > rr.H() ? rr.H() : rr.W(), rrad1, rrad2, &rb,thickness); break;
            case 7: g.FillArc(rc, rr.MW(), rr.MH(), rr.W() > rr.H() ? rr.H() : rr.W(), rrad1, rrad2, &rb); break;
            case 8: g.DrawLine(rc, dir == 0 ? rr.L : rr.R, rr.B, dir == 0 ? rr.R : rr.L, rr.T, &rb,thickness); break;
            case 9: g.DrawDottedLine(rc, dir == 0 ? rr.L : rr.R, rr.B, dir == 0 ? rr.R : rr.L, rr.T, &rb, thickness); break;
            case 10: g.DrawFittedBitmap(smiley, rr, &rb); break;
            default:
              break;
          }
          
          dir = !dir;
        }
//        pCaller->mLayer = g.EndLayer();
      }

//      g.DrawLayer(pCaller->mLayer);

    }, 10000, false, false));
    
    pGraphics->AttachControl(new ITextControl(*this, bounds.GetGridCell(0, 2, 1), "Number of things = 16", IText(100)), kCtrlTagLabel);
    pGraphics->AttachControl(new ITextControl(*this, bounds.GetGridCell(1, 2, 1), "Test 1 of 32", IText(100)));
  };
  
#endif
}
