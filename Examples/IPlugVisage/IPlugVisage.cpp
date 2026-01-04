#include "IPlugVisage.h"
#include "IPlug_include_in_plug_src.h"
#include <visage/windowing.h>
#include <visage_utils/dimension.h>
#include <filesystem>
#include <string>

VISAGE_THEME_COLOR(BackgroundColor, 0xffff0000);

std::string getShaderFolder()
{
  namespace fs = std::filesystem;
  fs::path filePath = __FILE__;
  fs::path folderPath = filePath.parent_path();
  
  return (folderPath / "visage/examples/shaders").string();
}

IPlugVisage::IPlugVisage(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kGain)->InitDouble("Gain", 0., 0., 100.0, 0.01, "%");
}

void IPlugVisage::OnIdle()
{
}

void* IPlugVisage::OpenWindow(void* pParent)
{
  mEditor = std::make_unique<visage::ApplicationEditor>();
  visage::IBounds bounds = visage::computeWindowBounds(80, 60);
  mEditor->setBounds(0, 0, bounds.width(), bounds.height());
  
  visage::ShaderCompiler compiler;
  compiler.watchShaderFolder(getShaderFolder());

  mEditor->onDraw() = [&](visage::Canvas& canvas) {
    canvas.setColor(BackgroundColor);
    canvas.fill(0, 0, mEditor->width(), mEditor->height());
  };

  using namespace visage::dimension;
  mShowcase = std::make_unique<Showcase>();
  mEditor->addChild( mShowcase.get());
  mEditor->layout().setFlex(true);
  mEditor->layout().setFlexItemAlignment(visage::Layout::ItemAlignment::Center);
  mShowcase->layout().setWidth(visage::Dimension::min(1000_px, 100_vw));
  mShowcase->layout().setHeight(100_vh);

  mWindow = visage::createPluginWindow(mEditor->width(), mEditor->height(), pParent);
  mWindow->setFixedAspectRatio(mEditor->isFixedAspectRatio());
  mEditor->addToWindow(mWindow.get());
  mWindow->show();
  
  OnUIOpen();
  return mWindow.get()->nativeHandle();
}

void IPlugVisage::OnParentWindowResize(int width, int height)
{
  if (mWindow)
  {
    mWindow->setWindowSize(width, height);
    mEditor->setDimensions(width, height);
  }
}

void IPlugVisage::CloseWindow()
{
  mEditor->removeFromWindow();
  mEditor->removeAllChildren();
  mWindow.reset();
  
  IEditorDelegate::CloseWindow();
}

void IPlugVisage::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kGain)->Value() / 100.;
  const int nChans = NOutChansConnected();
  
  for (int s = 0; s < nFrames; s++) {
    for (int c = 0; c < nChans; c++) {
      outputs[c][s] = inputs[c][s] * gain;
    }
  }
}
