/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
 */

#pragma once

/**
 * @file
 * @ingroup Controls
 * @copydoc ISpectrogramControl
 */

#include <string>
#include <fstream>
#include <streambuf>
#include <sstream>

#include "INanoVGShaderControl.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

template <int MAX_FFT_SIZE = 4096>
class ISpectrogramControl : public INanoVGShaderControl
{
public:
  using TDataPacket = std::array<float, MAX_FFT_SIZE>;
  
  ISpectrogramControl(const IRECT& bounds, int numRows = 128,
                      EDirection direction = EDirection::Horizontal)
  : INanoVGShaderControl(bounds)
  , mNumRows(numRows)
  , mDirection(direction)
  , mNeedUpdateTexture(true)
  , mTextureBufWriteIndex(0)
  {
    SetVertexShaderStr(
#if defined IGRAPHICS_GL2
    R"(
      attribute vec4 apos;
      attribute vec2 atexcoord;
      varying vec2 texcoord;
      void main() {
        texcoord = atexcoord;
        gl_Position = apos;
      }
    )"
#elif defined IGRAPHICS_GL3
#elif defined IGRAPHICS_METAL
#endif
    );
     
#if defined IGRAPHICS_GL2
    const char *shader = R"(
      uniform sampler2D texid;
      uniform float texorigin;
      uniform int dir;
      varying vec2 texcoord;

      vec4 colormap(float x);

      void main() {
        vec2 tc;
        if (dir == 0)
        {
          float t = texcoord.y + texorigin;
          if (t > 1.0) t -= 1.0;
          tc = vec2(texcoord.x, t);
        }
        else
        {
          float t = texcoord.x + texorigin;
          if (t > 1.0) t -= 1.0;
          tc = vec2(texcoord.y, t);
        }
        vec4 col = texture2D(texid, tc);
        float x = col.r;
        gl_FragColor = colormap(x);
      }
    
    // MATLAB_jet.frag
    float colormap_red(float x) {
        if (x < 0.7) {
            return 4.0 * x - 1.5;
        } else {
            return -4.0 * x + 4.5;
        }
    }

    float colormap_green(float x) {
        if (x < 0.5) {
            return 4.0 * x - 0.5;
        } else {
            return -4.0 * x + 3.5;
        }
    }

    float colormap_blue(float x) {
        if (x < 0.3) {
           return 4.0 * x + 0.5;
        } else {
           return -4.0 * x + 2.5;
        }
    }

    vec4 colormap(float x) {
        float r = clamp(colormap_red(x), 0.0, 1.0);
        float g = clamp(colormap_green(x), 0.0, 1.0);
        float b = clamp(colormap_blue(x), 0.0, 1.0);
        return vec4(r, g, b, 1.0);
    }
    
    )";
#elif defined IGRAPHICS_GL3
#endif

    SetFragmentShaderStr(shader);
    
    CheckSpectrogramDataSize();

    SetFreqRange(20.f, 20000.f, 44100.f);
    SetDBRange(-90.f, 0.f);
  }
  
  ~ISpectrogramControl()
  {
  }
  
  void OnMsgFromDelegate(int msgTag, int dataSize, const void* pData) override
  {
    if (!IsDisabled() && msgTag == ISender<>::kUpdateMessage)
    { 
      IByteStream stream(pData, dataSize);
      
      int pos = 0;
      ISenderData<1, TDataPacket> d;
      pos = stream.Get(&d, pos);
      
      for (auto c = d.chanOffset; c < (d.chanOffset + d.nChans); c++)
      {
        ScaleSpectrogramData(c, d.vals[c].data(), mFFTSize / 2);
        UpdateSpectrogram(c, d.vals[c].data());
      }
      
      SetDirty(false);
    }
  }

  void SetFFTSize(int fftSize)
  {
    assert(fftSize > 0);
    assert(fftSize <= MAX_FFT_SIZE);
    mFFTSize = fftSize;

    CheckSpectrogramDataSize();
  }
  
  void DrawToFBO(int w, int h) override
  {
    if (mNeedUpdateTexture)
    {
      CreateTexture();
      mNeedUpdateTexture = false;
    }
    
    static const float posAndTexCoords[] = {
    //     x,     y,    u,   v
          -1.0f, -1.0f, 0.0, 0.0,
           1.0f, -1.0f, 1.0, 0.0,
           1.0f,  1.0f, 1.0, 1.0,
          -1.0f,  1.0f, 0.0, 1.0,
    };
    
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(posAndTexCoords),
                 posAndTexCoords, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 16, 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 16, (void*)8);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mTexId);
    GLint texLoc = glGetUniformLocation(mProgram, "texid");
    glUniform1i(texLoc, 0);

    GLint xOriginLog = glGetUniformLocation(mProgram, "texorigin");
    glUniform1f(xOriginLog,
                ((float)mTextureBufWriteIndex)/(mNumRows - 1));

    GLint dirLoc = glGetUniformLocation(mProgram, "dir");
    glUniform1i(dirLoc, (int)mDirection);
    
    glDrawArrays(GL_QUADS, 0, 4);
  }
  
  void PreDraw(IGraphics& g) override
  {
    g.DrawDottedRect(COLOR_BLACK, mRECT);
//    g.FillRect(COLOR_GREEN, mRECT);
  }
  
  void PostDraw(IGraphics& g) override
  {
    g.DrawText(mText, "Spectrogram", mRECT);
  }

  void SetFreqRange(float freqLo, float freqHi, float sampleRate)
  {
    mLogXLo = std::log(freqLo / (sampleRate / 2.f));
    mLogXHi = std::log(freqHi / (sampleRate / 2.f));
  }
  
  void SetDBRange(float dbLo, float dbHi)
  {
    mLogYLo = std::log(std::pow(10.f, dbLo / 10.0));
    mLogYHi = std::log(std::pow(10.f, dbHi / 10.0));
  }
  
  //void OnMouseDown(float x, float y, const IMouseMod& mod) override {}
  //void OnMouseUp(float x, float y, const IMouseMod& mod) override {}
  //void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override {}

