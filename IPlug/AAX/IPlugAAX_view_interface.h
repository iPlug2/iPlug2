/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once
// you can inherit this class in your GUI framework editor in order to get a pointer to the AAX viewcontainter

#include "AAX_IViewContainer.h"

#include "IPlugPlatform.h"

BEGIN_IPLUG_NAMESPACE

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

END_IPLUG_NAMESPACE
