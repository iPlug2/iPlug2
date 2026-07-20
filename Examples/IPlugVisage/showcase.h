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

#pragma once

#include "examples_frame.h"

#include <visage/app.h>
#include <visage_graphics/palette.h>
#include <visage_ui/popup_menu.h>
#include <visage_ui/undo_history.h>
#include <visage_utils/dimension.h>
#include <visage_widgets/palette_editor.h>
#include <visage_widgets/shader_editor.h>

class DebugInfo;

class Overlay : public visage::Frame {
public:
  Overlay();

  void resized() override;
  void draw(visage::Canvas& canvas) override;
  visage::Bounds bodyBounds() const;
  float bodyRounding();

  void mouseDown(const visage::MouseEvent& e) override {
    animation_.target(false);
    redraw();
  }
  void visibilityChanged() override { animation_.target(isVisible()); }
  auto& onAnimate() { return on_animate_; }

private:
  visage::Animation<float> animation_;
  visage::CallbackList<void(float)> on_animate_;
};

class PaletteColorWindow : public visage::ApplicationWindow {
public:
  PaletteColorWindow(visage::Palette* palette) : editor_(palette) {
    addChild(&editor_);
    editor_.layout().setMargin(0);
  }

private:
  visage::PaletteColorEditor editor_;
};

class PaletteValueWindow : public visage::ApplicationWindow {
public:
  PaletteValueWindow(visage::Palette* palette) : editor_(palette) {
    addChild(&editor_);
    editor_.layout().setMargin(0);
  }

private:
  visage::PaletteValueEditor editor_;
};

class Showcase : public visage::Frame,
                 public visage::UndoHistory {
public:
  Showcase();
  ~Showcase() override;

  void resized() override;
  void draw(visage::Canvas& canvas) override;

  void toggleDebug();
  bool keyPress(const visage::KeyEvent& key) override;

private:
  std::unique_ptr<visage::BlurPostEffect> blur_;
  std::unique_ptr<visage::ShaderPostEffect> overlay_zoom_;
  std::unique_ptr<ExamplesFrame> examples_;
  std::unique_ptr<DebugInfo> debug_info_;

  visage::Palette palette_;
  PaletteColorWindow palette_color_window_;
  PaletteValueWindow palette_value_window_;
  Overlay overlay_;

  VISAGE_LEAK_CHECKER(Showcase)
};
