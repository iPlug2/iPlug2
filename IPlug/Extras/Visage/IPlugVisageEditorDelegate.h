/*
 ==============================================================================

  MIT License

  iPlug2 Visage Support
  Copyright (c) 2025 Oliver Larkin

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

 ==============================================================================
*/

#pragma once

#include "IPlugEditorDelegate.h"
#include <visage/app.h>
#include <visage/windowing.h>
#include <visage/graphics.h>
#include <functional>
#include <memory>

/**
 * @file
 * @copydoc VisageEditorDelegate
 */

BEGIN_IPLUG_NAMESPACE

/** An editor delegate base class that uses Visage for the UI
 * @ingroup EditorDelegates */
class VisageEditorDelegate : public IEditorDelegate
{
public:
  VisageEditorDelegate(int nParams);
  virtual ~VisageEditorDelegate();

  // IEditorDelegate overrides
  void* OpenWindow(void* pParent) override;
  void CloseWindow() override;
  void OnParentWindowResize(int width, int height) override;

  /** Get the Visage editor (for adding child frames, etc.)
   * @return Pointer to the ApplicationEditor, or nullptr if not open */
  visage::ApplicationEditor* GetEditor() { return mEditor.get(); }

  /** Get the Visage window
   * @return Pointer to the Window, or nullptr if not open */
  visage::Window* GetWindow() { return mWindow.get(); }

  /** Request a redraw of the editor */
  void Redraw()
  {
    if (mEditor)
      mEditor->redraw();
  }

protected:
  /** Override to draw custom UI content
   * @param canvas The Visage canvas to draw on */
  virtual void OnDraw(visage::Canvas& canvas) {}

  /** Override to handle mouse down events
   * @param e The mouse event */
  virtual void OnMouseDown(const visage::MouseEvent& e) {}

  /** Override to handle mouse drag events
   * @param e The mouse event */
  virtual void OnMouseDrag(const visage::MouseEvent& e) {}

  /** Override to handle mouse up events
   * @param e The mouse event */
  virtual void OnMouseUp(const visage::MouseEvent& e) {}

  /** Override to handle mouse move events
   * @param e The mouse event */
  virtual void OnMouseMove(const visage::MouseEvent& e) {}

  /** Override to handle mouse wheel events
   * @param e The mouse event
   * @return true if the event was handled */
  virtual bool OnMouseWheel(const visage::MouseEvent& e) { return false; }

private:
  std::unique_ptr<visage::ApplicationEditor> mEditor;
  std::unique_ptr<visage::Window> mWindow;
};

END_IPLUG_NAMESPACE
