/*
    NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
    The widget drawing code is based on the NanoVG demo application
    by Mikko Mononen.

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/

/**
 * \file nanogui/texture.h
 *
 * \brief Defines an abstraction for textures that works with
 * OpenGL, OpenGL ES, and Metal.
 */

#pragma once

#include "object.h"
#include "vector.h"
#include "traits.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

class Texture : public Object {
public:
    /// Overall format of the texture (e.g. luminance-only or RGBA)
    enum class PixelFormat : uint8_t {
        /// Single-channel bitmap
        R,

        /// Two-channel bitmap
        RA,

        /// RGB bitmap
        RGB,

        /// RGB bitmap + alpha channel
        RGBA,

        /// BGR bitmap
        BGR,

        /// BGR bitmap + alpha channel
        BGRA,

        /// Depth map
        Depth,

        /// Combined depth + stencil map
        DepthStencil
    };

    /// Number format of pixel components
    enum class ComponentFormat : uint8_t {
        // Signed and unsigned integer formats
        UInt8  = (uint8_t) VariableType::UInt8,
        Int8   = (uint8_t) VariableType::Int8,
        UInt16 = (uint16_t) VariableType::UInt16,
        Int16  = (uint16_t) VariableType::Int16,
        UInt32 = (uint32_t) VariableType::UInt32,
        Int32  = (uint32_t) VariableType::Int32,

        // Floating point formats
        Float16  = (uint16_t) VariableType::Float16,
        Float32  = (uint32_t) VariableType::Float32
    };

    /// Texture interpolation mode
    enum class InterpolationMode : uint8_t {
        /// Nearest neighbor interpolation
        Nearest,

        /// Bilinear ineterpolation
        Bilinear,

        /// Trilinear interpolation (using MIP mapping)
        Trilinear
    };

    /// How should out-of-bounds texture evaluations be handled?
    enum class WrapMode : uint8_t {
        /// Clamp evaluations to the edge of the texture
        ClampToEdge,

        /// Repeat the texture
        Repeat,

        /// Repeat, but flip the texture after crossing the boundary
        MirrorRepeat,
    };

    /// How will the texture be used? (Must specify at least one)
    enum TextureFlags {
        /// Texture to be read in shaders
        ShaderRead = 0x01,

        /// Target framebuffer for rendering
        RenderTarget = 0x02
    };

    /**
     * \brief Allocate memory for a texture with the given configuration
     *
     * \note
     *   Certain combinations of pixel and component formats may not be
     *   natively supported by the hardware. In this case, \ref init() chooses
     *   a similar supported configuration that can subsequently be queried
     *   using \ref pixel_format() and \ref component_format().
     *   Some caution must be exercised in this case, since \ref upload() will
     *   need to provide the data in a different storage format.
     */
    Texture(PixelFormat pixel_format,
            ComponentFormat component_format,
            const Vector2i &size,
            InterpolationMode min_interpolation_mode = InterpolationMode::Bilinear,
            InterpolationMode mag_interpolation_mode = InterpolationMode::Bilinear,
            WrapMode wrap_mode = WrapMode::ClampToEdge,
            uint8_t samples = 1,
            uint8_t flags = (uint8_t) TextureFlags::ShaderRead,
            bool mipmap_manual = false);

    /// Load an image from the given file using stb-image
    Texture(const std::string &filename,
            InterpolationMode min_interpolation_mode = InterpolationMode::Bilinear,
            InterpolationMode mag_interpolation_mode = InterpolationMode::Bilinear,
            WrapMode wrap_mode                       = WrapMode::ClampToEdge);

    /// Return the pixel format
    PixelFormat pixel_format() const { return m_pixel_format; }

    /// Return the component format
    ComponentFormat component_format() const { return m_component_format; }

    /// Return the interpolation mode for minimization
    InterpolationMode min_interpolation_mode() const { return m_min_interpolation_mode; }

    /// Return the interpolation mode for minimization
    InterpolationMode mag_interpolation_mode() const { return m_mag_interpolation_mode; }

    /// Return the wrap mode
    WrapMode wrap_mode() const { return m_wrap_mode; }

    /// Return the number of samples (MSAA)
    uint8_t samples() const { return m_samples; }

    /// Return a combination of flags (from \ref Texture::TextureFlags)
    uint8_t flags() const { return m_flags; }

    /// Return the size of this texture
    const Vector2i &size() const { return m_size; }

    /// Return the number of bytes consumed per pixel of this texture
    size_t bytes_per_pixel() const;

    /// Return the number of channels of this texture
    size_t channels() const;

    /// Upload packed pixel data from the CPU to the GPU
    void upload(const uint8_t *data);

    /// Upload packed pixel data to a rectangular sub-region of the texture from the CPU to the GPU
    void upload_sub_region(const uint8_t *data, const Vector2i& origin, const Vector2i& size);

    /// Download packed pixel data from the GPU to the CPU
    void download(uint8_t *data);

    /// Resize the texture (discards the current contents)
    void resize(const Vector2i &size);

    /// Generates the mipmap. Done automatically upon upload if manual mipmapping is disabled.
    void generate_mipmap();

#if defined(NANOGUI_USE_OPENGL) || defined(NANOGUI_USE_GLES)
    uint32_t texture_handle() const { return m_texture_handle; }
    uint32_t renderbuffer_handle() const { return m_renderbuffer_handle; }
#elif defined(NANOGUI_USE_METAL)
    void *texture_handle() const { return m_texture_handle; }
    void *sampler_state_handle() const { return m_sampler_state_handle; }
#endif

protected:
    /// Initialize the texture handle
    void init();

    /// Release all resources
    virtual ~Texture();

protected:
    PixelFormat m_pixel_format;
    ComponentFormat m_component_format;
    InterpolationMode m_min_interpolation_mode;
    InterpolationMode m_mag_interpolation_mode;
    WrapMode m_wrap_mode;
    uint8_t m_samples;
    uint8_t m_flags;
    Vector2i m_size;
    bool m_mipmap_manual;

    #if defined(NANOGUI_USE_OPENGL) || defined(NANOGUI_USE_GLES)
        uint32_t m_texture_handle = 0;
        uint32_t m_renderbuffer_handle = 0;
    #elif defined(NANOGUI_USE_METAL)
        void *m_texture_handle = nullptr;
        void *m_sampler_state_handle = nullptr;
    #endif
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
