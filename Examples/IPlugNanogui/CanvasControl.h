/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#ifndef IGRAPHICS_NANOVG
#error This IControl only works with the NanoVG graphics backend
#endif

/**
 * @file
 * @ingroup Controls
 * @copydoc NanoGUIShaderControl
 */

#include "IControl.h"

#include "shader.h"
#include "vector.h"
#include "texture.h"
#include "renderpass.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

bool m_float_buffer = false;

Texture::PixelFormat pixel_format() {
#if defined(NANOGUI_USE_METAL)
    if (!m_float_buffer)
        return Texture::PixelFormat::BGRA;
#endif
    return Texture::PixelFormat::RGBA;
}

Texture::ComponentFormat component_format() {
    if (m_float_buffer)
        return Texture::ComponentFormat::Float16;
    else
        return Texture::ComponentFormat::UInt8;
}

/* Base class for NanoVG controls that draw to a framebuffer with OpenGL or Metal */
class CanvasControl : public IControl
{
public:
  CanvasControl(const IRECT& bounds, uint8_t samples = 4,
                bool has_depth_buffer = true, bool has_stencil_buffer = false,
                bool clear = true)
  : IControl(bounds)
  {
    m_size = Vector2i(bounds.W(), bounds.H());

    Object *color_texture = nullptr,
           *depth_texture = nullptr;

    if (has_stencil_buffer && !has_depth_buffer)
        throw std::runtime_error("Canvas::Canvas(): has_stencil implies has_depth!");

      color_texture = new Texture(
          pixel_format(),
          component_format(),
          m_size,
          Texture::InterpolationMode::Bilinear,
          Texture::InterpolationMode::Bilinear,
          Texture::WrapMode::ClampToEdge,
          samples,
          Texture::TextureFlags::RenderTarget
      );

#if defined(NANOGUI_USE_METAL)
      Texture *color_texture_resolved = nullptr;

      if (samples > 1) {
          color_texture_resolved = new Texture(
              pixel_format(),
              component_format(),
              m_size,
              Texture::InterpolationMode::Bilinear,
              Texture::InterpolationMode::Bilinear,
              Texture::WrapMode::ClampToEdge,
              1,
              Texture::TextureFlags::RenderTarget
          );

          m_render_pass_resolved = new RenderPass(
              { color_texture_resolved }
          );
      }
#endif

      depth_texture = new Texture(
          has_stencil_buffer ? Texture::PixelFormat::DepthStencil
                             : Texture::PixelFormat::Depth,
          Texture::ComponentFormat::Float32,
          m_size,
          Texture::InterpolationMode::Bilinear,
          Texture::InterpolationMode::Bilinear,
          Texture::WrapMode::ClampToEdge,
          samples,
          Texture::TextureFlags::RenderTarget
      );

    m_render_pass = new RenderPass(
        { color_texture },
        depth_texture,
        has_stencil_buffer ? depth_texture : nullptr,
#if defined(NANOGUI_USE_METAL)
        m_render_pass_resolved,
#else
        nullptr,
#endif
        clear
    );
  }
  
  virtual ~CanvasControl()
  {
    CleanUp();
  }
  
  void CleanUp()
  {
//    m_shader = nullptr;
  }
  
  void OnAttached() override
  {
  }
  
  void Draw(IGraphics& g) override
  {
    g.DrawDottedRect(COLOR_BLACK, mRECT);
    g.FillRect(mMouseIsOver ? COLOR_TRANSLUCENT : COLOR_TRANSPARENT, mRECT);

    auto* pCtx = static_cast<NVGcontext*>(g.GetDrawContext());
    
    auto w = mRECT.W() * g.GetTotalScale();
    auto h = mRECT.H() * g.GetTotalScale();
      

    
//    scr->nvg_flush();

    Vector2i fbsize = m_size;
    Vector2i offset = Vector2i(0,0);

#if defined(NANOGUI_USE_OPENGL) || defined(NANOGUI_USE_GLES)
    offset = Vector2i(offset.x(), scr->size().y() - offset.y() - m_size.y());
#endif

    fbsize = Vector2i(Vector2f(fbsize) * pixel_ratio);
    offset = Vector2i(Vector2f(offset) * pixel_ratio);

    m_render_pass->resize(fbsize);
#if defined(NANOGUI_USE_METAL)
    if (m_render_pass_resolved)
      m_render_pass_resolved->resize(fbsize);
#endif

    m_render_pass->begin();
    draw_contents();
    m_render_pass->end();

    RenderPass *rp = m_render_pass;
#if defined(NANOGUI_USE_METAL)
    if (m_render_pass_resolved)
        rp = m_render_pass_resolved;
#endif
    rp->blit_to(Vector2i(0, 0), fbsize, nullptr, offset); // TODO: actually BLIT!
  }
  
  virtual void draw_contents() = 0;
  
  void OnResize() override
  {
  }
  
