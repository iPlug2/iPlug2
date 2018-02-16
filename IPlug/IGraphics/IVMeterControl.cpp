#include "IVMeterControl.h"

const IColor IVMeterControl::DEFAULT_BG_COLOR = IColor(255, 70, 70, 70);
const IColor IVMeterControl::DEFAULT_M_COLOR = IColor(255, 240, 240, 240);
const IColor IVMeterControl::DEFAULT_PK_COLOR = IColor(255, 255, 60, 60);
const IColor IVMeterControl::DEFAULT_TXT_COLOR = DEFAULT_BG_COLOR;
const IColor IVMeterControl::DEFAULT_FR_COLOR = DEFAULT_TXT_COLOR;

/*
IVMeterControl::IVMeterControl(IDelegate & dlg, IRECT rect, int paramIdx, double * inputBuf)
  {
  }
*/

void IVMeterControl::Draw(IGraphics& graphics){
    float v = (float) mValue;
    double fps = graphics.FPS();
    // todo watch out for 0 in db conversions
    auto meterRect = GetMeterRect();

    //auto dynRange = mMaxDisplayVal - mMinDisplayVal;

    // background and shadows
    auto bgRect = meterRect;
    if (mShowODRect) bgRect.T -= mODRectHeight;
    graphics.FillRect(GetColor(mBg), bgRect);

    // actual value rect
    auto valR = meterRect;
    valR.T = valR.B - v * valR.H();
    graphics.FillRect(GetColor(mM), valR);

    auto pc = GetColor(mPk);

    // overdrive rect
    // graphics stuff
    auto odRect = meterRect;
    odRect.T = meterRect.T - mODRectHeight;
    odRect.B = meterRect.T;
    pc = LinearBlendColors(COLOR_TRANSPARENT, GetColor(mPk), mODBlink);
    graphics.FillRect(pc, odRect);
    // math
    if (!mHoldingAPeak)
      mODBlink *= GetExpForDrop(1000.0 + 2.5 * mDropMs, fps);

    // memory rect
    // math
    auto p = GetPeakFromMemExp();
    if (p < 0.0) p = 0.0;
    if (p < v || p == 0.0) {
      p = v;
      mMemPeak = v;
      mMemExp = 1.0;
      }
    else {
      auto t = mDropMs;
      if (p > 0.0 && p < 1.0) t /= p; // low values should decay ~at the same rate
      mMemExp *= GetInvExpForDrop(t, fps);
      }
    // graphics
    auto memR = meterRect;
    memR.T = memR.B - p * meterRect.H();
    memR.B = valR.T;
    auto c = GetColor(mM);
    c.A /= 2;
    graphics.FillRect(c, memR);
    pc = LinearBlendColors(GetColor(mM), GetColor(mPk), mODBlink);
    graphics.DrawLine(pc, valR.L, memR.T, valR.R, memR.T);


    SetDirty();

#ifdef _DEBUG
    auto txt = mText;
    txt.mFGColor = COLOR_GREEN;
    WDL_String fpss;
    fpss.SetFormatted(16, "fps %d \np_e %1.3f", (int)fps, p);
    float th, tw;
    BasicTextMeasure(fpss.Get(), th, tw);
    auto dtr = valR;
    dtr.T = dtr.B - th * txt.mSize - 30.0f;
    graphics.DrawTextA(txt, fpss.Get(),dtr);
#endif

    }