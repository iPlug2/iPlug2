/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc TestKeyboardControl
 */

#include "IControl.h"

const char* vk_to_string(int vk_code)
{
  switch(vk_code)
  {
    case kVK_HELP:                 return "VK_HELP";
    case kVK_BACK:                 return "VK_BACK";
    case kVK_TAB:                  return "VK_TAB";
    case kVK_CLEAR:                return "VK_CLEAR";
    case kVK_RETURN:               return "VK_RETURN";
    case kVK_SHIFT:                return "VK_SHIFT";
    case kVK_CONTROL:              return "VK_CONTROL";
    case kVK_MENU:                 return "VK_MENU";
    case kVK_PAUSE:                return "VK_PAUSE";
    case kVK_CAPITAL:              return "VK_CAPITAL";
    case kVK_ESCAPE:               return "VK_ESCAPE";
    case kVK_SPACE:                return "VK_SPACE";
    case kVK_PRIOR:                return "VK_PAGE_UP";
    case kVK_NEXT:                 return "VK_PAGE_DOWN";
    case kVK_END:                  return "VK_END";
    case kVK_HOME:                 return "VK_HOME";
    case kVK_LEFT:                 return "VK_LEFT";
    case kVK_UP:                   return "VK_UP";
    case kVK_RIGHT:                return "VK_RIGHT";
    case kVK_DOWN:                 return "VK_DOWN";
    case kVK_SELECT:               return "VK_SELECT";
    case kVK_PRINT:                return "VK_PRINT";
    case kVK_INSERT:               return "VK_INSERT";
    case kVK_DELETE:               return "VK_DELETE";
    case kVK_NUMPAD0:              return "VK_NUMPAD0";
    case kVK_NUMPAD1:              return "VK_NUMPAD1";
    case kVK_NUMPAD2:              return "VK_NUMPAD2";
    case kVK_NUMPAD3:              return "VK_NUMPAD3";
    case kVK_NUMPAD4:              return "VK_NUMPAD4";
    case kVK_NUMPAD5:              return "VK_NUMPAD5";
    case kVK_NUMPAD6:              return "VK_NUMPAD6";
    case kVK_NUMPAD7:              return "VK_NUMPAD7";
    case kVK_NUMPAD8:              return "VK_NUMPAD8";
    case kVK_NUMPAD9:              return "VK_NUMPAD9";
    case kVK_MULTIPLY:             return "VK_MULTIPLY";
    case kVK_ADD:                  return "VK_ADD";
    case kVK_SEPARATOR:            return "VK_SEPARATOR";
    case kVK_SUBTRACT:             return "VK_SUBTRACT";
    case kVK_DECIMAL:              return "VK_DECIMAL";
    case kVK_DIVIDE:               return "VK_DIVIDE";
    case kVK_F1:                   return "VK_F1";
    case kVK_F2:                   return "VK_F2";
    case kVK_F3:                   return "VK_F3";
    case kVK_F4:                   return "VK_F4";
    case kVK_F5:                   return "VK_F5";
    case kVK_F6:                   return "VK_F6";
    case kVK_F7:                   return "VK_F7";
    case kVK_F8:                   return "VK_F8";
    case kVK_F9:                   return "VK_F9";
    case kVK_F10:                  return "VK_F10";
    case kVK_F11:                  return "VK_F11";
    case kVK_F12:                  return "VK_F12";
    case kVK_F13:                  return "VK_F13";
    case kVK_F14:                  return "VK_F14";
    case kVK_F15:                  return "VK_F15";
    case kVK_F16:                  return "VK_F16";
    case kVK_F17:                  return "VK_F17";
    case kVK_F18:                  return "VK_F18";
    case kVK_F19:                  return "VK_F19";
    case kVK_F20:                  return "VK_F20";
    case kVK_F21:                  return "VK_F21";
    case kVK_F22:                  return "VK_F22";
    case kVK_F23:                  return "VK_F23";
    case kVK_F24:                  return "VK_F24";
    case kVK_NUMLOCK:              return "VK_NUMLOCK";
    case kVK_SCROLL:               return "VK_SCROLL";
    case kVK_RETURN|0x8000:        return "ENTER";
    default:                      return "Unknown VK code";
  }
}

/** Control to test keyboard input
 *   @ingroup TestControls */
class TestKeyboardControl : public IControl
{
public:
  TestKeyboardControl(const IRECT& rect)
  : IControl(rect)
  {
    mX = rect.MW();
    mY = rect.MH();
    mStr.Set("Press a key...");
    SetTooltip("TestKeyboardControl");
  }

  void Draw(IGraphics& g) override
  {
    g.FillRect(COLOR_BLACK, mRECT);

    if (g.CheckLayer(mLayer))
    {
      g.ResumeLayer(mLayer);

      if(mNewText)
      {
        g.DrawText(IText(static_cast<float>((rand() % 50) + 10), COLOR_WHITE), mStr.Get(), mX, mY);
        mNewText = false;
      }

      if(GetAnimationFunction())
      {
        g.FillRect(COLOR_BLACK, mRECT, &BLEND_05);
      }
    }
    else
    {
      g.StartLayer(this, mRECT);
      g.DrawText(IText(20, COLOR_WHITE), mStr.Get(), mX, mY);
    }

    mLayer = g.EndLayer();

    g.DrawLayer(mLayer);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    mLayer->Invalidate();
    SetDirty(false);
  }

  bool OnKeyDown(float x, float y, const IKeyPress& key) override
  {
    mStr.Set(vk_to_string(key.VK));

    if(strcmp(mStr.Get(),"Unknown VK code")==0)
    {
      mStr.Set(key.utf8);
    }
    
    mNewText = true;
    SetAnimation(DefaultAnimationFunc);
    StartAnimation(5000);
    mX = x;
    mY = y;

    SetDirty(false);

    return true;
  }

private:
  bool mNewText = false;
  float mX = 0.;
  float mY = 0.;
  WDL_String mStr;
  ILayerPtr mLayer;
};
