#pragma once

#include "INanoVGShaderControl.h"

class TestNanoVGShaderControl : public INanoVGShaderControl
{
public:
  TestNanoVGShaderControl(const IRECT& bounds)
  : INanoVGShaderControl(bounds)
  {
//    SetVertexShaderStr(
//#if defined IGRAPHICS_GL
//    R"(
//      attribute vec2 apos;
//      
//      void main() {
//        gl_Position = vec4(apos.x, apos.y, 0.0, 1.0);
//      }
//    )"
//#elif defined IGRAPHICS_METAL
//    R"(
//    )"
//#endif
//    );
//
//    SetFragmentShaderStr(
//#if defined IGRAPHICS_GL
//    R"(
//      varying vec4 color;
//      void main() {
//        gl_FragColor = color;
//      }
//    )"
//#elif defined IGRAPHICS_METAL
//    R"(
//    )"
//#endif
//    );
  }

  void DrawToFBO(int w, int h) override
  {
    // corner00x, corner00y, corner00u, corner00v,
    // corner10x, corner10y, corner10u, corner10v,
    // corner11x, corner11y, corner11u, corner11v,
    // corner01x, corner01y, corner01u, corner01v
    float shaderData[24];

    constexpr float coords[4][2] =
    {
      { -1.0, -1.0 },
      {  1.0, -1.0 },
      {  1.0,  1.0 },
      { -1.0,  1.0 }
    };

    constexpr float texcoords[4][2] =
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

    // Draw
//    glPushMatrix();
//    glLoadIdentity();
    
#ifdef IGRAPHICS_GL
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, 4 * 6 * sizeof(float), shaderData, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*) (2 * sizeof(float)));

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    
    glDrawArrays(GL_TRIANGLES, 0, 6);

//    glPopMatrix();

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glDeleteBuffers(1, &vbo);
#endif
  }
};

