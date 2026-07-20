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

#include <visage_widgets/bar_list.h>
#include <visage_widgets/button.h>
#include <visage_widgets/graph_line.h>
#include <visage_widgets/shader_quad.h>
#include <visage_widgets/text_editor.h>

class DragDropExample;
class AnimatedLines;

class TitleBar : public visage::Frame {
public:
  TitleBar(const std::string& title) : title_(title) { setIgnoresMouseEvents(true, true); }

  void draw(visage::Canvas& canvas) override;

private:
  std::string title_;
};

class ExampleSection : public visage::Frame {
public:
  ExampleSection(const std::string& title, visage::Frame* example);

private:
  std::string title_;
  TitleBar title_bar_;
  visage::Frame* example_;
};

class ExamplesFrame : public visage::ScrollableFrame {
public:
  static constexpr int kNumLines = 2;
  static constexpr int kNumBars = 21;

  ExamplesFrame();
  ~ExamplesFrame() override;

  void resized() override;
  void setupBars();
  void setupButtons();
  void setupShapes();
  void setupTextEditors();

  auto& onScreenshot() { return on_screenshot_; }
  auto& onShowOverlay() { return on_show_overlay_; }
  auto& onToggleDebug() { return on_toggle_debug_; }

private:
  std::vector<std::unique_ptr<ExampleSection>> sections_;

  visage::Frame button_container_;
  visage::Frame text_editor_container_;
  visage::Frame shader_container_;
  visage::Frame image_container_;

  visage::CallbackList<void(const std::string& file_path)> on_screenshot_;
  visage::CallbackList<void()> on_show_overlay_;
  visage::CallbackList<void()> on_toggle_debug_;

  std::unique_ptr<DragDropExample> drag_drop_;
  std::unique_ptr<visage::BarList> bar_list_;
  std::unique_ptr<visage::ShaderQuad> shader_quad_;
  std::unique_ptr<visage::ToggleIconButton> icon_button_;
  std::unique_ptr<visage::ToggleTextButton> text_button_;
  std::unique_ptr<visage::UiButton> ui_button_;
  std::unique_ptr<visage::UiButton> action_button_;
  std::unique_ptr<visage::TextEditor> text_editor_;
  std::unique_ptr<visage::TextEditor> left_text_editor_;
  std::unique_ptr<visage::TextEditor> center_editor_;
  std::unique_ptr<visage::TextEditor> right_text_editor_;
  std::unique_ptr<visage::Frame> shapes_;
  std::unique_ptr<AnimatedLines> animated_lines_;
};
