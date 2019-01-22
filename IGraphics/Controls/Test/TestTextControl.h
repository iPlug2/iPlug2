/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc TestTextControl
 */

#include "IControl.h"

/** Control to test drawing text
 *   @ingroup TestControls */
class TestTextControl : public IControl
{
public:
  TestTextControl(IRECT bounds)
  : IControl(bounds)
  {
    SetTooltip("TestTextControl");
    Randomise();
  }

  void Draw(IGraphics& g) override
  {
    const char* words[] = { "there", "are many" , "possible", "ways", "to display text", "here" };

    g.FillRect(COLOR_BLACK, mRECT);
    g.DrawText(mText, words[mStringIndex], mRECT);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    Randomise();
    SetDirty(false);
  }

  void Randomise()
  {
    int size = (std::rand() % 100) + 5;
    int style = (std::rand() % 3);
    int align = (std::rand() % 3);
    int valign = (std::rand() % 3);
    int type = (std::rand() % 2);
    mStringIndex = (std::rand() % 6);

    const char* types[] = { "Roboto-Regular", "Montserrat-LightItalic" };

    mText = IText(size, IColor::GetRandomColor(), types[type], (IText::EStyle) style, (IText::EAlign) align, (IText::EVAlign) valign);
  }

private:
  int mStringIndex;
};