protected:
  virtual void ScaleSpectrogramData(int ch, float* powerSpectrum, int size)
  {
    // Scale X
    if (mTmpBuf.GetSize() != size)
      mTmpBuf.Resize(size);
    memcpy(mTmpBuf.Get(), powerSpectrum, size*sizeof(float));

    float xRecip = 1.f / static_cast<float>(size);
    // Start at 1, don't use the DC bin
    for (int i = 1; i < size; i++)
    {
      // Possible improvement: interpolation with the 2 closest values
      float x = CalcXNormInv(i * xRecip);
      powerSpectrum[(int)(x * size)] = mTmpBuf.Get()[i];
    }
    
    // Scale Y
    for (int i = 0; i < size; i++)
      powerSpectrum[i] = CalcYNorm(powerSpectrum[i]);
  }
    
  void UpdateSpectrogram(int ch, const float* powerSpectrum)
  {
    // tmp
    if (ch != 0)
      return;
    
    int rowIndex = mTextureBufWriteIndex++ % mNumRows;
    memcpy(&mTextureBuf.Get()[rowIndex*(mFFTSize / 2)],
           powerSpectrum,
           (mFFTSize / 2)*sizeof(float));
    
    mNeedUpdateTexture = true;
  }

  int mFFTSize = 1024;
  int mNumRows = 128;

  float mLogXLo;
  float mLogXHi;
  float mLogYLo;
  float mLogYHi;
  
private:
  void CheckSpectrogramDataSize()
  {
    if (mTextureBuf.GetSize() != (mFFTSize / 2)*mNumRows)
    {
      mTextureBuf.Resize((mFFTSize / 2)*mNumRows);
      memset(mTextureBuf.Get(), 0,
             (mFFTSize / 2)*mNumRows*sizeof(float));
    }
  }
  
  void CreateTexture()
  {    
    glGenTextures(1, &mTexId);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mTexId);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Float format texture, 1 component
#ifdef  __APPLE__ // Apple
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, (mFFTSize / 2), mNumRows,
                 0, GL_RED, GL_FLOAT, mTextureBuf.Get());
#else // Windows or Linux
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (mFFTSize / 2), mNumRows,
                 0, GL_RED, GL_FLOAT, mTextureBuf.Get());
#endif
    
    glGenerateMipmap(GL_TEXTURE_2D);
  }

  float CalcXNormInv(float x) const { return (std::exp(mLogXLo + x/(mLogXHi - mLogXLo))); }
  float CalcYNorm(float y) const { return (std::log(y) - mLogYLo) / (mLogYHi - mLogYLo); }

  // Raw buffer used to generate the texture
  // (kind of circular buffer)
  WDL_TypedBuf<float> mTextureBuf;
  int mTextureBufWriteIndex;
  
  EDirection mDirection;

  bool mNeedUpdateTexture;
  unsigned int mTexId;

  WDL_TypedBuf<float> mTmpBuf;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
