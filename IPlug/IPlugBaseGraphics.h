#pragma once

#include "IPlugBase.h"
#include "IGraphics.h"
#include "IControl.h"

class IGraphics;

class IPlugBaseGraphics : public IPlugBase
{
public:
  
  ~IPlugBaseGraphics();
  IGraphics* GetGUI() { return mGraphics; }
  
  void AttachGraphics(IGraphics* pGraphics);
  void RedrawParamControls() override;
  
private:
  IGraphics* mGraphics = nullptr;
};
