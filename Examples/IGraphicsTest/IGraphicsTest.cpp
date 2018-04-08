#include "IGraphicsTest.h"
#include "IGraphics_include_in_plug_src.h"

#include "IGraphicsTest_controls.h"

#include "IControls.h"

IGraphicsTest::IGraphicsTest()
: IGraphicsDelegate(0)
{
  IGraphics* pGraphics = new IGraphicsWeb(*this, UI_WIDTH, UI_HEIGHT, UI_FPS);

//   pGraphics->OpenWindow((void*)gHWND);
  pGraphics->AttachPanelBackground(COLOR_RED);
  pGraphics->HandleMouseOver(true);

  IBitmap knobBitmap = pGraphics->LoadBitmap("knob.png", 60);

  pGraphics->AttachControl(new IGradientControl(*this, IRECT(20, 20, 150, 120), -1));
  pGraphics->AttachControl(new IPolyControl(*this, IRECT(20, 200, 150, 330), -1));
  pGraphics->AttachControl(new IArcControl(*this, IRECT(220, 20, 320, 120), -1));
  pGraphics->AttachControl(new IBKnobControl(*this, 220, 200, knobBitmap, -1));
  pGraphics->AttachControl(new RandomTextControl(*this, IRECT(220, 260, 380, 380)));
  pGraphics->GetControl(2)->SetValueFromDelegate((double) rand() / RAND_MAX);
  pGraphics->GetControl(3)->SetValueFromDelegate((double) rand() / RAND_MAX);
  pGraphics->GetControl(4)->SetValueFromDelegate((double) rand() / RAND_MAX);
  pGraphics->Draw(pGraphics->GetBounds());

  AttachGraphics(pGraphics);
}

void IGraphicsTest::SetParameterValueFromUI(int paramIdx, double value)
{
  DBGMSG("SetParameterValueFromUI p %i %f\n", paramIdx, value);
}
