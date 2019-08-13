#pragma once

#include "fft.h"
#include <sstream>
#include "IControl.h"
#include "IPlugQueue.h"
#include "IPlugStructs.h"

/*

IPlug spectrum analyzer FFT example
(c) Matthew Witmer 2015, 2019
<http://lvcaudio.com>

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. If you make it better, please let me know
2. If there are problems, please let me know


*************************************************************
Simple IPlug audio effect that shows how to implement a graphical spectrum analyzer in conjunction with fft.h/c included with WDL.  This file contains
an FFT class (Spect_FFT), a graphical display class for the FFT (gFFTAnalyzer), and a class to calculate and display frequency indicators (gFFTFreqDraw).

Notes:
   - fft.h can be used for double or floats.  The default setting is to use float.  This is defined within fft.h.
   - right-clicking on the display allows for changing colors and parameters of the analyzer.
   - The EQ lines need a pointer to the gFFTAnalyzer to pass the right-click.  The gFFTAnalyzer needs a pointer to the Spect_FFT to allow for setting changes
			with the right-click.

****************************************

To Do:
- more window functions for FFT
- I am sure there are places to optimize
- Better interpolation method than linear for drawing (see comments within Draw())
- Add phase display and or Left/Right
- better peak display and scaling the peak timing with the FFT bin sizes
- Mouse over to show frequency at certain point, pitch/key, etc.
- Other cool things that I can't think of ???

*/


//WDL_FFT_REAL is typedef to either float or double, depending what is defined in fft.h
static const WDL_FFT_REAL pi2 = 2. * pi;
static const WDL_FFT_REAL pi4 = 4. * pi;

//a few helper functions
// convert from one linear range to another
template <typename T> T RangeConvert(T OldV, T OldMax, T NewMax, T OldMin = (T)0., T NewMin = (T)0.)
{
    if (OldMax == OldMin) return (T)0.;
    else return (((OldV - OldMin) * (NewMax - NewMin)) / (OldMax - OldMin)) + NewMin;
}

//linear interpolation tends to work better than fancier interpolation.  See also comment in Draw about using Cairo or something else that can handle floating x/y coordinates
template <typename T> T LinInterp(T x1, T x2, T y1, T y2, T x)
{
	return (y1 + (y2 - y1) * (x - x1) / (x2 - x1));
}

template <typename T>T sqr(T x)
{
	return(x * x);
}

//a class for calculating FFT
class Spect_FFT
{
public:
    enum eWindowType
    {
		win_Hann = 0,
		win_Blackman,
		win_BlackmanHarris,
		win_Hamming,
		win_Flattop,
		win_KaiserBessel,
		win_BlackmanNuttall,
		win_Rectangular
    };

    Spect_FFT( const int initialsize, const int initialframes)
    {
		fftSize = initialsize;
		frameSize = initialframes;
		windowType = win_Hann;
		vWindowFx.reserve(32768 + 1);
		vFFTBuffer.reserve(10);
		vFFTOuputs.reserve(10);
		SetBufferSize();
		WDL_fft_init();
    }

    ~Spect_FFT()
    {
    }

    void ClearBuffers()
    {
        SetBufferSize();
    }

	//higher frames increase the smoothness of the signal at high FFT sizes.  The cost is that more FFT calculations are made for each sample
	void SetFrameSize(const int x)
  {
		frameSize = x;
		SetBufferSize();
	}

	int GetFrameSize() { return frameSize; }
  int GetWindowType() { return windowType; }
  int GetSize() { return fftSize; }

    void SetFFTSize(const int x)
    {
		if (x == 16 || x == 32 || x == 64 || x == 128 || x == 256 || x == 512 || x == 1024 || x == 2048 || x == 4096 || x == 8192 || x == 16384 || x == 32768) {
			fftSize = x;
			SetBufferSize();
		}
    }

