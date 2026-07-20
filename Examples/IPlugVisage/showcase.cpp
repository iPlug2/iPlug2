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

#include <visage/app.h>
#include <visage/graphics.h>
#include <visage/utils.h>
#include <visage/widgets.h>

using namespace visage::dimension;

VISAGE_THEME_COLOR(BackgroundColor, 0xff33393f);
VISAGE_THEME_COLOR(OverlayBody, 0xff212529);
VISAGE_THEME_COLOR(OverlayBorder, 0x66ffffff);

VISAGE_THEME_VALUE(BloomSize, 25.0f);
VISAGE_THEME_VALUE(BloomIntensity, 3.0f);
VISAGE_THEME_VALUE(BlurSize, 50.0f);
VISAGE_THEME_VALUE(OverlayRounding, 25.0f);

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

  visage::Bounds body = bodyBounds();
  float rounding = bodyRounding();
  canvas.setColor(OverlayBody);
  canvas.roundedRectangle(body.x(), body.y(), body.width(), body.height(), rounding);

  canvas.setColor(OverlayBorder);
  canvas.roundedRectangleBorder(body.x(), body.y(), body.width(), body.height(), rounding, 1.0f);

  on_animate_.callback(overlay_amount);

  if (animation_.isAnimating())
    redraw();
}

visage::Bounds Overlay::bodyBounds() const {
  float x_border = width() / 4.0f;
  float y_border = height() / 4.0f;

  return { x_border, y_border, width() - 2 * x_border, height() - 2 * y_border };
}

float Overlay::bodyRounding() {
  return paletteValue(OverlayRounding);
}

Showcase::Showcase() : palette_color_window_(&palette_), palette_value_window_(&palette_) {
  setIgnoresMouseEvents(true, true);
  setAcceptsKeystrokes(true);

  palette_.initWithDefaults();
  setPalette(&palette_);

  blur_ = std::make_unique<visage::BlurPostEffect>();
  examples_ = std::make_unique<ExamplesFrame>();
  examples_->setPostEffect(blur_.get());
  examples_->onShowOverlay() = [this] { overlay_.setVisible(true); };
  examples_->onToggleDebug() = [this]() { toggleDebug(); };

  examples_->onScreenshot() = [this](const std::string& file_path) {
    visage::ApplicationEditor* parent = findParent<visage::ApplicationEditor>();
    if (parent)
      parent->takeScreenshot().save(file_path);
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
  canvas.setPalette(palette());
  blur_->setBlurSize(canvas.value(BlurSize));
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
  else if (key.keyCode() == visage::KeyCode::Number1 && modifier) {
    palette_color_window_.show(300_px, 800_px);
    return true;
  }

  return false;
}

int runExample() {
  visage::ShaderCompiler compiler;
  compiler.watchShaderFolder(SHADERS_FOLDER);

  visage::ApplicationWindow editor;

  editor.onDraw() = [&editor](visage::Canvas& canvas) {
    canvas.setColor(BackgroundColor);
    canvas.fill(0, 0, editor.width(), editor.height());
  };

  std::unique_ptr<Showcase> showcase = std::make_unique<Showcase>();
  editor.addChild(showcase.get());
  editor.layout().setFlex(true);
  editor.layout().setFlexItemAlignment(visage::Layout::ItemAlignment::Center);
  showcase->layout().setWidth(visage::Dimension::min(1000_px, 100_vw));
  showcase->layout().setHeight(100_vh);

  editor.setTitle("Visage Showcase");
  if (visage::isMobileDevice())
    editor.showMaximized();
  else
    editor.show(100_vmin, 70_vmin);
  editor.runEventLoop();
  return 0;
}
