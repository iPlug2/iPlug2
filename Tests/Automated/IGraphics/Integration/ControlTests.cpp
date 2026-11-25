#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "TestEditorDelegate.h"
#include "IControls.h"
using namespace iplug; using namespace igraphics;
TEST_CASE("Knob drag sets linked parameter", "[integration][controls]")
{
  TestEditorDelegate dlg {1, 400, 300, [](IGraphics* pGraphics) {
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->AttachControl(new IVKnobControl(IRECT(100, 100, 200, 200), 0), 1);
  }};
  dlg.GetParam(0)->InitDouble("Gain", 0., 0., 100., 0.01, "%");
  auto* ui = dlg.OpenUI();
  REQUIRE(ui != nullptr);
  REQUIRE(ui->NControls() == 2);
  REQUIRE(ui->RenderFrame());
  REQUIRE(ui->CountOps(IGraphicsHeadless::DrawOp::EType::Fill) > 0);
  REQUIRE_FALSE(ui->RenderFrame());
  ui->Drag(150, 150, 150, 50);
  REQUIRE(dlg.GetParam(0)->Value() > 0.);
  REQUIRE(dlg.mParamEvents.front().mType == TestEditorDelegate::ParamEvent::EType::Begin);
  REQUIRE(dlg.mParamEvents.back().mType == TestEditorDelegate::ParamEvent::EType::End);
  dlg.SendParameterValueFromDelegate(0, 0.25, true);
  REQUIRE(ui->GetControlWithTag(1)->GetValue() == Catch::Approx(0.25));
  REQUIRE(ui->RenderFrame());
}
