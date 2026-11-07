#include "IPlugShaderUI.h"
#include "IPlug_include_in_plug_src.h"

#if IPLUG_EDITOR
#include "ShaderControl.h"
#endif

#include <filesystem>

enum EParam
{
  kParamDummy = 0,
  kNumParams
};

IPlugShaderUI::IPlugShaderUI(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, 1))
{
  GetParam(kParamDummy)->InitPercentage("Dummy", 50.f);
  
#if IPLUG_EDITOR
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    IRECT bounds = pGraphics->GetBounds();

    if (pGraphics->NControls())
    {
      pGraphics->GetBackgroundControl()->SetRECT(bounds);
      pGraphics->GetControl(1)->SetTargetAndDrawRECTs(bounds.SubRectHorizontal(3, 0));
      pGraphics->GetControl(2)->SetTargetAndDrawRECTs(bounds.SubRectHorizontal(3, 1));
      pGraphics->GetControl(3)->SetTargetAndDrawRECTs(bounds.SubRectHorizontal(3, 2));      
      return;
    }

    pGraphics->EnableMouseOver(true);
    pGraphics->EnableTooltips(true);
    pGraphics->EnableMultiTouch(true);
    pGraphics->SetLayoutOnResize(true);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    
    pGraphics->AttachPanelBackground(COLOR_WHITE);
    pGraphics->AttachControl(new ShaderControl(bounds.SubRectHorizontal(3, 0), kParamDummy, COLOR_RED));
    pGraphics->AttachControl(new ShaderControl(bounds.SubRectHorizontal(3, 1), kParamDummy, COLOR_GREEN));
    pGraphics->AttachControl(new ShaderControl(bounds.SubRectHorizontal(3, 2), kParamDummy, COLOR_BLUE));
  };
  
#endif
}

void IPlugShaderUI::OnHostSelectedViewConfiguration(int width, int height)
{
  DBGMSG("SELECTED: W %i, H%i\n", width, height);
//  const float scale = (float) height / (float) PLUG_HEIGHT;
  
//  if(GetUI())
//    GetUI()->Resize(width, height, 1);
}

bool IPlugShaderUI::OnHostRequestingSupportedViewConfiguration(int width, int height)
{
  DBGMSG("SUPPORTED: W %i, H%i\n", width, height); return true;
}

void IPlugShaderUI::OnParentWindowResize(int width, int height)
{
  if (GetUI())
    GetUI()->Resize(width, height, 1.f, false);
}
