/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc IVColorPickerControl
 */

#include "IControl.h"
#include <unordered_map>

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

using ILayoutMap = std::unordered_map<const char*, IRECT>;

/** A control for choosing a color
 * @ingroup IControls */
class IVColorPickerControl : public IContainerBase
                           , public IVectorBase
{
public:
  IVColorPickerControl(const IVStyle& style = DEFAULT_STYLE);

  void OnAttached() override;
  void OnResize() override;
  void Draw(IGraphics& g) override;
  void OnMouseDown(float x, float y, const IMouseMod& mod) override;
  bool CreateColorPicker(float x, float y, IColor& color, const char* str, IColorPickerHandlerFunc func);
private:
  static const ILayoutMap GetLayout(const IRECT& bounds);
  IRECT mPanel;
  IColor mColor = COLOR_RED;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
