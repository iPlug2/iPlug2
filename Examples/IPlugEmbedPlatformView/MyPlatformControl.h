#pragma once

#include "IPlatformViewControl.h"

using namespace iplug::igraphics;

class MyPlatformControl : public IPlatformViewControl
{
public:
  MyPlatformControl(const IRECT& bounds)
  : IPlatformViewControl(bounds)
  {
  }
  
//  void OnRescale() override
//  {
//  }
  
  void AttachSubViews(void* pPlatformView) override;
};


