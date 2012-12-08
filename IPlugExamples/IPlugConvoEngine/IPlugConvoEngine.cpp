/*

 IPlug convoengine example
 (c) Theo Niessink 2010
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


 A simple IPlug plug-in effect that shows how to use WDL's fast convolution
 engine.

 */

// WDL-OL Version

#include "IPlugConvoEngine.h"
#include "IPlug_include_in_plug_src.h"
#include "IAutoGUI.h"

#ifdef OS_OSX
// need xcode/gcc to treat this file as C++ for the Mac RTAS build
#include "../../WDL/fft.c"
#endif

enum EParams
{
  kDry,
  kWet,
  kNumParams
};

IPlugConvoEngine::IPlugConvoEngine(IPlugInstanceInfo instanceInfo):
  IPLUG_CTOR(kNumParams, 0, instanceInfo)
  , mSampleRate(0.)
  , mWet(1.)
  , mDry(0.)
{
  TRACE;

  GetParam(kDry)->InitDouble("Dry", 1.,  0., 1., 0.001);
  GetParam(kWet)->InitDouble("Wet", 0.5, 0., 1., 0.001);
  
  IGraphics* pGraphics = MakeGraphics(this, GUI_WIDTH, GUI_HEIGHT);
  IText textProps(12, &COLOR_BLACK, "Verdana", IText::kStyleNormal, IText::kAlignNear, 0, IText::kQualityNonAntiAliased);
	GenerateKnobGUI(pGraphics, this, &textProps, &COLOR_WHITE, &COLOR_BLACK, 60, 70);
  AttachGraphics(pGraphics);
}

void IPlugConvoEngine::OnParamChange(int paramIdx)
{
  IMutexLock lock(this);

  switch (paramIdx)
  {
    case kDry:
      mDry = GetParam(kDry)->Value();
      break;

    case kWet:
      mWet = GetParam(kWet)->Value();
      break;
  }
}

void IPlugConvoEngine::Reset()
{
  TRACE; IMutexLock lock(this);

  // Detect a change in sample rate.
  if (GetSampleRate() != mSampleRate)
  {
    mSampleRate = GetSampleRate();

    // Create a mono impulse response of 250 ms.
    mImpulse.SetNumChannels(1);
    int nSamples = mImpulse.SetLength(int(0.250 * mSampleRate + 0.5));
    if (nSamples > 0)
    {
      WDL_FFT_REAL* buf = mImpulse.impulses[0].Get();
      memset(buf, 0, nSamples * sizeof(WDL_FFT_REAL));

      // Set echo taps every ~25 ms.
      buf[0] = 0.5;
      buf[int(0.1 * (double)nSamples)] = 0.5;
      buf[int(0.2 * (double)nSamples)] = 0.46875;
      buf[int(0.3 * (double)nSamples)] = 0.4375;
      buf[int(0.4 * (double)nSamples)] = 0.40625;
      buf[int(0.5 * (double)nSamples)] = 0.375;
      buf[int(0.6 * (double)nSamples)] = 0.34375;
      buf[int(0.7 * (double)nSamples)] = 0.3125;
      buf[int(0.8 * (double)nSamples)] = 0.28125;
      buf[int(0.9 * (double)nSamples)] = 0.25;
    }

    // Tie the impulse response to the convolution engine.
    mEngine.SetImpulse(&mImpulse, 0);
  }
}

void IPlugConvoEngine::ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames)
{
  // Send input samples to the convolution engine.
#if WDL_FFT_REALSIZE == 8
  mEngine.Add(inputs, nFrames, 1);
#else
  {
    // Convert the input samples from doubles to WDL_FFT_REALs.
    double* in = inputs[0];
    // Use outputs[0] as a temporary buffer.
    WDL_FFT_REAL* tmp = (WDL_FFT_REAL*)outputs[0];
    for (int i = 0; i < nFrames; ++i) *tmp++ = *in++;
    mEngine.Add((WDL_FFT_REAL**)outputs, nFrames, 1);
  }
#endif

  double* in = inputs[0];
  double *out_l = outputs[0];
  double *out_r = outputs[1];

  int nAvail = IPMIN(mEngine.Avail(nFrames), nFrames);

  // If not enough samples are available yet, then only output the dry
  // signal.
  for (int i = 0; i < nFrames - nAvail; ++i) *out_l++ = mDry * *in++;

  // Output samples from the convolution engine.
  if (nAvail > 0)
  {
    // Apply the dry/wet mix (and convert from WDL_FFT_REALs back to
    // doubles).
    WDL_FFT_REAL* convo = mEngine.Get()[0];
    for (int i = 0; i < nAvail; ++i) *out_l++ = *out_r++ = mDry * *in++ + mWet * *convo++;

    // Remove the sample block from the convolution engine's buffer.
    mEngine.Advance(nAvail);
  }
}
