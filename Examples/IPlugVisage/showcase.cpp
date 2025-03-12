/* Copyright Vital Audio, LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "showcase.h"

#include "embedded/example_fonts.h"
#include "embedded/example_shaders.h"

#include <visage_app/application_window.h>
#include <visage_graphics/post_effects.h>
#include <visage_utils/dimension.h>
#include <visage_widgets/shader_editor.h>

using namespace visage::dimension;

THEME_COLOR(BackgroundColor, 0xff33393f);
THEME_COLOR(OverlayBody, 0xff212529);
THEME_COLOR(OverlayBorder, 0x66ffffff);

THEME_VALUE(BloomSize, 25.0f, ScaledHeight, false);
THEME_VALUE(BloomIntensity, 3.0f, Constant, false);
THEME_VALUE(BlurSize, 50.0f, ScaledHeight, false);
THEME_VALUE(OverlayRounding, 25.0f, ScaledHeight, false);

class DebugInfo : public visage::Frame {
public:
  DebugInfo() { setIgnoresMouseEvents(true, true); }

  void draw(visage::Canvas& canvas) override {
    canvas.setColor(0x88000000);
    canvas.fill(0, 0, width(), height());

    canvas.setColor(0xffffffff);

    std::vector<std::string> info = canvas.debugInfo();
    int line_height = height() / info.size();
    int text_height = line_height * 0.65f;

    const visage::Font font(text_height, resources::fonts::Lato_Regular_ttf);
    for (int i = 0; i < info.size(); ++i)
      canvas.text(info[i], font, visage::Font::kLeft, line_height, line_height * i, width(), line_height);
    redraw();
  }
};

Overlay::Overlay() :
    animation_(visage::Animation<float>::kRegularTime, visage::Animation<float>::kLinear,
               visage::Animation<float>::kLinear) {
  animation_.setAnimationTime(160.0f);
  animation_.setTargetValue(1.0f);
}

void Overlay::resized() { }

void Overlay::draw(visage::Canvas& canvas) {
  float overlay_amount = animation_.update();
  if (!animation_.isTargeting() && overlay_amount == 0.0f)
    setVisible(false);

  visage::Bounds body = getBodyBounds();
  float rounding = getBodyRounding();
  canvas.setPaletteColor(kOverlayBody);
  canvas.roundedRectangle(body.x(), body.y(), body.width(), body.height(), rounding);

  canvas.setPaletteColor(kOverlayBorder);
  canvas.roundedRectangleBorder(body.x(), body.y(), body.width(), body.height(), rounding, 1.0f);

  on_animate_.callback(overlay_amount);

  if (animation_.isAnimating())
    redraw();
}

visage::Bounds Overlay::getBodyBounds() const {
  int x_border = width() / 4;
  int y_border = height() / 4;

  return { x_border, y_border, width() - 2 * x_border, height() - 2 * y_border };
}

float Overlay::getBodyRounding() {
  return paletteValue(kOverlayRounding);
}

Showcase::Showcase() {
  setAcceptsKeystrokes(true);

  palette_.initWithDefaults();
  setPalette(&palette_);

  blur_ = std::make_unique<visage::BlurPostEffect>();
  examples_ = std::make_unique<ExamplesFrame>();
  examples_->setPostEffect(blur_.get());
  examples_->onShowOverlay() = [this] { overlay_.setVisible(true); };
  examples_->onToggleDebug() = [this]() { toggleDebug(); };

  examples_->onScrenshot() = [this](const std::string& file_path) {
    visage::ApplicationEditor* parent = findParent<visage::ApplicationEditor>();
    if (parent)
      parent->takeScreenshot(file_path);
  };

  addChild(examples_.get());
  overlay_zoom_ = std::make_unique<visage::ShaderPostEffect>(resources::shaders::vs_overlay,
                                                             resources::shaders::fs_overlay);
  overlay_.setPostEffect(overlay_zoom_.get());
  addChild(&overlay_, false);
  overlay_.onAnimate() = [this](float overlay_amount) {
    static constexpr float kMaxZoom = 0.075f;
    blur_->setBlurAmount(overlay_amount);
    overlay_zoom_->setUniformValue("u_zoom", kMaxZoom * (1.0f - overlay_amount) + 1.0f);
    overlay_zoom_->setUniformValue("u_alpha", overlay_amount * overlay_amount);
    examples_->redraw();
  };

  debug_info_ = std::make_unique<DebugInfo>();
  addChild(debug_info_.get());
  debug_info_->setOnTop(true);
  debug_info_->setVisible(false);
}

Showcase::~Showcase() = default;

void Showcase::resized() {
  int w = width();
  int h = height();
  int main_width = w;

  debug_info_->setBounds(0, 0, main_width, h);

  examples_->setBounds(0, 0, main_width, h);

  overlay_.setBounds(examples_->bounds());
}

void Showcase::draw(visage::Canvas& canvas) {
  static constexpr float kMaxZoom = 0.075f;
  canvas.setPalette(palette());
  blur_->setBlurSize(canvas.value(kBlurSize));
}

void Showcase::toggleDebug() {
  debug_info_->setVisible(!debug_info_->isVisible());
}

bool Showcase::keyPress(const visage::KeyEvent& key) {
  static constexpr int kPaletteWidth = 200;
  static constexpr int kShaderEditorWidth = 600;

  bool modifier = key.isMainModifier();
  if (key.keyCode() == visage::KeyCode::D && modifier && key.isShiftDown()) {
    toggleDebug();
    return true;
  }
  else if (key.keyCode() == visage::KeyCode::Z && modifier) {
    undo();
    return true;
  }
  else if (key.keyCode() == visage::KeyCode::Y && modifier) {
    redo();
    return true;
  }

  return false;
}
