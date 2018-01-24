#include "IPlugEffect.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"
#include "config.h"

class IArcControl : public IKnobControl
{
public:
  IArcControl(IPlugBaseGraphics& plug, IRECT rect, int paramIdx, float angle1 = -PI * 0.75, float angle2 = PI * 0.75) : IKnobControl(plug, rect, paramIdx), mAngle1(angle1), mAngle2(angle2)
  {
    
  }
  
  void Draw(IGraphics& graphics)
  {
    graphics.FillRect(COLOR_GRAY, mRECT.GetPadded(-2));
    graphics.DrawRect(COLOR_BLACK, mRECT.GetPadded(-2));
    double angle = mAngle1 + mValue * (mAngle2 - mAngle1);
    graphics.FillArc(COLOR_BLUE, mRECT.MW(), mRECT.MH(), mRECT.W() * 0.44, mAngle1, angle);
    graphics.DrawArc(COLOR_BLACK, mRECT.MW(), mRECT.MH(), mRECT.W() * 0.44, mAngle1, angle);
    graphics.DrawLine(COLOR_BLACK, mRECT.MW(), mRECT.MH(), mRECT.MW() + cos(angle) * mRECT.W() * 0.49, mRECT.MH() + sin(angle)* mRECT.W() * 0.49);
    graphics.FillCircle(COLOR_WHITE, mRECT.MW(), mRECT.MH(), mRECT.W() * 0.1);
    graphics.DrawCircle(COLOR_BLACK, mRECT.MW(), mRECT.MH(), mRECT.W() * 0.1);
    
    float x1 = mRECT.MW() + cos(angle - 0.3) * mRECT.W() * 0.3;
    float y1 = mRECT.MH() + sin(angle - 0.3) * mRECT.W() * 0.3;
    float x2 = mRECT.MW() + cos(angle + 0.3) * mRECT.W() * 0.3;
    float y2 = mRECT.MH() + sin(angle + 0.3) * mRECT.W() * 0.3;
    float x3 = mRECT.MW() + cos(angle) * mRECT.W() * 0.44;
    float y3 = mRECT.MH() + sin(angle) * mRECT.W() * 0.44;
    
    graphics.FillTriangle(COLOR_WHITE, x1, y1, x2, y2, x3, y3);
    graphics.DrawTriangle(COLOR_BLACK, x1, y1, x2, y2, x3, y3);
  }

  double mAngle1;
  double mAngle2;
};

class IPolyControl : public IKnobControl
{
public:
  IPolyControl(IPlugBaseGraphics& plug, IRECT rect, int paramIdx) : IKnobControl(plug, rect, paramIdx)
  {
    
  }
  
  void Draw(IGraphics& graphics)
  {
    float xarray[32];
    float yarray[32];
    int npoints = 3 + round(mValue * 29);
    double angle = (-0.75 * PI) + mValue * (1.5 * PI);
    double incr = (2 * PI) / npoints;
    double cr = mValue * (mRECT.W() / 2.0);
    
    graphics.FillRoundRect(COLOR_GRAY, mRECT.GetPadded(-2), cr);
    graphics.DrawRoundRect(COLOR_BLACK, mRECT.GetPadded(-2), cr);
    
    for (int i = 0; i < npoints; i++)
    {
      xarray[i] = mRECT.MW() + sin(angle + i * incr) * mRECT.W() * 0.45;
      yarray[i] = mRECT.MH() + cos(angle + i * incr) * mRECT.W() * 0.45;
    }
    
    graphics.FillConvexPolygon(COLOR_ORANGE, xarray, yarray, npoints);
    graphics.DrawConvexPolygon(COLOR_BLACK, xarray, yarray, npoints);
    
  }
};


IPlugEffect::IPlugEffect(IPlugInstanceInfo instanceInfo)
: IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo)
{
  TRACE; 

  //arguments are: name, defaultVal, minVal, maxVal, step, label
  GetParam(kGain)->InitDouble("Gain", 50., 0., 100.0, 0.01, "%");
  GetParam(kGain)->SetShape(2.);

  //create user interface
  IGraphics* pGraphics = MakeGraphics(*this, kWidth, kHeight, 30);
  pGraphics->AttachPanelBackground(COLOR_RED);
  
  pGraphics->AttachControl(new IArcControl(*this, IRECT(30, 100, 130, 200), kGain));
  pGraphics->AttachControl(new IPolyControl(*this, IRECT(150, 100, 250, 200), -1));
//  pGraphics->AttachControl(new ITextControl(*this, IRECT(kTextX, kTextY, 290, kTextY+10), DEFAULT_TEXT, GetBuildInfoStr()));

  WDL_String buildInfo;
  GetBuildInfoStr(buildInfo);
  printf("%s", buildInfo.Get());

  AttachGraphics(pGraphics);
  
  //MakePreset("preset 1", ... );
  MakeDefaultPreset("-", kNumPrograms);
}

IPlugEffect::~IPlugEffect() {}

void IPlugEffect::ProcessBlock(double** inputs, double** outputs, int nFrames)
{
  mParams_mutex.Enter();
  const double gain = GetParam(kGain)->Value() / 100.;
  mParams_mutex.Leave();
  
  double* in1 = inputs[0];
  double* in2 = inputs[1];
  double* out1 = outputs[0];
  double* out2 = outputs[1];

  for (int s = 0; s < nFrames; ++s, ++in1, ++in2, ++out1, ++out2)
  {
    *out1 = *in1 * gain;
    *out2 = *in2 * gain;
  }
}
