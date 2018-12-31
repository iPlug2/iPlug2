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

#if !defined OS_WIN
  #if defined OS_IOS
    #include "swell-ios.h"
  #else
    #include "swell-types.h"
  #endif
#endif

const char* vk_to_string(int vk_code)
{
  switch(vk_code)
  {
    case VK_HELP:                 return "VK_HELP";
    case VK_BACK:                 return "VK_BACK";
    case VK_TAB:                  return "VK_TAB";
    case VK_CLEAR:                return "VK_CLEAR";
    case VK_RETURN:               return "VK_RETURN";
    case VK_SHIFT:                return "VK_SHIFT";
    case VK_CONTROL:              return "VK_CONTROL";
    case VK_MENU:                 return "VK_MENU";
    case VK_PAUSE:                return "VK_PAUSE";
    case VK_CAPITAL:              return "VK_CAPITAL";
    case VK_ESCAPE:               return "VK_ESCAPE";
    case VK_SPACE:                return "VK_SPACE";
    case VK_PRIOR:                return "VK_PAGE_UP";
    case VK_NEXT:                 return "VK_PAGE_DOWN";
    case VK_END:                  return "VK_END";
    case VK_HOME:                 return "VK_HOME";
    case VK_LEFT:                 return "VK_LEFT";
    case VK_UP:                   return "VK_UP";
    case VK_RIGHT:                return "VK_RIGHT";
    case VK_DOWN:                 return "VK_DOWN";
    case VK_SELECT:               return "VK_SELECT";
    case VK_PRINT:                return "VK_PRINT";
    case VK_INSERT:               return "VK_INSERT";
    case VK_DELETE:               return "VK_DELETE";
    case VK_NUMPAD0:              return "VK_NUMPAD0";
    case VK_NUMPAD1:              return "VK_NUMPAD1";
    case VK_NUMPAD2:              return "VK_NUMPAD2";
    case VK_NUMPAD3:              return "VK_NUMPAD3";
    case VK_NUMPAD4:              return "VK_NUMPAD4";
    case VK_NUMPAD5:              return "VK_NUMPAD5";
    case VK_NUMPAD6:              return "VK_NUMPAD6";
    case VK_NUMPAD7:              return "VK_NUMPAD7";
    case VK_NUMPAD8:              return "VK_NUMPAD8";
    case VK_NUMPAD9:              return "VK_NUMPAD9";
    case VK_MULTIPLY:             return "VK_MULTIPLY";
    case VK_ADD:                  return "VK_ADD";
    case VK_SEPARATOR:            return "VK_SEPARATOR";
    case VK_SUBTRACT:             return "VK_SUBTRACT";
    case VK_DECIMAL:              return "VK_DECIMAL";
    case VK_DIVIDE:               return "VK_DIVIDE";
    case VK_F1:                   return "VK_F1";
    case VK_F2:                   return "VK_F2";
    case VK_F3:                   return "VK_F3";
    case VK_F4:                   return "VK_F4";
    case VK_F5:                   return "VK_F5";
    case VK_F6:                   return "VK_F6";
    case VK_F7:                   return "VK_F7";
    case VK_F8:                   return "VK_F8";
    case VK_F9:                   return "VK_F9";
    case VK_F10:                  return "VK_F10";
    case VK_F11:                  return "VK_F11";
    case VK_F12:                  return "VK_F12";
    case VK_F13:                  return "VK_F13";
    case VK_F14:                  return "VK_F14";
    case VK_F15:                  return "VK_F15";
    case VK_F16:                  return "VK_F16";
    case VK_F17:                  return "VK_F17";
    case VK_F18:                  return "VK_F18";
    case VK_F19:                  return "VK_F19";
    case VK_F20:                  return "VK_F20";
    case VK_F21:                  return "VK_F21";
    case VK_F22:                  return "VK_F22";
    case VK_F23:                  return "VK_F23";
    case VK_F24:                  return "VK_F24";
    case VK_NUMLOCK:              return "VK_NUMLOCK";
    case VK_SCROLL:               return "VK_SCROLL";
    case VK_RETURN|0x8000:        return "ENTER";
    default:                      return "Unknown VK code";
  }
}

/** Control to test keyboard input
 *   @ingroup TestControls */
class TestKeyboardControl : public IControl
{
public:
  TestKeyboardControl(IGEditorDelegate& dlg, IRECT rect)
  : IControl(dlg, rect)
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
        g.DrawText(IText((rand() % 50) + 10, COLOR_WHITE), mStr.Get(), mX, mY);
        mNewText = false;
      }

      if(GetAnimationFunction())
      {
        g.FillRect(COLOR_BLACK, mRECT, &BLEND_05);
      }
    }
    else
    {
      g.StartLayer(mRECT);
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
      mStr.Set(&key.Ascii);

    mNewText = true;
    SetAnimation(DefaultAnimationFunc);
    StartAnimation(5000.);
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
