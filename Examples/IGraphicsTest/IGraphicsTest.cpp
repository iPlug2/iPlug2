#include "IGraphicsTest.h"

#include "IPlugParameter.h"
#include "IControls.h"

void IGraphicsTest::init()
{
  IGraphicsWeb* pGraphics = new IGraphicsWeb(*this, UI_WIDTH, UI_HEIGHT, UI_FPS);

//   pGraphics->OpenWindow((void*)gHWND);
  pGraphics->AttachPanelBackground(COLOR_RED);
  pGraphics->HandleMouseOver(true);

  mGraphics = pGraphics;
}

void IGraphicsTest::SetParameterValueFromUI(int paramIdx, double value)
{
  DBGMSG("SetParameterValueFromUI p %i %f\n", paramIdx, value);
}

void IGraphicsTest::ResizeGraphicsFromUI()
{
}
