#include "IPlugVisage.h"
#include "IPlug_include_in_plug_src.h"

#if IPLUG_EDITOR
#include "embedded/IPlugVisageFonts.h"
#include <algorithm>
#include <cstdio>
#endif

IPlugVisage::IPlugVisage(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kGain)->InitDouble("Gain", 100., 0., 100., 0.01, "%");
}

#if IPLUG_DSP
void IPlugVisage::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kGain)->Value() / 100.;
  for (int c = 0; c < NOutChansConnected(); ++c)
    for (int s = 0; s < nFrames; ++s)
      outputs[c][s] = inputs[c][s] * gain;
}
#endif

#if IPLUG_EDITOR
void IPlugVisage::OnDraw(visage::Canvas& canvas)
{
  const float width = GetEditor()->width();
  const float height = GetEditor()->height();
  const float left = width * 0.12f;
  const float trackWidth = width * 0.76f;
  const float y = height * 0.6f;
  const float value = static_cast<float>(GetParam(kGain)->GetNormalized());
  const visage::Font title(28.f, resources::fonts::Roboto_Regular_ttf);
  const visage::Font label(18.f, resources::fonts::Roboto_Regular_ttf);

  canvas.setColor(0xff19232d);
  canvas.fill(0, 0, width, height);
  canvas.setColor(0xfff1f5f9);
  canvas.text("Visage / iPlug2", title, visage::Font::kCenter, 0, height * 0.12f, width, 40);
  char text[32];
  std::snprintf(text, sizeof(text), "Gain  %.1f%%", GetParam(kGain)->Value());
  canvas.text(text, label, visage::Font::kCenter, 0, height * 0.36f, width, 30);
  canvas.setColor(0xff354452);
  canvas.roundedRectangle(left, y - 4, trackWidth, 8, 4);
  canvas.setColor(0xff65d9bf);
  canvas.roundedRectangle(left, y - 4, trackWidth * value, 8, 4);
  canvas.circle(left + trackWidth * value - 12, y - 12, 24);
  canvas.setColor(0xff9cacba);
  canvas.text("Drag to adjust gain", label, visage::Font::kCenter, 0, height * 0.78f, width, 30);
}

void IPlugVisage::SetGainFromMouse(float x)
{
  const float width = GetEditor()->width();
  if (width <= 0.f)
    return;
  const double value = std::clamp((x - width * 0.12f) / (width * 0.76f), 0.f, 1.f);
  SendParameterValueFromUI(kGain, value);
}

void IPlugVisage::OnMouseDown(const visage::MouseEvent& e)
{
  const float width = GetEditor()->width();
  const float y = GetEditor()->height() * 0.6f;
  if (!e.isLeftButton() || mDragging || e.position.x < width * 0.12f - 12 ||
      e.position.x > width * 0.88f + 12 || e.position.y < y - 20 || e.position.y > y + 20)
    return;

  mDragging = true;
  BeginInformHostOfParamChangeFromUI(kGain);
  SetGainFromMouse(e.position.x);
}

void IPlugVisage::OnMouseDrag(const visage::MouseEvent& e)
{
  if (mDragging)
    SetGainFromMouse(e.position.x);
}

void IPlugVisage::OnMouseUp(const visage::MouseEvent& e)
{
  if (mDragging && e.isLeftButton())
  {
    mDragging = false;
    EndInformHostOfParamChangeFromUI(kGain);
  }
}

void IPlugVisage::OnUIClose()
{
  // Balance the host gesture even when the editor closes during a drag.
  if (mDragging)
  {
    mDragging = false;
    EndInformHostOfParamChangeFromUI(kGain);
  }
}

void IPlugVisage::OnParamChangeUI(int paramIdx, EParamSource source)
{
  if (paramIdx == kGain)
    Redraw();
}
#endif
