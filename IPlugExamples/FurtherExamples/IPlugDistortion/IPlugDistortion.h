#ifndef __IPLUGDISTORTION_H__
#define __IPLUGDISTORTION_H__

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


#include "IPlug_include_in_plug_hdr.h"

#define WDL_BESSEL_FILTER_ORDER 8
#define WDL_BESSEL_DENORMAL_AGGRESSIVE
#include "../../WDL/besselfilter.h"


enum EParams
{
  kDrive = 0,
  kNumParams
};


class IPlugDistortion: public IPlug
{
public:
  IPlugDistortion(IPlugInstanceInfo instanceInfo);
  ~IPlugDistortion() {}

  void OnParamChange(int paramIdx);
  void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames);

private:
  const int mOversampling;

  const double mDC;
  double mDistortedDC;

  double mDrive, mGain;

  WDL_BesselFilterCoeffs mAntiAlias;
  WDL_BesselFilterStage mUpsample, mDownsample;
};


#endif // __IPLUGDISTORTION_H__
