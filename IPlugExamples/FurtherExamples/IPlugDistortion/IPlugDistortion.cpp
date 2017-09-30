/*

 IPlug distortion example
 (c) Theo Niessink 2011
 <http://www.taletn.com/>

 This software is provided 'as-is', without any express or implied
 warranty. In no event will the authors be held liable for any damages
 arising from the use of this software.

 Permission is granted to anyone to use this software for any purpose,
 including commercial applications, and to alter it and redistribute it
 freely, subject to the following restrictions:

 1. The origin of this software must not be misrepresented; you must not
 claim that you wrote the original software. If you use this software in a
 product, an acknowledgment in the product documentation would be
 appreciated but is not required.
 2. Altered source versions must be plainly marked as such, and must not be
 misrepresented as being the original software.
 3. This notice may not be removed or altered from any source distribution.


 Simple IPlug audio effect that shows how to implement oversampling to reduce
 aliasing.

 */


#include "IPlugDistortion.h"
#include "IPlug_include_in_plug_src.h"
#include "IAutoGUI.h"

#include <math.h>
#include "../../WDL/denormal.h"


inline double fast_tanh(double x)
{
  x = exp(x + x);
  return (x - 1) / (x + 1);
}


IPlugDistortion::IPlugDistortion(IPlugInstanceInfo instanceInfo):
  IPLUG_CTOR(kNumParams, 0, instanceInfo),
  mOversampling(8), mDC(0.2)
{
  TRACE;

  mAntiAlias.Calc(0.5 / (double)mOversampling);
  mUpsample.Reset();
  mDownsample.Reset();

  mDistortedDC = fast_tanh(mDC);

  GetParam(kDrive)->InitDouble("Drive", 0.5, 0., 1., 0.001);
  
  IGraphics* pGraphics = MakeGraphics(this, GUI_WIDTH, GUI_HEIGHT);
  IText textProps(12, &COLOR_BLACK, "Verdana", IText::kStyleNormal, IText::kAlignNear, 0, IText::kQualityNonAntiAliased);
	GenerateKnobGUI(pGraphics, this, &textProps, &COLOR_WHITE, &COLOR_BLACK, 60, 70);
  AttachGraphics(pGraphics);
}


void IPlugDistortion::OnParamChange(int paramIdx)
{
  IMutexLock lock(this);

  switch (paramIdx)
  {
    case kDrive:
    {
      double value = GetParam(kDrive)->Value();
      mDrive = 1. + 15. * value;
      mGain = pow(2., value) / mDrive;
      break;
    }
  }
}


void IPlugDistortion::ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames)
{
  bool isMono = !IsInChannelConnected(1);

  for (int i = 0; i < nFrames; ++i)
  {
    double sample = isMono ? inputs[0][i] : 0.5 * (inputs[0][i] + inputs[1][i]);
    double output;

    for (int j = 0; j < mOversampling; ++j)
    {
      // Upsample
      if (j > 0) sample = 0.;
      mUpsample.Process(sample, mAntiAlias.Coeffs());
      sample = (double)mOversampling * mUpsample.Output();

      // Distortion
      if (WDL_DENORMAL_OR_ZERO_DOUBLE_AGGRESSIVE(&sample))
        sample = 0.;
      else
        sample = mGain * (fast_tanh(mDC + mDrive * sample) - mDistortedDC);

      // Downsample
      mDownsample.Process(sample, mAntiAlias.Coeffs());
      if (j == 0) output = mDownsample.Output();
    }

    outputs[0][i] = outputs[1][i] = output;
  }
}
