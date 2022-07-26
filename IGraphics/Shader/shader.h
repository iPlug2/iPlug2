/*
    NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
    The widget drawing code is based on the NanoVG demo application
    by Mikko Mononen.

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/

/**
 * \file nanogui/shader.h
 *
 * \brief Defines abstractions for shaders that work with OpenGL,
 * OpenGL ES, and Metal.
 */

#pragma once

#include "object.h"
#include "traits.h"
#include <unordered_map>

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

class Shader : public Object {
public:
    /// The type of geometry that should be rendered
    enum class PrimitiveType {
        Point,
        Line,
        LineStrip,
        Triangle,
        TriangleStrip
    };

    /// Alpha blending mode
    enum class BlendMode {
        None,
        AlphaBlend // alpha * new_color + (1 - alpha) * old_color
    };

    /**
     * \brief Initialize the shader using the specified source strings.
     *
     * \param render_pass
     *     RenderPass object encoding targets to which color, depth,
     *     and stencil information will be rendered.
     *
     * \param name
     *     A name identifying this shader
     *
     * \param vertex_shader
     *     The source of the vertex shader as a string.
     *
     * \param fragment_shader
     *     The source of the fragment shader as a string.
     */
    Shader(RenderPass *render_pass,
           const std::string &name,
           const std::string &vertex_shader,
           const std::string &fragment_shader,
           BlendMode blend_mode = BlendMode::None);

    /// Return the render pass associated with this shader
    RenderPass *render_pass() { return m_render_pass; }

    /// Return the name of this shader
    const std::string &name() const { return m_name; }

    /// Return the blending mode of this shader
    BlendMode blend_mode() const { return m_blend_mode; }

    /**
     * \brief Upload a buffer (e.g. vertex positions) that will be associated
     * with a named shader parameter.
     *
     * Note that this function should be used both for 'varying' and 'uniform'
     * data---the implementation takes care of routing the data to the right
     * endpoint. Matrices should be specified in column-major order.
     *
     * The buffer will be replaced if it is already present.
     */
    void set_buffer(const std::string &name, VariableType type, size_t ndim,
                    const size_t *shape, const void *data);

    void set_buffer(const std::string &name, VariableType type,
                    std::initializer_list<size_t> shape, const void *data) {
        set_buffer(name, type, shape.end() - shape.begin(), shape.begin(), data);
    }

    /**
     * \brief Upload a uniform variable (e.g. a vector or matrix) that will be
     * associated with a named shader parameter.
     */
    template <typename Array> void set_uniform(const std::string &name,
                                               const Array &value) {
        size_t shape[3] = { 1, 1, 1 };
        size_t ndim = (size_t) -1;
        const void *data;
        VariableType vtype = VariableType::Invalid;

        if constexpr (std::is_scalar_v<Array>) {
            data = &value;
            ndim = 0;
            vtype = get_type<Array>();
        } else if constexpr (is_nanogui_array_v<Array>) {
            data = value.v;
            ndim = 1;
            shape[0] = Array::Size;
            vtype = get_type<typename Array::Value>();
        } else if constexpr (is_nanogui_matrix_v<Array>) {
            data = value.m;
            ndim = 2;
            shape[0] = Array::Size;
            shape[1] = Array::Size;
            vtype = get_type<typename Array::Value>();
        // } else if constexpr (is_enoki_array_v<Array>) {
        //     if constexpr (Array::Depth == 1) {
        //         shape[0] = value.size();
        //         ndim = 1;
        //     } else if constexpr (Array::Depth == 2) {
        //         shape[0] = value.size();
        //         shape[1] = value[0].size();
        //         ndim = 2;
        //     } else if constexpr (Array::Depth == 3) {
        //         shape[0] = value.size();
        //         shape[1] = value[0].size();
        //         shape[2] = value[0][0].size();
        //         ndim = 3;
        //     }
        //     data = value.data();
        //     vtype = get_type<typename Array::Scalar>();
        }

        if (ndim == (size_t) -1)
            throw std::runtime_error("Shader::set_uniform(): invalid input array dimension!");

        set_buffer(name, vtype, ndim, shape, data);
    }

