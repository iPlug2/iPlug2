/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Tests the parts of IEditorDelegate/IPlugAPIBase that deal with the editor window, without a UI framework attached.

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "TestPlugins.h"

using namespace iplug;
using Catch::Approx;

namespace
{

/** A plug-in with a 400x300 editor which may be resized between 200x150 and 800x600 */
class EditorPlug : public GainPlug
{
public:
  static TestConfig MakeConfig()
  {
    TestConfig config = GainPlug::MakeConfig();
    config.hasUI = true;
    config.width = 400;
    config.height = 300;
    config.minWidth = 200;
    config.maxWidth = 800;
    config.minHeight = 150;
    config.maxHeight = 600;
    config.hostResize = true;
    return config;
  }

  EditorPlug()
  : GainPlug(InstanceInfo{}, MakeConfig())
  {
  }
};

} // namespace

TEST_CASE("Editor: SetEditorSize() updates the size getters", "[integration][editor]")
{
  EditorPlug plug;

  REQUIRE(plug.GetEditorWidth() == 400);
  REQUIRE(plug.GetEditorHeight() == 300);

  plug.SetEditorSize(640, 480);

  CHECK(plug.GetEditorWidth() == 640);
  CHECK(plug.GetEditorHeight() == 480);

  // It records the size, it doesn't ask the host for anything
  CHECK(plug.mNumEditorResizeRequests == 0);
}

TEST_CASE("Editor: size constraints", "[integration][editor]")
{
  EditorPlug plug;

  SECTION("from the config")
  {
    CHECK(plug.GetMinWidth() == 200);
    CHECK(plug.GetMaxWidth() == 800);
    CHECK(plug.GetMinHeight() == 150);
    CHECK(plug.GetMaxHeight() == 600);
  }

  SECTION("can be changed")
  {
    plug.SetSizeConstraints(10, 20, 30, 40);
    CHECK(plug.GetMinWidth() == 10);
    CHECK(plug.GetMaxWidth() == 20);
    CHECK(plug.GetMinHeight() == 30);
    CHECK(plug.GetMaxHeight() == 40);
  }

  SECTION("low and high are sorted if given the wrong way round")
  {
    plug.SetSizeConstraints(900, 300, 700, 100);
    CHECK(plug.GetMinWidth() == 300);
    CHECK(plug.GetMaxWidth() == 900);
    CHECK(plug.GetMinHeight() == 100);
    CHECK(plug.GetMaxHeight() == 700);
  }

  SECTION("changing the constraints does not resize the editor")
  {
    plug.SetSizeConstraints(10, 20, 30, 40);
    CHECK(plug.GetEditorWidth() == 400);
    CHECK(plug.GetEditorHeight() == 300);
  }
}

TEST_CASE("Editor: ConstrainEditorResize() clips a requested size to the constraints", "[integration][editor]")
{
  EditorPlug plug;

  struct ConstrainCase
  {
    int mW, mH;         // requested
    bool mExpectedOK;
    int mExpectedW, mExpectedH;
  };

  const ConstrainCase cases[] = {
    {400, 300, true, 400, 300},    // well inside
    {200, 150, true, 200, 150},    // the minimum is allowed
    {800, 600, true, 800, 600},    // the maximum is allowed
    {199, 300, false, 200, 300},   // just too narrow
    {801, 300, false, 800, 300},   // just too wide
    {400, 149, false, 400, 150},   // just too short
    {400, 601, false, 400, 600},   // just too tall
    {0, 0, false, 200, 150},       // both too small
    {5000, 5000, false, 800, 600}, // both too big
    {100, 5000, false, 200, 600},  // one of each
    {-50, 300, false, 200, 300},   // negative
  };

  for (const ConstrainCase& constrainCase : cases)
  {
    INFO("requested " << constrainCase.mW << "x" << constrainCase.mH);

    int w = constrainCase.mW;
    int h = constrainCase.mH;

    CHECK(plug.ConstrainEditorResize(w, h) == constrainCase.mExpectedOK);
    CHECK(w == constrainCase.mExpectedW);
    CHECK(h == constrainCase.mExpectedH);
  }

  // Querying doesn't resize anything
  CHECK(plug.GetEditorWidth() == 400);
  CHECK(plug.GetEditorHeight() == 300);
}

