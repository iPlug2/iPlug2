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
  TestTextControl(const IRECT& bounds)
  : IControl(bounds)
  {
    SetTooltip("TestTextControl");
    Randomise();
  }

  void Draw(IGraphics& g) override
  {
    const char* words[] = { "there", "are many" , "possible", "ways", "to display text", "here" };

    g.FillRect(COLOR_WHITE, mRECT);
    g.DrawText(mText, words[mStringIndex], mRECT);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    Randomise();
    SetDirty(false);
  }
    
  void OnMouseDblClick(float x, float y, const IMouseMod& mod) override
  {
    GetUI()->CreateTextEntry(*this, mText, mRECT);
    SetDirty(false);
  }

  void Randomise()
  {
    int size = (std::rand() % 200) + 12;
    int align = (std::rand() % 3);
    int valign = (std::rand() % 3);
    int type = (std::rand() % 2);
    mStringIndex = (std::rand() % 6);

    const char* types[] = { "Roboto-Regular", "Montserrat-LightItalic" };

    mText = IText(static_cast<float>(size), IColor::GetRandomColor(), types[type], (EAlign) align, (EVAlign) valign);
  }

private:
  int mStringIndex;
};