	void SetWindowType(const int type)
  {
		windowType = type;
		CalculateWindowFx(windowType);
		SetBufferSize();
	}

  bool Process(WDL_FFT_REAL &in)
  {
    bool r = false;
    int currentPos = 0;

    std::vector<sFFTBuffer>::iterator it, end(vFFTBuffer.end());
    for (it = vFFTBuffer.begin(); it != end; ++it) {
      currentPos = it->currentPosition;
      it->vReIm[currentPos].re = in * vWindowFx[currentPos];
      it->vReIm[currentPos].im = 0.;
      it->currentPosition++;
      if (it->currentPosition >= fftSize) {
        it->currentPosition = 0;
        Permute(it);
        r = true;
      }
    }
    return r;
  }

  WDL_FFT_REAL GetOutput(const int pos)
  {
    if (pos >= 0 && pos < vFFTOuputs[0].size()) {
      WDL_FFT_REAL out = 0.;
      for (int i = 0; i < frameSize; i++) {
        out += vFFTOuputs[i][pos];
      }
      return out / (WDL_FFT_REAL)frameSize;
    }
    else {
      // return 0 for first bin and any bin that is larger than expected
      return 0.;
    }
  }

protected:

  struct sFFTBuffer
  {
    int currentPosition;
    std::vector<WDL_FFT_COMPLEX>WDL_FIXALIGN vReIm;

  }WDL_FIXALIGN;

    void Permute(std::vector<sFFTBuffer>::iterator it)
    {
		WDL_FFT_REAL *buf = new WDL_FFT_REAL[fftSize];
		// gainAdj is used to adjust the gain of the signal on the display.  WDL_real_fft claims that this should be 0.5/len.  
		//  I added some additional adjustment for the fft size.  I am sure there is a better mathematical way, but this feels about right . 
		const WDL_FFT_REAL gainAdj = 1. / fftSize * (log2(fftSize) / 11); 
		for (int i = 0; i < fftSize; ++i) buf[i] = it->vReIm[i].re * gainAdj;
		WDL_real_fft(buf, fftSize, false);
    int vIndex = it - vFFTBuffer.begin();
		int halfFFT = fftSize / 2;
		int sortIDX = 0;
		int x = 0;
		while (x < halfFFT - 1)
		{
			sortIDX = WDL_fft_permute(halfFFT, x) * 2;
			vFFTOuputs[vIndex][x] = std::sqrt(sqr(buf[sortIDX]) + sqr(buf[sortIDX + 1]));
			x++;
		}
		vFFTOuputs[vIndex][halfFFT] = buf[1];
		vFFTOuputs[vIndex][0] = 0.;
		delete[] buf;
	}

	// values from https://holometer.fnal.gov/GH_FFT.pdf
	// Page 29 (ROV %)
	WDL_FFT_REAL GetOverlapfromWindowType(const int wt)
  {
		switch (wt)
		{
		case  win_Hann: return 0.5; break;
		case win_Blackman: return 0.5; break;
		case win_BlackmanHarris: return 0.661; break;
		case win_Hamming: return 0.5; break;
		case win_Flattop: return 0.655; break;
		case win_KaiserBessel: return 0.619; break;
		case win_BlackmanNuttall: return 0.661; break;
		case win_Rectangular: return 0.5; break;
		default: return 0.5;
			break;
		}
	}

	//sets each FFT frame to correct index based on overlap and number of frames
	void SetOverlapPosition(const WDL_FFT_REAL percentOverlap = 0.5)
  {
		assert(percentOverlap > 0.0001 && percentOverlap < 1.0);
		if (frameSize > 1) {
			double overStart = fftSize / (double)frameSize * (0.5 / percentOverlap);
			for (int i = 1; i < frameSize; i++) {
				vFFTBuffer[i].currentPosition = (int)(overStart * i + 0.5);
			}
		}
		vFFTBuffer[0].currentPosition = 0;
	}