TEST_CASE("Editor: a resize requested by the UI goes through the host", "[integration][editor]")
{
  EditorPlug plug;

  SECTION("the host accepts")
  {
    plug.mHostAcceptsResize = true;

    CHECK(plug.EditorResizeFromUI(600, 450, true));
    CHECK(plug.mNumEditorResizeRequests == 1);
    CHECK(plug.GetEditorWidth() == 600);
    CHECK(plug.GetEditorHeight() == 450);
  }

  SECTION("the host refuses")
  {
    plug.mHostAcceptsResize = false;

    CHECK_FALSE(plug.EditorResizeFromUI(600, 450, true));
    CHECK(plug.mNumEditorResizeRequests == 1);
    CHECK(plug.GetEditorWidth() == 400);
    CHECK(plug.GetEditorHeight() == 300);
  }

  SECTION("the host changes its mind")
  {
    plug.mHostAcceptsResize = false;
    CHECK_FALSE(plug.EditorResizeFromUI(600, 450, true));

    plug.mHostAcceptsResize = true;
    CHECK(plug.EditorResizeFromUI(500, 350, true));

    CHECK(plug.mNumEditorResizeRequests == 2);
    CHECK(plug.GetEditorWidth() == 500);
    CHECK(plug.GetEditorHeight() == 350);
  }

  SECTION("no platform resize needed: the host is not asked")
  {
    plug.mHostAcceptsResize = false;

    CHECK(plug.EditorResizeFromUI(600, 450, false));
    CHECK(plug.mNumEditorResizeRequests == 0);
  }
}

TEST_CASE("Editor: opening the window sends the current parameter values to the UI", "[integration][editor]")
{
  EditorPlug plug;
  plug.GetParam(GainPlug::kGain)->Set(50.);
  plug.GetParam(GainPlug::kMode)->Set(2.);

  CHECK(plug.mNumUIOpenCalls == 0);
  CHECK(plug.OpenWindow(nullptr) == nullptr); // a plug-in without a UI framework has no view to return

  CHECK(plug.mNumUIOpenCalls == 1);
  CHECK(plug.mNumUICloseCalls == 0);

  REQUIRE(plug.mParamValuesToUI.size() == 3);

  const double expected[] = {0.25, 0., 1.};

  for (int i = 0; i < 3; i++)
  {
    const auto& toUI = plug.mParamValuesToUI[static_cast<size_t>(i)];
    CHECK(toUI.mIdx == i);
    CHECK(toUI.mNormalized);
    CHECK(toUI.mValue == Approx(expected[i]));
  }

  // The DSP and the host have nothing to do with it
  CHECK(plug.mParamChanges.empty());
  CHECK(plug.mParamEvents.empty());

  SECTION("closing the window")
  {
    plug.CloseWindow();
    CHECK(plug.mNumUICloseCalls == 1);
    CHECK(plug.mNumUIOpenCalls == 1);
  }

  SECTION("the window can be reopened, and gets the values as they are then")
  {
    plug.CloseWindow();
    plug.HostSetParameterNormalized(GainPlug::kGain, 1.);
    plug.ClearAll();

    plug.OpenWindow(nullptr);

    CHECK(plug.mNumUIOpenCalls == 1);
    REQUIRE(plug.mParamValuesToUI.size() == 3);
    CHECK(plug.mParamValuesToUI[0].mValue == Approx(1.));
  }
}

TEST_CASE("Editor: the plug-in keeps working while the window is open", "[integration][editor]")
{
  EditorPlug plug;
  plug.HostActivate(48000., 64);
  plug.OpenWindow(nullptr);
  plug.ClearAll();

  // The user drags a control...
  plug.BeginInformHostOfParamChangeFromUI(GainPlug::kGain);
  plug.SendParameterValueFromUI(GainPlug::kGain, 0.5);
  plug.EndInformHostOfParamChangeFromUI(GainPlug::kGain);

  // ...and the host automates another
  plug.HostSetParameterNormalized(GainPlug::kMute, 1.);
  plug.HostIdle();

  CHECK(plug.mParamEvents.size() == 3);
  CHECK(plug.GetParam(GainPlug::kGain)->Value() == Approx(100.));
  CHECK(plug.GetParam(GainPlug::kMute)->Bool());

  // Only the host's change is sent back to the UI
  REQUIRE(plug.mParamValuesToUI.size() == 1);
  CHECK(plug.mParamValuesToUI[0].mIdx == GainPlug::kMute);

  plug.CloseWindow();
  plug.HostIdle();
  CHECK(plug.mNumUICloseCalls == 1);
}

TEST_CASE("Editor: default implementations of the optional hooks", "[integration][editor]")
{
  EditorPlug plug;

  const IKeyPress key {"a", kVK_A};
  CHECK_FALSE(plug.OnKeyDown(key));
  CHECK_FALSE(plug.OnKeyUp(key));

  plug.OnParentWindowResize(1000, 1000);
  plug.SetScreenScale(2.f);

  CHECK(plug.GetEditorWidth() == 400);
  CHECK(plug.GetEditorHeight() == 300);
  CHECK(plug.GetAAXViewInterface() == nullptr);
}
