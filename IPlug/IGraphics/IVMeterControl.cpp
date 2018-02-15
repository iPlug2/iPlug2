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
    float fps = (float) graphics.FPS();
    float pDrop = (float) pow(0.01, 1000.0 / (mPeakDropMs * fps));
    float v = (float) mValue;
    // todo watch out for 0 in db conversions
    auto meterRect = GetMeterRect();

    auto bgRect = meterRect;
    if (mShowPeakRect) bgRect.T -= mPeakRectHeight;
    graphics.FillRect(GetColor(mBg), bgRect);



    auto valR = meterRect;
    valR.T = valR.B - v * valR.H();
    graphics.FillRect(GetColor(mM), valR);


    auto peakR = meterRect;
    peakR.T = meterRect.T - mPeakRectHeight;
    peakR.B = meterRect.T;
    auto prc = LinearBlendColors(IColor(0,0,0,0), GetColor(mPk), mPeakRectBlink);
    mPeakRectBlink *= pDrop;
    graphics.FillRect(prc, peakR);


    auto p = meterRect.B - mPeak * meterRect.H();
    auto memR = meterRect;
    memR.T = p;
    memR.B = valR.T;
    auto cM = GetColor(mM);
    cM.A /= 4;
    graphics.FillRect(cM, memR);
    graphics.DrawLine(GetColor(mPk), valR.L, p, valR.R, p);
    mPeak *= pDrop;
    if (mPeak < v) mPeak = v;
    SetDirty();

#ifdef _DEBUG
    //auto txt = mText;
    //txt.mFGColor = COLOR_GREEN;
    //WDL_String fpss;
    //fpss.SetFormatted(16, "fps %d \ndrop %1.3f", (int)fps, pDrop);
    //float h, w;
    //BasicTextMeasure(fpss.Get(), h, w);
    //auto dtr = valR;
    //dtr.T = dtr.B - h * txt.mSize - 30.0f;
    //graphics.DrawTextA(txt, fpss.Get(),dtr);
#endif

    }