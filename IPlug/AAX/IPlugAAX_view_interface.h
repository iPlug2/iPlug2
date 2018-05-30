/*
 ==============================================================================
 
 This file is part of the iPlug 2 library
 
 Oli Larkin et al. 2018 - https://www.olilarkin.co.uk
 
 iPlug 2 is an open source library subject to commercial or open-source
 licensing.
 
 The code included in this file is provided under the terms of the WDL license
 - https://www.cockos.com/wdl/
 
 ==============================================================================
 */

#pragma once
// you can inherit this class in your GUI framework editor in order to get a pointer to the AAX viewcontainter

#include "AAX_IViewContainer.h"


class IPlugAAXView_Interface
{
public:
  IPlugAAXView_Interface() {};
  virtual void SetPTParameterHighlight(int paramIdx, bool isHighlighted, int colour) {};
  void SetViewContainer(AAX_IViewContainer* viewContainer) { mAAXViewContainer = viewContainer; }
  AAX_IViewContainer* GetViewContainer() { return mAAXViewContainer; }
protected:
  AAX_IViewContainer* mAAXViewContainer = nullptr;
};
