#pragma once

#include "IControls.h"
//#include "IPlugPaths.h"

void IGraphics::GenerateSliderGUI(const IRECT& bounds, int startIdx, int endIdx, int paramJump, const char* groupName, int rows, int columns, EDirection dir, const char** pParamNameStrings)
{
  IPluginDelegate& dlg = dynamic_cast<IPluginDelegate&>(GetDelegate()); // TODO: will crash if not a plugin
  
  IRECT sliderBounds = bounds;
  int cellIdx = 0;
  IText labelText;

  // header row is the first row
  //  IRECT header = bounds.SubRect(dir, rows + 1, 0);
  //  IRECT sliders = bounds.GetPadded(0, header.H(), 0, 0);
  //  AttachControl(new IPanelControl(dlg, header, DEFAULT_GRAPHICS_BGCOLOR));
  
  std::function<void(int, IParam&)> makeCell = [&](int paramIdx, IParam& param) {
    IRECT r = sliderBounds.GetGridCell(cellIdx++, rows, columns);
    IRECT sliderInfoRect = r.SubRectVertical(2, 0);
    AttachControl(new ITextControl(dlg, sliderInfoRect.FracRectHorizontal(0.75), labelText, param.GetNameForHost()));
    AttachControl(new ICaptionControl(dlg, sliderInfoRect.FracRectHorizontal(0.25, true), paramIdx, labelText, true));
    AttachControl(new IVSliderControl(dlg, r.SubRectVertical(2, 1), paramIdx, DEFAULT_SPEC, EDirection::kHorizontal));
  };

  if(CStringHasContents(groupName))
  {
    dlg.ForParamInGroup(groupName, [&](int paramIdx, IParam& param){
      makeCell(paramIdx, param);
    });
  }
  else
  {
    dlg.ForParamInRange(startIdx, endIdx, [&](int paramIdx, IParam& param){
      makeCell(paramIdx, param);
    });
  }
}
