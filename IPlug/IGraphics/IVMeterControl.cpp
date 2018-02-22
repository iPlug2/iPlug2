#include "IVMeterControl.h"

const IColor IVMeterControl::DEFAULT_BG_COLOR = IColor(255, 70, 70, 70);
const IColor IVMeterControl::DEFAULT_RAW_COLOR = IColor(255, 200, 200, 200);
const IColor IVMeterControl::DEFAULT_RMS_COLOR = IColor(200, 70, 150, 80);
const IColor IVMeterControl::DEFAULT_PK_COLOR = IColor(255, 240, 60, 60);
const IColor IVMeterControl::DEFAULT_FR_COLOR = DEFAULT_BG_COLOR;


/*
IVMeterControl::IVMeterControl(IDelegate & dlg, IRECT rect, int paramIdx, double * inputBuf)
  {
  }
*/

void IVMeterControl::Draw(IGraphics& graphics) {
  double fps = graphics.FPS();
  auto spf = 1.0 / fps;
  auto sampPerDraw = mSampleRate / fps;
  auto shadowColor = IColor(60, 0, 0, 0);

  for (auto ch = 0; ch != NumChannels(); ++ch) {
    auto v = RawValue(ch); // always >= 0.0
    *RawValuePtr(ch) = 0.0;

    auto meterRect = GetMeterRect(ch);

    // background and shadows
    auto bgRect = meterRect;
    if (DrawPeakRect(ch)) bgRect.T -= mPeakRectHeight;
    graphics.FillRect(GetColor(mBg), bgRect);
    if (mDrawShadows)
      DrawInnerShadowForRect(bgRect, shadowColor, graphics);

    // raw value rect
    auto rawR = meterRect;
    rawR.T = GetVCoordFromValInMeterRect(ch, v, meterRect);
    if (v >= MinDisplayVal(ch))
      graphics.FillRect(GetColor(mRaw), rawR);

    // memory rect
    // math
    auto p = GetPeakFromMemExp(ch);
    if (p < 0.0) p = 0.0;
    if (p < v || p == 0.0) {
      p = v;
      *MemPeakPtr(ch) = v;
      *MemExpPtr(ch) = 1.0;
      *PeakSampHeldPtr(ch) = 0;
      }
    else {
      if (PeakSampHeld(ch) >= 0.001 * DropMs(ch) * mSampleRate) {
        auto t = DropMs(ch);
        if (p > 0.0 && p < 1.0) t /= p; // low values should decay ~at the same rate
        *MemExpPtr(ch) *= GetInvExpForDrop(t, fps); // different decay character than a simple exp
        }
      else
        *PeakSampHeldPtr(ch) += (size_t) sampPerDraw;
      }
    // graphics
    if (DrawMemRect(ch) && p >= MinDisplayVal(ch) && DropMs(ch) > spf * 1000.0)
      // if drop time is shorter than the time between Draw() calls it's redundant
      {
      auto memR = meterRect;
      memR.T = GetVCoordFromValInMeterRect(ch, p, meterRect);
      memR.B = rawR.T;
      auto c = GetColor(mRaw);
      c.A /= 2;
      graphics.FillRect(c, memR);
      auto pc = LinearBlendColors(GetColor(mRaw), GetColor(mPeak), OverBlink(ch));
      if (p <= MaxDisplayVal(ch))
        graphics.DrawLine(pc, memR.L, memR.T, memR.R, memR.T);
      }

    // rms rect
    auto rms = 0.0;
    if (DrawRMS(ch)) {
      auto avg = RMSSum(ch) / RMSBufLen(ch);
      if (AESFix(ch))
        avg *= 2.0;
      rms = sqrt(avg);

      if (rms > MinDisplayVal(ch)) {
        auto rmsR = meterRect;
        rmsR.T = GetVCoordFromValInMeterRect(ch, rms, meterRect);
        graphics.FillRect(GetColor(mRms), rmsR);

        }
      }

    // peak rect
    if (v > MaxPeak(ch))
      *MaxPeakPtr(ch) = v;
    // graphics stuff
    IRECT pvtr;
    if (DrawPeakRect(ch)) {
      auto pR = meterRect;
      pR.T = meterRect.T - mPeakRectHeight;
      pR.B = meterRect.T;
      auto pc = LinearBlendColors(COLOR_TRANSPARENT, GetColor(mPeak), OverBlink(ch));
      pvtr = pR;
      graphics.FillRect(pc, pR);
      }
    // math
    if (!HoldingAPeak(ch))
      *OverBlinkPtr(ch) *= GetExpForDrop(1000.0 + 2.5 * DropMs(ch), fps);

    if (mDrawBorders)
      graphics.DrawRect(GetColor(mFr), bgRect);

    if (DrawPeakRect(ch) && DrawMaxPeak(ch)) // not to draw borders over the peak value
      {
       WDL_String tps;
        auto v = MaxPeak(ch);
        if (UnitsDB(ch)) {
          v = AmpToDB(v);
          if (v >= -300.0)
            tps.SetFormatted(8, PrecisionString(ch).Get(), v);
          else tps.Set("<-300");
          }
        else
          tps.SetFormatted(8, PrecisionString(ch).Get(), v);

        pvtr = ShiftRectBy(pvtr, 0.f, 1.0);
        if (mDrawShadows) {
          auto tt = mMarkText;
          tt.mFGColor = shadowColor;
          auto sr = ShiftRectBy(pvtr, 1.0, 1.0);
          graphics.DrawTextA(tt, tps.Get(), sr);
          }
        graphics.DrawTextA(mMarkText, tps.Get(), pvtr);
      }

    if (DrawChanName(ch)) // can be inside the loop because names are below the meters
      {
      auto cnr = meterRect;
      auto h = mText.mSize;
      cnr.B += h;
      cnr.T = cnr.B - h;
      cnr = ShiftRectBy(cnr, ChanNameHOffset(ch));
      graphics.DrawTextA(mText, ChanNamePtr(ch)->Get(), cnr);
      }


#ifdef _DEBUG

    auto trms = mText;
    trms.mFGColor = COLOR_BLACK;
    WDL_String ps;
    auto vt = rms;
    if (UnitsDB(ch)) vt = AmpToDB(vt);
    ps.SetFormatted(16, "rms\n%4.2f", vt);
    auto dtr = rawR;
    dtr.T = dtr.B - 2.0f * trms.mSize - 10.0f;
    graphics.DrawTextA(trms, ps.Get(), dtr);

    auto tl = GetVCoordFromValInMeterRect(ch, OverThresh(ch), meterRect);
    graphics.DrawLine(COLOR_ORANGE, meterRect.L, tl, meterRect.R + 0.3f * DistToTheNextM(ch), tl);
#endif
    }

  DrawMarks(graphics); // draw outside because meters can be drawn over the marks

#ifdef _DEBUG
   auto txtfps = mText;
    txtfps.mFGColor = COLOR_GREEN;
    WDL_String fpss;
    fpss.SetFormatted(8, "fps\n%d", (int) fps);
    auto dtr = mRECT;
    dtr.T = dtr.B - 2.0f * txtfps.mSize - 10.0f;
    graphics.DrawTextA(txtfps, fpss.Get(), dtr);

    //graphics.DrawRect(COLOR_BLUE, mRECT);
#endif

  SetDirty();
  }