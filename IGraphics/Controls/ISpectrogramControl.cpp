/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include "ISpectrogramControl.h"

using namespace iplug;
using namespace igraphics;

#if defined IGRAPHICS_GL

void ISpectrogramControl::DrawToFBO(int w, int h)
{
  if (mScrollSpeed != mQueuedScrollSpeed)
  {
    mScrollSpeed = mQueuedScrollSpeed;
    CheckSpectrogramDataSize();
    mTransport.Reset();
    UpdateTimeAxis();
  }
  
  if (mNeedUpdateTexture)
  {
    CreateTexture();
    mNeedUpdateTexture = false;
  }

  if (mNeedSetupShader)
  {
    SetupFragmentShader();
    OnAttached();
    
    mNeedSetupShader = false;
  }
  
  // corner00x, corner00y, corner00u, corner00v,
  // corner10x, corner10y, corner10u, corner10v,
  // corner11x, corner11y, corner11u, corner11v,
  // corner01x, corner01y, corner01u, corner01v
  float shaderData[24];

  float coords[4][2] =
  {
    { -1.0, -1.0 },
    {  1.0, -1.0 },
    {  1.0,  1.0 },
    { -1.0,  1.0 }
  };

  float texcoords[4][2] =
  {
    { 0.0, 0.0 },
    { 1.0, 0.0 },
    { 1.0, 1.0 },
    { 0.0, 1.0 }
  };
      
  // First triangle
  
  // corner 00
  shaderData[0] = coords[0][0];
  shaderData[1] = coords[0][1];
  shaderData[2] = texcoords[0][0];
  shaderData[3] = texcoords[0][1];
    
  // corner 10
  shaderData[4] = coords[1][0];
  shaderData[5] = coords[1][1];
  shaderData[6] = texcoords[1][0];
  shaderData[7] = texcoords[1][1];
    
  // corner 11
  shaderData[8] = coords[2][0];
  shaderData[9] = coords[2][1];
  shaderData[10] = texcoords[2][0];
  shaderData[11] = texcoords[2][1];
    
  // Second triangle
      
  // corner 00
  shaderData[12] = coords[0][0];
  shaderData[13] = coords[0][1];
  shaderData[14] = texcoords[0][0];
  shaderData[15] = texcoords[0][1];
      
  // corner 11
  shaderData[16] = coords[2][0];
  shaderData[17] = coords[2][1];
  shaderData[18] = texcoords[2][0];
  shaderData[19] = texcoords[2][1];
      
  // corner 01
  shaderData[20] = coords[3][0];
  shaderData[21] = coords[3][1];
  shaderData[22] = texcoords[3][0];
  shaderData[23] = texcoords[3][1];
  
  GLuint vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, 4 * 6 * sizeof(float), shaderData, GL_STATIC_DRAW);
  
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 16, 0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 16, (void*)8);
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, mTexId);
  auto program = GetProgram();
  GLint texLoc = glGetUniformLocation(program, "texid");
  glUniform1i(texLoc, 0);

  GLint texOriginLoc = glGetUniformLocation(program, "texorigin");
  int numRows = NumRows();
  float texOrigin = ((float)(mTextureBufWriteIndex % numRows))/(numRows - 1);
  glUniform1f(texOriginLoc, texOrigin);

  GLint dirLoc = glGetUniformLocation(program, "dir");
  glUniform1i(dirLoc, (int) mDirection);

  GLint rangeLoc = glGetUniformLocation(program, "crange");
  glUniform1f(rangeLoc, mColorMapRange);

  GLint contrastLoc = glGetUniformLocation(program, "ccontrast");
  glUniform1f(contrastLoc, mColorMapContrast);
  
  float scrollOffset = 0.0;
  
  if (mScrollEnabled)
  {
    // Compute scroll offset for smooth scrolling
    int numRows = NumRows();
    double spectrogramDuration = mTransport.ComputeDataDuration(numRows*mFFTSize);
    // Choose an arbitrary drift threshold corresponding
    // to 8 rows of spectrogram
    double driftThreshold = mTransport.ComputeDataDuration(mMaxDriftRows*mFFTSize);
    double timeDiff = mTransport.ComputeTimeDifference(driftThreshold);
    scrollOffset = timeDiff/spectrogramDuration;
  }

  // Apply scroll offset
  GLint soffsetLoc = glGetUniformLocation(program, "soffset");
  glUniform1f(soffsetLoc, scrollOffset);

  // Scale a little the object, to hide the object bounds
  float dscale = ((float)numRows)/(numRows - mMaxDriftRows * 2.0);
  GLint dscaleLoc = glGetUniformLocation(program, "dscale");
  glUniform1f(dscaleLoc, dscale);
  
  glDrawArrays(GL_TRIANGLES, 0, 6);

  glDeleteBuffers(1, &vbo);
}

void ISpectrogramControl::CreateTexture()
{
  if (mTextureGenerated)
  {
    glDeleteTextures(1, &mTexId);
  }
  
  glGenTextures(1, &mTexId);
  mTextureGenerated = true;
  
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, mTexId);
  
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  int numRows = NumRows();
  // Float format texture, 1 component
#ifdef  __APPLE__ // Apple
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, mFFTSize / 2, numRows, 0, GL_RED, GL_FLOAT, mTextureBuf.data());
#else // Windows or Linux

#ifdef OS_LINUX
#define GL_RGBA32F 0x8814 // Hack: value taken from glad.h
#endif
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, mFFTSize / 2, numRows, 0, GL_RED, GL_FLOAT, mTextureBuf.data());
#endif
  glGenerateMipmap(GL_TEXTURE_2D);
}


#endif
