#include "IGraphicsTest.h"
#include "IGraphicsTest_controls.h"

#include "IPlugParameter.h"
#include "IControls.h"

void IGraphicsTest::init()
{
  IGraphicsWeb* pGraphics = new IGraphicsWeb(*this, UI_WIDTH, UI_HEIGHT, UI_FPS);

//   pGraphics->OpenWindow((void*)gHWND);
  pGraphics->AttachPanelBackground(COLOR_RED);
  pGraphics->HandleMouseOver(true);
  
  mGraphics = pGraphics;
  
  mGraphics->Resize(400, 400, 2);
  mGraphics->AttachControl(new IGradientControl(*this, IRECT(20, 20, 150, 120), -1));
  mGraphics->AttachControl(new IPolyControl(*this, IRECT(20, 200, 150, 330), -1));
  mGraphics->AttachControl(new IArcControl(*this, IRECT(220, 20, 320, 120), -1));
  
  mGraphics->GetControl(2)->SetValueFromDelegate((double) rand() / RAND_MAX);
  mGraphics->GetControl(3)->SetValueFromDelegate((double) rand() / RAND_MAX);

mGraphics->Draw(mGraphics->GetBounds());
}

void IGraphicsTest::SetParameterValueFromUI(int paramIdx, double value)
{
  DBGMSG("SetParameterValueFromUI p %i %f\n", paramIdx, value);
}

void IGraphicsTest::ResizeGraphicsFromUI()
{
}
