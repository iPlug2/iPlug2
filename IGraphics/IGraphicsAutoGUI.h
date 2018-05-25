#pragma once

#include "IControls.h"
#include "wdlstring.h"
#include "IPlugPaths.h"

void IGraphics::GenerateSliderGUI(const char* group, const IRECT& bounds, int rows, int columns, EDirection dir, const char** pParamNameStrings)
{
  IDelegate& dlg = GetDelegate();
  
  const int nCells = rows * columns;
  const int nParams = dlg.NParams();
  
  // header row is the first row
//  IRECT header = bounds.SubRect(dir, rows + 1, 0);
//  IRECT sliders = bounds.GetPadded(0, header.H(), 0, 0);
  IRECT sliders = bounds;
//  AttachControl(new IPanelControl(dlg, header, DEFAULT_GRAPHICS_BGCOLOR));

  IText labelText;
  
  int paramIdx = 0;
  int foundIdx = 0;
  
  for (auto paramIdx = 0; paramIdx < nParams; paramIdx++)
  {
    IParam* pParam = dlg.GetParam(paramIdx);
    
    if(strcmp(pParam->GetGroupForHost(), group) == 0 )
    {
      IRECT r = sliders.GetGridCell(foundIdx++, rows, columns);
      IRECT sliderInfoRect = r.SubRectVertical(2, 0);
      AttachControl(new ITextControl(dlg, sliderInfoRect.SubRectHorizontal(2, 0), labelText, pParam->GetNameForHost()));
      AttachControl(new ICaptionControl(dlg, sliderInfoRect.SubRectHorizontal(2, 1), paramIdx, labelText, true));
      AttachControl(new IVSliderControl(dlg, r.SubRectVertical(2, 1), paramIdx, DEFAULT_SPEC, EDirection::kHorizontal));
    }
  }
}