	void SetBufferSize()
  {
		if (vWindowFx.size() != fftSize) vWindowFx.resize(fftSize);
		CalculateWindowFx();
		if (vFFTBuffer.size() != frameSize) {
			vFFTBuffer.resize(frameSize);
			vFFTOuputs.resize(frameSize);
		}
		std::vector<sFFTBuffer>::iterator it, end(vFFTBuffer.end());
		for (it = vFFTBuffer.begin(); it != end; ++it) {
			if (it->vReIm.size() != fftSize) it->vReIm.resize(fftSize);
			for (int i = 0; i < fftSize; i++) {
				it->vReIm[i].im = 0.0;
				it->vReIm[i].re = 0.0;
			}
			it->currentPosition = 0;
		}

		for (int i = 0; i < frameSize; ++i) {
			vFFTOuputs[i].resize(fftSize / 2 + 1);
			for (int x = 0; x < fftSize / 2 + 1; ++x) vFFTOuputs[i][x] = 0.;
		}

		SetOverlapPosition(GetOverlapfromWindowType(GetWindowType()));
	}

	void CalculateWindowFx(const int type = -1)
  {
		int wType = type;
		if (wType == -1) wType = windowType;
		for (int i = 0; i < fftSize; i++) {
			const double x = pi2 * i / fftSize;
			if (wType == win_Hann)  vWindowFx[i] = 0.5 * (1. - std::cos(x));
			else if (wType == win_Blackman)  vWindowFx[i] = 0.42659071 - 0.49656062 * std::cos(x) + 0.07684867 *std::cos(2. * x);
			else if (wType == win_BlackmanHarris)  vWindowFx[i] = 0.35875 - 0.48829 * std::cos(x) + 0.14128*std::cos(2. * x) - 0.01168 * std::cos(3. * x);
			else if (wType == win_Hamming) vWindowFx[i] = 0.5434782609 - 0.4565217391 * std::cos(x);
			else if (wType == win_Flattop) vWindowFx[i] = 0.21557895 - 0.41663158 * std::cos(x) + 0.277263158 * std::cos(2. * x) - 0.083578947 * std::cos(3. * x) + 0.006947368 * std::cos(4. * x);
			else if (wType == win_KaiserBessel) vWindowFx[i] = 0.402 - 0.498 * std::cos(pi2 * (double)(i + 1) / fftSize) + 0.098 * std::cos(pi4 * (double)(i + 1) / fftSize) + 0.001 * std::cos(6.0 * pi * (double)(i + 1) / fftSize);
			else if (wType == win_BlackmanNuttall) vWindowFx[i] = 0.3635819 - 0.4891775 * std::cos(pi2*i / fftSize) + 0.1365995 * std::cos(pi4*i / fftSize) - 0.0106411 * std::cos(6.0 * pi * i / fftSize);
			else vWindowFx[i] = 1.; //rectangular
		}
	}

	std::vector<WDL_FFT_REAL>vWindowFx;
	std::vector<sFFTBuffer>vFFTBuffer;
	std::vector<std::vector<WDL_FFT_REAL>>vFFTOuputs;
	int fftSize, frameSize, windowType;
};

