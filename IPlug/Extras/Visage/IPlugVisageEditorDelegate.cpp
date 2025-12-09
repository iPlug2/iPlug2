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

#include "IPlugVisageEditorDelegate.h"

BEGIN_IPLUG_NAMESPACE

VisageEditorDelegate::VisageEditorDelegate(int nParams)
: IEditorDelegate(nParams)
{
}

VisageEditorDelegate::~VisageEditorDelegate()
{
  CloseWindow();
}

void* VisageEditorDelegate::OpenWindow(void* pParent)
{
  mEditor = std::make_unique<visage::ApplicationEditor>();
  visage::IBounds bounds = visage::computeWindowBounds(GetEditorWidth(), GetEditorHeight());
  mEditor->setBounds(0, 0, bounds.width(), bounds.height());

  mEditor->onDraw() = [this](visage::Canvas& canvas) {
    OnDraw(canvas);
  };

  mEditor->onMouseDown() = [this](const visage::MouseEvent& e) {
    OnMouseDown(e);
  };

  mEditor->onMouseDrag() = [this](const visage::MouseEvent& e) {
    OnMouseDrag(e);
  };

  mEditor->onMouseUp() = [this](const visage::MouseEvent& e) {
    OnMouseUp(e);
  };

  mEditor->onMouseMove() = [this](const visage::MouseEvent& e) {
    OnMouseMove(e);
  };

  mEditor->onMouseWheel() = [this](const visage::MouseEvent& e) {
    return OnMouseWheel(e);
  };

  mWindow = visage::createPluginWindow(mEditor->width(), mEditor->height(), pParent);
  mEditor->addToWindow(mWindow.get());
  mWindow->show();

  OnUIOpen();
  return mWindow->nativeHandle();
}

void VisageEditorDelegate::CloseWindow()
{
  OnUIClose();
  if (mEditor)
    mEditor->removeFromWindow();
  mWindow.reset();
  mEditor.reset();
}

void VisageEditorDelegate::OnParentWindowResize(int width, int height)
{
  if (mWindow && mEditor)
  {
    mWindow->setWindowSize(width, height);
    mEditor->setBounds(0, 0, width, height);
  }
}

END_IPLUG_NAMESPACE