  void OnRescale() override
  {
  }
  
  RenderPass *render_pass() { return m_render_pass; }
  
private:
  NVGframebuffer* mFBO = nullptr;
  
#ifdef IGRAPHICS_GL
  int mInitialFBO = 0;
#endif
  bool invalidateFBO = true;
  
  Vector2i m_size;
  float pixel_ratio = 1.0f;
  ref<RenderPass> m_render_pass;
#if defined(NANOGUI_USE_METAL)
  ref<RenderPass> m_render_pass_resolved;
#endif
};

class MyCanvas : public CanvasControl
{
public:
  MyCanvas(const IRECT& bounds)
  : CanvasControl(bounds, 2)
  {
    m_shader = new Shader(
        render_pass(),

        // An identifying name
        "a_simple_shader",

#if defined(NANOGUI_USE_OPENGL)
        // Vertex shader
        R"(#version 330
        uniform mat4 mvp;
        in vec3 position;
        in vec3 color;
        out vec4 frag_color;
        void main() {
            frag_color = vec4(color, 1.0);
            gl_Position = mvp * vec4(position, 1.0);
        })",

        // Fragment shader
        R"(#version 330
        out vec4 color;
        in vec4 frag_color;
        void main() {
            color = frag_color;
        })"
#elif defined(NANOGUI_USE_GLES)
        // Vertex shader
        R"(precision highp float;
        uniform mat4 mvp;
        attribute vec3 position;
        attribute vec3 color;
        varying vec4 frag_color;
        void main() {
            frag_color = vec4(color, 1.0);
            gl_Position = mvp * vec4(position, 1.0);
        })",

        // Fragment shader
        R"(precision highp float;
        varying vec4 frag_color;
        void main() {
            gl_FragColor = frag_color;
        })"
#elif defined(NANOGUI_USE_METAL)
        // Vertex shader
        R"(using namespace metal;

        struct VertexOut {
            float4 position [[position]];
            float4 color;
        };

        vertex VertexOut vertex_main(const device packed_float3 *position,
                                     const device packed_float3 *color,
                                     constant float4x4 &mvp,
                                     uint id [[vertex_id]]) {
            VertexOut vert;
            vert.position = mvp * float4(position[id], 1.f);
            vert.color = float4(color[id], 1.f);
            return vert;
        })",

        /* Fragment shader */
        R"(using namespace metal;

        struct VertexOut {
            float4 position [[position]];
            float4 color;
        };

        fragment float4 fragment_main(VertexOut vert [[stage_in]]) {
            return vert.color;
        })"
#endif
    );

    uint32_t indices[3*12] = {
        3, 2, 6, 6, 7, 3,
        4, 5, 1, 1, 0, 4,
        4, 0, 3, 3, 7, 4,
        1, 5, 6, 6, 2, 1,
        0, 1, 2, 2, 3, 0,
        7, 6, 5, 5, 4, 7
    };

    float positions[3*8] = {
        -1.f, 1.f, 1.f, -1.f, -1.f, 1.f,
        1.f, -1.f, 1.f, 1.f, 1.f, 1.f,
        -1.f, 1.f, -1.f, -1.f, -1.f, -1.f,
        1.f, -1.f, -1.f, 1.f, 1.f, -1.f
    };

    float colors[3*8] = {
        0, 1, 1, 0, 0, 1,
        1, 0, 1, 1, 1, 1,
        0, 1, 0, 0, 0, 0,
        1, 0, 0, 1, 1, 0
    };

    m_shader->set_buffer("indices", VariableType::UInt32, {3*12}, indices);
    m_shader->set_buffer("position", VariableType::Float32, {8, 3}, positions);
    m_shader->set_buffer("color", VariableType::Float32, {8, 3}, colors);
  }

  void draw_contents() override {
    
      Matrix4f view = Matrix4f::look_at(
          Vector3f(0, -2, -10),
          Vector3f(0, 0, 0),
          Vector3f(0, 1, 0)
      );

      Matrix4f model = Matrix4f::rotate(
          Vector3f(0, 1, 0),
                                        0.0f
//          (float) glfwGetTime()
      );

      Matrix4f model2 = Matrix4f::rotate(
          Vector3f(1, 0, 0),
          m_rotation
      );

      Matrix4f proj = Matrix4f::perspective(
          float(25 * iplug::PI / 180),
          0.1f,
          20.f,
          mRECT.W() / mRECT.H()
      );

      Matrix4f mvp = proj * view * model * model2;

      m_shader->set_uniform("mvp", mvp);

      // Draw 12 triangles starting at index 0
      m_shader->begin();
      m_shader->draw_array(Shader::PrimitiveType::Triangle, 0, 12*3, true);
      m_shader->end();
  }
  
  void set_rotation(float rotation) {
      m_rotation = rotation;
  }

public:
  ref<Shader> m_shader;
  float m_rotation;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE


