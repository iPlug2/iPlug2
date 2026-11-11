#include "IPlugVisage.h"
#include "IPlug_include_in_plug_src.h"
#include <visage/windowing.h>
#include <visage_utils/dimension.h>

IPlugVisage::IPlugVisage(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kGain)->InitDouble("Gain", 0., 0., 100.0, 0.01, "%");
}

IPlugVisage::~IPlugVisage()
{
  CloseWindow();
}

void* IPlugVisage::OpenWindow(void* pParent)
{
  CloseWindow();
  mEditor = std::make_unique<visage::ApplicationEditor>();
  mEditor->setBounds(0, 0, GetEditorWidth(), GetEditorHeight());

  mEditor->onDraw() = [this](visage::Canvas& canvas) {
    canvas.setColor(0xff33393f);
    canvas.fill(0, 0, mEditor->width(), mEditor->height());
  };

  using namespace visage::dimension;
  mShowcase = std::make_unique<Showcase>();
  mEditor->addChild(mShowcase.get());
  mEditor->layout().setFlex(true);
  mEditor->layout().setFlexItemAlignment(visage::Layout::ItemAlignment::Center);
  mShowcase->layout().setWidth(visage::Dimension::min(1000_px, 100_vw));
  mShowcase->layout().setHeight(100_vh);

#ifdef OS_WIN
  // Windows hosts pass physical pixels; do not apply the display scale twice.
  mWindow = visage::createPluginWindow(visage::Dimension::nativePixels(GetEditorWidth()),
                                      visage::Dimension::nativePixels(GetEditorHeight()), pParent);
#else
  mWindow = visage::createPluginWindow(GetEditorWidth(), GetEditorHeight(), pParent);
#endif
  mWindow->setFixedAspectRatio(mEditor->isFixedAspectRatio());
  mEditor->addToWindow(mWindow.get());
  mWindow->show();
  
  OnUIOpen();
  return mWindow->nativeHandle();
}

void IPlugVisage::OnParentWindowResize(int width, int height)
{
  SetEditorSize(width, height);
  if (mWindow)
  {
#ifdef OS_MAC
    // macOS hosts pass points, Windows hosts pass physical pixels.
    mWindow->setWindowSize(width, height);
#else
    mWindow->setNativeWindowSize(width, height);
#endif
  }
}

void IPlugVisage::CloseWindow()
{
  if (!mEditor)
    return;

  mWindow->hide();
  mEditor->removeFromWindow();
  mEditor->removeAllChildren();
  mShowcase.reset();
  mEditor.reset();
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