template<int QUEUE_SIZE = 4096>
class gFFTAnalyzer : public IControl
    {
    public:
      static constexpr int kUpdateMessage = 0;

      /** Used on the DSP side in order to queue sample values and transfer data to low priority thread. */
      class Sender {

      public:
        Sender(int controlTag)
          : mControlTag(controlTag) {
        }

        void ProcessBlock(sample** input, int nFrames, int nChans) {

          double inVal = 0.;

          for (int s = 0; s < nFrames; s++) {
            inVal = 0.;
            for (int c = 0; c < nChans; c++) {
              inVal += input[c][s];
            }
            mQueue.Push(inVal / nChans);
          }
        }

        // this must be called on the main thread - typically in MyPlugin::OnIdle()
        void TransmitData(IEditorDelegate& dlg)
        {
          while (mQueue.ElementsAvailable())
          {
            WDL_FFT_REAL d;
            mQueue.Pop(d);
            dlg.SendControlMsgFromDelegate(mControlTag, kUpdateMessage, sizeof(WDL_FFT_REAL), (void*)&d);
          }
        }

      private:
        int mControlTag;
        IPlugQueue<WDL_FFT_REAL> mQueue{ QUEUE_SIZE };
      };

      gFFTAnalyzer(const IRECT &pR, const int &initialSize = 4096)
        : IControl(pR, -1)
      {
        
        mColorPeak = COLOR_BLACK;
        mColorFill = COLOR_RED;
        fftBins = (WDL_FFT_REAL)initialSize;
        dBFloor = -120.;
        val = 0.0;
        minFreq = 1.;
        maxFreq = 44100. / 0.5;
        sampleRate = 44100.;
        OctaveGain = 1.;
        value.resize(32768 / 2 + 1);
        showFill = showGradient = true;
        pFFT = new Spect_FFT(initialSize, 2);
      }

      ~gFFTAnalyzer()
      {
        menu.RemoveEmptySubmenus();
        delete pFFT;
      }

        void OnMsgFromDelegate(int messageTag, int dataSize, const void* pData) override
        {
          IByteStream stream(pData, dataSize);

          int pos = 0;
          WDL_FFT_REAL data;
          while (pos < stream.Size())
          {
            pos = stream.Get(&data, pos);
            pFFT->Process(data);
          }

          SetDirty(false);
        }


		void SetFFTSize(const int x)
    {
			if (x == 16 || x == 32 || x == 64 || x == 128 || x == 256 || x == 512 || x == 1024 || x == 2048 || x == 4096 || x == 8192 || x == 16384 || x == 32768) {
				fftBins = (WDL_FFT_REAL)x; 
				ResetValuestoFloor();
			}
		}

    Spect_FFT * getFFT() { return pFFT; }
    WDL_FFT_REAL GetMinFreq() { return minFreq; }
    WDL_FFT_REAL GetMaxFreq() { return maxFreq; }

    void SetColors(IColor fill, IColor peakLine)
    {
      SetColorFill(fill);
      SetColorPeak(peakLine);
      mDirty = true;
    }

    void SetColorFill(IColor fill)   {  mColorFill = fill;  }
    void SetColorPeak(IColor peakLine) { mColorPeak = peakLine;  }
    void SetFillOn(const bool &on) { showFill = on; }
    void SetGradientOn(const bool &on) { showGradient = on; }
    IColor GetColorFill() { return mColorFill; }
    IColor GetColorPeak() { return mColorPeak; }

    void OnResize() override
    {
      if (iVal.size() < mRECT.W()) {
        iVal.resize(mRECT.W());
        iPeak.resize(mRECT.W());
      }
      ResetValuestoFloor();
      decayValue = 0.50;
      peakdecayValue = 0.975;
    }

    void OnRescale() override
    {
      OnResize();
    }

    void SetdbFloor(const WDL_FFT_REAL f)
    {
      dBFloor = f;
      ResetValuestoFloor();
      mDirty = true;
    }

    void SetMinFreq(const WDL_FFT_REAL f)
    {
      minFreq = Clip(f, (WDL_FFT_REAL)1, maxFreq);
      ResetValuestoFloor();
      mDirty = true;
    }

    void SetMaxFreq(const WDL_FFT_REAL f)
    {
      maxFreq = Clip(f, minFreq, sampleRate * (WDL_FFT_REAL)0.5);
      ResetValuestoFloor();
      mDirty = true;
    }

    // per-octave gain (e.g., +3 dB makes pink noise appear flat).  Most analyzers use between +3 and +4.5 dB/octave compensation
    void SetOctaveGain(const WDL_FFT_REAL g, const bool isDB)
    {
      if (isDB) OctaveGain = DBToAmp(g);
      else OctaveGain = g;
      mDirty = true;
    }

		void GetFFTData()
    {
			int stop = (int)(fftBins / 2. + 1. + 0.5);
			for (int c = 0; c < stop; ++c) {
				value[c] = pFFT->GetOutput(c);
			}
		}

    void OnInit() override
    {
      menu.AddItem("Change FFT Fill Color");//0
      menu.AddItem("Change FFT Peak Color");//1
      menu.AddItem("Fill");//2
      menu.AddItem("Gradient");//3
      menu.AddItem("FFT Size...", &fftsizemenu, 111);//4
      fftsizemenu.AddItem("256");//0
      fftsizemenu.AddItem("512");//1
      fftsizemenu.AddItem("1024");//2
      fftsizemenu.AddItem("2048");//3
      fftsizemenu.AddItem("4096");//4
      fftsizemenu.AddItem("8192");//5
      fftsizemenu.AddItem("16384");//6
      fftsizemenu.AddItem("32768");//7
      menu.AddItem("FFT Window...", &fftwindowmenu, 222);//5
      fftwindowmenu.AddItem("Hann");//0
      fftwindowmenu.AddItem("Blackman"); //1
      fftwindowmenu.AddItem("Blackman-Harris");//2
      fftwindowmenu.AddItem("Hamming");//3
      fftwindowmenu.AddItem("Flattop");//4
      fftwindowmenu.AddItem("Kaiser-Bessel");//5
      fftwindowmenu.AddItem("Blackman-Nuttall");//6
      fftwindowmenu.AddItem("Rectangular");//7
      menu.AddItem("FFT Frames...", &fftframes, 333);//6
      fftframes.AddItem("2");
      fftframes.AddItem("3");
      fftframes.AddItem("4");
      fftframes.AddItem("5");
      fftframes.AddItem("6");
      fftframes.AddItem("7");
      fftframes.AddItem("8");
      fftframes.AddItem("9");
      fftframes.AddItem("10");
    }

    void Draw(IGraphics& g) override
    {
      GetFFTData();
      int width = mRECT.W();
      WDL_FFT_REAL x, y, yPeak;
      WDL_FFT_REAL xPrev = mRECT.L;
      WDL_FFT_REAL yPrev = mRECT.MH();
      WDL_FFT_REAL yPrevPeak = yPrev;
      int startBin = 1;
      const WDL_FFT_REAL mF = maxFreq / minFreq;
      /*  This should probably be revised.  Originally, this loop was meant to look up the frequency at each (int) column from left to right.
      With code like Cairo, floating point numbers can be used to draw.  Revising this to just step through the frequency of the FFT bins
      might be faster and look a little better.
      */
      for (int f = 0; f < width; ++f) {
        const WDL_FFT_REAL FreqForBin = minFreq * std::pow(mF, (WDL_FFT_REAL)f / (WDL_FFT_REAL)(width - 1));
        bool isSearch = false;
        while (!isSearch)
        {
          const WDL_FFT_REAL b2 = Clip((WDL_FFT_REAL)(startBin * sampleRate / fftBins), (WDL_FFT_REAL)0., sampleRate);
          const WDL_FFT_REAL b3 = Clip((WDL_FFT_REAL)((startBin + 1) * sampleRate / fftBins), (WDL_FFT_REAL)0., sampleRate);
          if (FreqForBin < b2) {
            iVal[0] = iVal[1];
            iPeak[0] = iPeak[1];
            --startBin;
            isSearch = true;
          }
          else if (b2 <= FreqForBin && b3 >= FreqForBin) {
            iVal[f] = LinInterp(b2, b3, value[startBin], value[startBin + 1], FreqForBin) * decayValue;
            iPeak[f] = std::max(iVal[f], iPeak[f] * peakdecayValue);
            --startBin;
            isSearch = true;
          }
          else if (b2 > FreqForBin) {
            startBin--;
            if (startBin < 1) {
              startBin = 1;
              isSearch = true;
            }
          }
          else {
            ++startBin;
            if (startBin >= value.size()) isSearch = true;
          }
        }
      }

      for (int b = 0; b < width; ++b)
      {
        x = b + mRECT.L;
        const WDL_FFT_REAL binFreq = minFreq * std::pow(mF, (WDL_FFT_REAL)b / (WDL_FFT_REAL)(width));
        const WDL_FFT_REAL oct = std::log10(binFreq / minFreq) / 0.30102999;
        const WDL_FFT_REAL gainO = std::pow(OctaveGain, oct);

        WDL_FFT_REAL dbv = AmpToDB(iVal[b] * gainO);
        y = RangeConvert(Clip(dbv, dBFloor, (WDL_FFT_REAL)0.), (WDL_FFT_REAL)0., (WDL_FFT_REAL)mRECT.T, dBFloor, (WDL_FFT_REAL)mRECT.B);
        WDL_FFT_REAL pdv = AmpToDB(iPeak[b] * gainO);
        yPeak = RangeConvert(Clip(pdv, dBFloor, (WDL_FFT_REAL)0.), (WDL_FFT_REAL)0., (WDL_FFT_REAL)mRECT.T, dBFloor, (WDL_FFT_REAL)mRECT.B);

        if (showFill) {
          if (showGradient) {
            //IBlend adds fill that varies alpha based on volume of each column
            IBlend m;
            m.mWeight = RangeConvert((float)y, (float)mRECT.T, 1.f, (float)mRECT.B, 0.1f);
            g.DrawVerticalLine(mColorFill, x, mRECT.B, y, &m);
          }
          else {
            g.DrawVerticalLine(mColorFill, x, mRECT.B, y);
          }
        }

        if (b == 0) {
          yPrev = y;
          yPrevPeak = yPeak;
        }
      
        g.DrawLine(mColorPeak, xPrev, yPrevPeak, x, yPeak, 0, 1.5);
        xPrev = x;
        yPrev = y;
        yPrevPeak = yPeak;
      }
      // Call SetDirty() to force gFFTlyzer to get all of the FFT data on the next draw
      SetDirty(false);
    }

    void ResetValuestoFloor()
    {
      const WDL_FFT_REAL ampF = 0.;
      for (int f = 0; f < mRECT.W(); f++) {
        iVal[f] = ampF;
        iPeak[f] = ampF;
      }
      for (std::vector<WDL_FFT_REAL>::iterator it = value.begin(); it != value.end(); ++it)
      {
        *it = ampF;
      }
    }

		void SetSampleRate(const WDL_FFT_REAL sr)
    {
			sampleRate = sr;
			SetMaxFreq(maxFreq);
			SetMinFreq(minFreq);
		}

    void OnMouseDown(float x, float y, const IMouseMod& mod) override
    {
      if (mod.R) {
        menu.CheckItem(2, showFill);
        menu.CheckItem(3, showGradient);
        int frames = pFFT->GetFrameSize();
        fftframes.CheckItemAlone(frames - 2);

        int fftcurrentsize = pFFT->GetSize();
        int check = -1;
        switch (fftcurrentsize)
        {
        case 256: check = 0; break;
        case 512: check = 1; break;
        case 1024: check = 2; break;
        case 2048: check = 3; break;
        case 4096: check = 4; break;
        case 8192: check = 5; break;
        case 16384: check = 6; break;
        case 32768: check = 7; break;
        default:  break;
        }
        fftsizemenu.CheckItemAlone(check);

        int fftcurrentwindow = pFFT->GetWindowType();
        int winT = -1;
        switch (fftcurrentwindow)
        {
        case Spect_FFT::win_Hann: winT = 0; break;
        case Spect_FFT::win_Blackman: winT = 1; break;
        case Spect_FFT::win_BlackmanHarris: winT = 2; break;
        case Spect_FFT::win_Hamming: winT = 3; break;
        case Spect_FFT::win_Flattop: winT = 4; break;
        case Spect_FFT::win_KaiserBessel: winT = 5; break;
        case Spect_FFT::win_BlackmanNuttall: winT = 6; break;
        case Spect_FFT::win_Rectangular: winT = 7; break;
        default:  break;
        }
        fftwindowmenu.CheckItemAlone(winT);
        
        GetUI()->CreatePopupMenu(*this, menu, x, y);
      }
    }

    void OnPopupMenuSelection(IPopupMenu* pSelectedMenu, int valIdx) override
    {
      int item = menu.GetChosenItemIdx();
      int fftsizepick = fftsizemenu.GetChosenItemIdx();
      int fftwindow = fftwindowmenu.GetChosenItemIdx();
      int fftframenumber = fftframes.GetChosenItemIdx();

      if (item >= 0) {
        switch (item)
        {
        case 0: {
            IColor pC = mColorFill;
            GetUI()->PromptForColor(pC, "FFT Fill Color", [&](const IColor& clr) {
                mColorFill = clr;
            });
        }
                break;
        case 1: {
          IColor pC = mColorPeak;
            GetUI()->PromptForColor(pC, "FFT Fill Color", [&](const IColor& clr) {
                mColorPeak = clr;
            });
        }
                break;
        case 2: 
          showFill = !showFill;
          break;
        case 3: 
          showGradient = !showGradient;
          break;
        default:
          break;
        }
      }
      else if (fftsizepick >= 0) {
        int size = 0;
        switch (fftsizepick)
        {
        case 0: size = 256; break;
        case 1: size = 512; break;
        case 2: size = 1024; break;
        case 3: size = 2048; break;
        case 4: size = 4096; break;
        case 5: size = 8192; break;
        case 6: size = 16384; break;
        case 7: size = 32768; break;
        default:
          break;
        }
        if (size > 0) {
          fftBins = size;
          this->SetFFTSize(size);
          pFFT->SetFFTSize(size);
        }
      }
      else if (fftwindow >= 0) {
        int win = 0;
        switch (fftwindow)
        {
        case 0: win = Spect_FFT::win_Hann; break;
        case 1: win = Spect_FFT::win_Blackman; break;
        case 2: win = Spect_FFT::win_BlackmanHarris; break;
        case 3: win = Spect_FFT::win_Hamming; break;
        case 4: win = Spect_FFT::win_Flattop; break;
        case 5: win = Spect_FFT::win_KaiserBessel; break;
        case 6: win = Spect_FFT::win_BlackmanNuttall; break;
        case 7: win = Spect_FFT::win_Rectangular; break;
        default:  break;
        }
        pFFT->SetWindowType(win);
      }
      else if (fftframenumber >= 0) {
        pFFT->SetFrameSize(fftframenumber + 2);
      }

      menu.SetChosenItemIdx(-1);
      fftsizemenu.SetChosenItemIdx(-1);
      fftwindowmenu.SetChosenItemIdx(-1);
      fftframes.SetChosenItemIdx(-1);


      SetDirty(false);
    }

    private:
		std::vector <WDL_FFT_REAL> value;
		std::vector<WDL_FFT_REAL>iVal;
		std::vector<WDL_FFT_REAL>iPeak;
		WDL_FFT_REAL val, fftBins, sampleRate;
		WDL_FFT_REAL decayValue, peakdecayValue;
		IColor mColorPeak, mColorFill;
		bool showFill, showGradient;
		WDL_FFT_REAL minFreq, maxFreq;
    WDL_FFT_REAL dBFloor;
		WDL_FFT_REAL OctaveGain;
     IPopupMenu fftsizemenu, fftwindowmenu, fftframes, menu;
     Spect_FFT* pFFT;
    };


    //Class for drawing vertical frequency line indicators with text values
    class gFFTFreqDraw : public IControl
    {
    public:
        gFFTFreqDraw(const IRECT pR,  const IText &fonttxt)
        : IControl (pR)
        {
            txt = fonttxt;
            mColor = txt.mFGColor;
            minFreq = 20.;
            maxFreq = 20000.;
            //space is used calculate text size.  It is only an estimate.  Using something like cairo_text_extents() is a better option
            space = txt.mSize + 4;
            CalcSpacing();
            mIgnoreMouse = true;
        }

        ~gFFTFreqDraw() {}

        void SetMinFreq(const double f)
        { 
            minFreq = Clip(f, 1., maxFreq);
            CalcSpacing();
        }

        void SetMaxFreq(const double f)
        { 
          maxFreq = std::max(f, minFreq);
            CalcSpacing();
        }
  
        void Draw(IGraphics& g) override
        {
          for (int i = 0; i < HLineDraw.size(); i++) {
            g.DrawVerticalLine(mColor, HLineDraw[i], mRECT.B - 20, mRECT.T);
          }

          for (int i = 0; i < FreqTxtDraw.size(); i++) {
            char hz[7];
            std::strcpy(hz, FreqTxtDraw[i].freq.c_str());
            IRECT box(FreqTxtDraw[i].pos - (space / 2), mRECT.B - 15, FreqTxtDraw[i].pos + (space / 2), mRECT.B);
            g.DrawText(txt, hz, box);
          }
        }

    private:
		//this works a lot nicer with something like Cairo where you can get the pixel width of the displayed text (cairo_text_extents()) to determine spacing
      void CalcSpacing()
      {
        int prevTxt = space / 2;
        FreqTxtDraw.resize(0);
        HLineDraw.resize(0);
        bool addToList = false;
        txtLocation toAdd;
        std::ostringstream s;
        int minFreq10 = minFreq;
        if (minFreq10 % 10 != 0) minFreq10 = (10 - (int)(minFreq) % 10) + (int)(minFreq);
        for (int i = minFreq10; i < maxFreq; i += 10) {
          const int bin = (int)((std::log10((double)i / (double)minFreq) / (std::log10((double)maxFreq / (double)minFreq))) * (double)(mRECT.W()));
          if (i < 80 && i % 10 == 0) {
            if (bin > prevTxt + space && bin < mRECT.W() - space / 2) {
              addToList = true;
              s << i;
            }
          }
          else if (i >= 80 && i < 799 && i % 100 == 0) {
            if (bin > prevTxt + space && bin < mRECT.W() - space / 2) {
              addToList = true;
              s << i;
            }
          }
          else if (i >= 800 && i < 14999 && i % 1000 == 0) {
            if (bin > prevTxt + space && bin < mRECT.W() - space / 2) {
              addToList = true;
              s << (int)(i * 0.001);
              s << 'k';
            }
          }
          else if (i % 5000 == 0) {
            if (bin > prevTxt + space && bin < mRECT.W() - space / 2) {
              addToList = true;
              s << (int)(i * 0.001);
              s << 'k';
            }
          }

          if (addToList) {
            addToList = false;
            toAdd.pos = bin + mRECT.L;
            const std::string freq_as_string(s.str());
            toAdd.freq = freq_as_string;
            FreqTxtDraw.push_back(toAdd);
            HLineDraw.push_back(toAdd.pos);
            s.clear();
            s.str("");
            prevTxt = bin;
          }
        }
      }

        struct txtLocation
        {
            int pos;
            std::string WDL_FIXALIGN freq;
        }WDL_FIXALIGN;

        std::vector<int>HLineDraw;
        std::vector<txtLocation>FreqTxtDraw;
        IColor mColor;
        IText txt;
        int space;
        double minFreq, maxFreq;
    };

