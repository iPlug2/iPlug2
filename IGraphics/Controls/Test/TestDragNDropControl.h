/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc TestDragAndDropControl
 */

#include "IControl.h"

/** Control to test dropping single and multiple files
 *   @ingroup TestControls */
class TestDragAndDropControl : public IControl
{
public:
  TestDragAndDropControl(const IRECT& bounds)
  : IControl(bounds)
  {
    SetTooltip("TestDragAndDropControl - Drag 'n drop here and see what is dropped");
  }

  void Draw(IGraphics& g) override
  {
    g.DrawDottedRect(COLOR_BLACK, mRECT);
    g.FillRect(mMouseIsOver ? COLOR_TRANSLUCENT : COLOR_TRANSPARENT, mRECT);

    if (mPaths.size())
    {
      IRECT r = mRECT.GetFromTop(20);
      g.DrawText(DEFAULT_TEXT, mPaths[0].c_str(), r);
      for (int i = 1; i < mPaths.size(); i++)
      {
        r.T += 20.f;
        g.DrawText(DEFAULT_TEXT, mPaths[i].c_str(), r);
      }
    }
    else
      g.DrawText(DEFAULT_TEXT, "Drop files here", mRECT);
  }

  void OnDrop(const char* str) override
  {
    mPaths.clear();
    mPaths.emplace_back(std::string(str));
    SetDirty(false);
  }

  void OnDropMultiple(const std::vector<const char*>& paths) override
  {
    mPaths.clear();
    for (auto path : paths)
    {
      mPaths.emplace_back(std::string(path));
    }
    SetDirty(false);
  }

private:
  std::vector<std::string> mPaths;
};
