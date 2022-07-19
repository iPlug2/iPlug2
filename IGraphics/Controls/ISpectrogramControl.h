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
     
    SetFragmentShaderStr(
#if defined IGRAPHICS_GL2
    R"(
      uniform sampler2D texid;
      varying vec2 texcoord;
      void main() {
        vec4 col = texture2D(texid, texcoord);
        gl_FragColor = vec4(col.r, col.r, col.r, 1.0);
      }
    )"
#elif defined IGRAPHICS_GL3
#elif defined IGRAPHICS_GL3
#endif
   );

    mSpectrogramData.resize((mFFTSize / 2)*mNumRows);
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
        CalculateSpectrogram(c, d.vals[c].data(), (mFFTSize / 2));
      }
      
      SetDirty(false);
    }
  }

  void SetFFTSize(int fftSize)
  {
    assert(fftSize > 0);
    assert(fftSize <= MAX_FFT_SIZE);
    mFFTSize = fftSize;
    
    if (mSpectrogramData.size() != (mFFTSize / 2)*mNumRows)
      mSpectrogramData.resize((mFFTSize / 2)*mNumRows);
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
    GLint loc = glGetUniformLocation(mProgram, "texid");
    glUniform1i(loc, 0);
 
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

  //void OnMouseDown(float x, float y, const IMouseMod& mod) override {}
  //void OnMouseUp(float x, float y, const IMouseMod& mod) override {}
  //void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override {}

protected:
  void CalculateSpectrogram(int ch, const float* powerSpectrum, int size)
  {
    // Generate random texture
    for (int i = 0; i < mSpectrogramData.size(); i++)
    {
      float rnd = ((float)rand())/RAND_MAX;
      mSpectrogramData[i] = rnd;
    }

    mNeedUpdateTexture = true;
  }

  int mFFTSize = 2048;
  int mNumRows = 128;
  
private:
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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, mNumRows, (mFFTSize / 2),
                 0, GL_RED, GL_FLOAT, &mSpectrogramData[0]);
#else // Windows or Linux
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mNumRows, (mFFTSize / 2),
                 0, GL_RED, GL_FLOAT, &mSpectrogramData[0]);
#endif
    
    glGenerateMipmap(GL_TEXTURE_2D);
  }
  
  std::vector<float> mSpectrogramData;
  
  EDirection mDirection;

  bool mNeedUpdateTexture;
  unsigned int mTexId;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
