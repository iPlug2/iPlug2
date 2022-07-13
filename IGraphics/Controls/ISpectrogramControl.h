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
  
  ISpectrogramControl(const IRECT& bounds, EDirection direction = EDirection::Horizontal)
  : INanoVGShaderControl(bounds)
  , mDirection(direction)
  {}
  
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
      
      // TODO: convert incoming spectral data in d.vals[] to mSpectrogramData
      
      SetDirty(false);
    }
  }
  
  void DrawToFBO(int w, int h) override
  {
    static const float posAndColor[] = {
    //     x,     y,    r,     g,  b
          -0.6f, -0.6f, 1.0, 0.0, 0.0,
           0.6f, -0.6f, 0.0, 1.0, 0.0,
           0.f,   0.6f, 0.0, 0.0, 1.0,
    };
    
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(posAndColor), posAndColor, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 20, 0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 20, (void*)8);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glDrawArrays(GL_TRIANGLES, 0, 3);
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
private:
//  std::vector<uint8_t> mSpectrogramData;
  EDirection mDirection;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