    /**
     * \brief Associate a texture with a named shader parameter
     *
     * The association will be replaced if it is already present.
     */
    void set_texture(const std::string &name, Texture *texture);

    /**
     * \brief Begin drawing using this shader
     *
     * Note that any updates to 'uniform' and 'varying' shader parameters
     * *must* occur prior to this method call.
     *
     * The Python bindings also include extra \c __enter__ and \c __exit__
     * aliases so that the shader can be activated via Pythons 'with'
     * statement.
     */
    void begin();

    /// End drawing using this shader
    void end();

    /**
     * \brief Render geometry arrays, either directly or
     * using an index array.
     *
     * \param primitive_type
     *     What type of geometry should be rendered?
     *
     * \param offset
     *     First index to render. Must be a multiple of 2 or 3 for lines and
     *     triangles, respectively (unless specified using strips).
     *
     * \param offset
     *     Number of indices to render. Must be a multiple of 2 or 3 for lines
     *     and triangles, respectively (unless specified using strips).
     *
     * \param indexed
     *     Render indexed geometry? In this case, an
     *     \c uint32_t valued buffer with name \c indices
     *     must have been uploaded using \ref set().
     */
    void draw_array(PrimitiveType primitive_type,
                    size_t offset, size_t count,
                    bool indexed = false);

#if defined(NANOGUI_USE_OPENGL) || defined(NANOGUI_USE_GLES)
    uint32_t shader_handle() const { return m_shader_handle; }
#elif defined(NANOGUI_USE_METAL)
    void *pipeline_state() const { return m_pipeline_state; }
#endif

#if defined(NANOGUI_USE_OPENGL)
    uint32_t vertex_array_handle() const { return m_vertex_array_handle; }
#endif

protected:
    enum BufferType {
        Unknown = 0,
        VertexBuffer,
        VertexTexture,
        VertexSampler,
        FragmentBuffer,
        FragmentTexture,
        FragmentSampler,
        UniformBuffer,
        IndexBuffer,
    };

    struct Buffer {
        void *buffer = nullptr;
        BufferType type = Unknown;
        VariableType dtype = VariableType::Invalid;
        int index = 0;
        size_t ndim = 0;
        size_t shape[3] { 0, 0, 0 };
        size_t size = 0;
        bool dirty = false;

        std::string to_string() const;
    };

    /// Release all resources
    virtual ~Shader();

protected:
    RenderPass* m_render_pass;
    std::string m_name;
    std::unordered_map<std::string, Buffer> m_buffers;
    BlendMode m_blend_mode;

    #if defined(NANOGUI_USE_OPENGL) || defined(NANOGUI_USE_GLES)
        uint32_t m_shader_handle = 0;
    #  if defined(NANOGUI_USE_OPENGL)
        uint32_t m_vertex_array_handle = 0;
        bool m_uses_point_size = false;
    #  endif
    #elif defined(NANOGUI_USE_METAL)
        void *m_pipeline_state;
    #endif
};

/// Access binary data stored in nanogui_resources.cpp
#define NANOGUI_RESOURCE_STRING(name) std::string(name, name + name##_size)

/// Access a shader stored in nanogui_resources.cpp
#if defined(NANOGUI_USE_OPENGL)
#  define NANOGUI_SHADER(name) NANOGUI_RESOURCE_STRING(name##_gl)
#elif defined(NANOGUI_USE_GLES)
#  define NANOGUI_SHADER(name) NANOGUI_RESOURCE_STRING(name##_gles)
#elif defined(NANOGUI_USE_METAL)
#  define NANOGUI_SHADER(name) NANOGUI_RESOURCE_STRING(name##_metallib)
#endif


END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
