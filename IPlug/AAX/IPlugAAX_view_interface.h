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
