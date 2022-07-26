#include "shader.h"
#include "opengl.h"
#include "screen.h"
#include "texture.h"
#include "renderpass.h"
#include "opengl_check.h"

#if !defined(GL_HALF_FLOAT)
#  define GL_HALF_FLOAT 0x140B
#endif

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

static GLuint compile_gl_shader(GLenum type,
                                const std::string &name,
                                const std::string &shader_string) {
    if (shader_string.empty())
        return (GLuint) 0;

    GLuint id = glCreateShader(type);
    const char *shader_string_const = shader_string.c_str();
    CHK(glShaderSource(id, 1, &shader_string_const, nullptr));
    CHK(glCompileShader(id));

    GLint status;
    CHK(glGetShaderiv(id, GL_COMPILE_STATUS, &status));

    if (status != GL_TRUE) {
        const char *type_str = nullptr;
        if (type == GL_VERTEX_SHADER)
            type_str = "vertex shader";
        else if (type == GL_FRAGMENT_SHADER)
            type_str = "fragment shader";
#if defined(NANOGUI_USE_OPENGL)
        else if (type == GL_GEOMETRY_SHADER)
            type_str = "geometry shader";
#endif
        else
            type_str = "unknown shader type";

        char error_shader[4096];
        CHK(glGetShaderInfoLog(id, sizeof(error_shader), nullptr, error_shader));

        std::string msg = std::string("compile_gl_shader(): unable to compile ") +
                          type_str + " \"" + name + "\":\n\n" + error_shader;
        throw std::runtime_error(msg);
    }

    return id;
}

Shader::Shader(RenderPass *render_pass,
               const std::string &name,
               const std::string &vertex_shader,
               const std::string &fragment_shader,
               BlendMode blend_mode)
    : m_render_pass(render_pass), m_name(name), m_blend_mode(blend_mode), m_shader_handle(0) {

    GLuint vertex_shader_handle   = compile_gl_shader(GL_VERTEX_SHADER,   name, vertex_shader),
           fragment_shader_handle = compile_gl_shader(GL_FRAGMENT_SHADER, name, fragment_shader);

    m_shader_handle = glCreateProgram();

    GLint status;
    CHK(glAttachShader(m_shader_handle, vertex_shader_handle));
    CHK(glAttachShader(m_shader_handle, fragment_shader_handle));
    CHK(glLinkProgram(m_shader_handle));
    CHK(glDeleteShader(vertex_shader_handle));
    CHK(glDeleteShader(fragment_shader_handle));
    CHK(glGetProgramiv(m_shader_handle, GL_LINK_STATUS, &status));

    if (status != GL_TRUE) {
        char error_shader[4096];
        CHK(glGetProgramInfoLog(m_shader_handle, sizeof(error_shader), nullptr, error_shader));
        m_shader_handle = 0;
        throw std::runtime_error("Shader::Shader(name=\"" + name +
                                 "\"): unable to link shader!\n\n" + error_shader);
    }

    GLint attribute_count, uniform_count;
    CHK(glGetProgramiv(m_shader_handle, GL_ACTIVE_ATTRIBUTES, &attribute_count));
    CHK(glGetProgramiv(m_shader_handle, GL_ACTIVE_UNIFORMS, &uniform_count));

    auto register_buffer = [&](BufferType type, const std::string &name,
                               int index, GLenum gl_type) {
        if (m_buffers.find(name) != m_buffers.end())
            throw std::runtime_error(
                "Shader::Shader(): duplicate attribute/uniform name in shader code!");
        else if (name == "indices")
            throw std::runtime_error(
                "Shader::Shader(): argument name 'indices' is reserved!");

        Buffer &buf = m_buffers[name];
        for (int i = 0; i < 3; ++i)
            buf.shape[i] = 1;
        buf.ndim = 1;
        buf.index = index;
        buf.type = type;

        switch (gl_type) {
            case GL_FLOAT:
                buf.dtype = VariableType::Float32;
                buf.ndim = 0;
                break;

            case GL_FLOAT_VEC2:
                buf.dtype = VariableType::Float32;
                buf.shape[0] = 2;
                break;

            case GL_FLOAT_VEC3:
                buf.dtype = VariableType::Float32;
                buf.shape[0] = 3;
                break;

            case GL_FLOAT_VEC4:
                buf.dtype = VariableType::Float32;
                buf.shape[0] = 4;
                break;

            case GL_INT:
                buf.dtype = VariableType::Int32;
                buf.ndim = 0;
                break;

            case GL_INT_VEC2:
                buf.dtype = VariableType::Int32;
                buf.shape[0] = 2;
                break;

            case GL_INT_VEC3:
                buf.dtype = VariableType::Int32;
                buf.shape[0] = 3;
                break;

            case GL_INT_VEC4:
                buf.dtype = VariableType::Int32;
                buf.shape[0] = 4;
                break;

#if defined(NANOGUI_USE_OPENGL)
            case GL_UNSIGNED_INT:
                buf.dtype = VariableType::UInt32;
                buf.ndim = 0;
                break;

            case GL_UNSIGNED_INT_VEC2:
                buf.dtype = VariableType::UInt32;
                buf.shape[0] = 2;
                break;

            case GL_UNSIGNED_INT_VEC3:
                buf.dtype = VariableType::UInt32;
                buf.shape[0] = 3;
                break;

            case GL_UNSIGNED_INT_VEC4:
                buf.dtype = VariableType::UInt32;
                buf.shape[0] = 4;
                break;
#endif

            case GL_BOOL:
                buf.dtype = VariableType::Bool;
                buf.ndim = 0;
                break;

            case GL_BOOL_VEC2:
                buf.dtype = VariableType::Bool;
                buf.shape[0] = 2;
                break;

            case GL_BOOL_VEC3:
                buf.dtype = VariableType::Bool;
                buf.shape[0] = 3;
                break;

            case GL_BOOL_VEC4:
                buf.dtype = VariableType::Bool;
                buf.shape[0] = 4;
                break;

            case GL_FLOAT_MAT2:
                buf.dtype = VariableType::Float32;
                buf.shape[0] = buf.shape[1] = 2;
                buf.ndim = 2;
                break;

            case GL_FLOAT_MAT3:
                buf.dtype = VariableType::Float32;
                buf.shape[0] = buf.shape[1] = 3;
                buf.ndim = 2;
                break;

            case GL_FLOAT_MAT4:
                buf.dtype = VariableType::Float32;
                buf.shape[0] = buf.shape[1] = 4;
                buf.ndim = 2;
                break;

            case GL_SAMPLER_2D:
                buf.dtype = VariableType::Invalid;
                buf.ndim = 0;
                buf.type = FragmentTexture;
                break;

            default:
                throw std::runtime_error("Shader::Shader(): unsupported "
                                         "uniform/attribute type!");
        };

        if (type == VertexBuffer) {
            for (int i = (int) buf.ndim - 1; i >= 0; --i) {
                buf.shape[i + 1] = buf.shape[i];
            }
            buf.shape[0] = 0;
            buf.ndim++;
        }
    };

    for (int i = 0; i < attribute_count; ++i) {
        char attr_name[128];
        GLenum type = 0;
        GLint size = 0;
        CHK(glGetActiveAttrib(m_shader_handle, i, sizeof(attr_name), nullptr,
                              &size, &type, attr_name));
        GLint index = glGetAttribLocation(m_shader_handle, attr_name);
        register_buffer(VertexBuffer, attr_name, index, type);
    }

    for (int i = 0; i < uniform_count; ++i) {
        char uniform_name[128];
        GLenum type = 0;
        GLint size = 0;
        CHK(glGetActiveUniform(m_shader_handle, i, sizeof(uniform_name), nullptr,
                               &size, &type, uniform_name));
        GLint index = glGetUniformLocation(m_shader_handle, uniform_name);
        register_buffer(UniformBuffer, uniform_name, index, type);
    }

    Buffer &buf = m_buffers["indices"];
    buf.index = -1;
    buf.ndim = 1;
    buf.shape[0] = 0;
    buf.shape[1] = buf.shape[2] = 1;
    buf.type = IndexBuffer;
    buf.dtype = VariableType::UInt32;

#if defined(NANOGUI_USE_OPENGL)
    CHK(glGenVertexArrays(1, &m_vertex_array_handle));

    m_uses_point_size = vertex_shader.find("gl_PointSize") != std::string::npos;
#endif
}

Shader::~Shader() {
    CHK(glDeleteProgram(m_shader_handle));
#if defined(NANOGUI_USE_OPENGL)
    CHK(glDeleteVertexArrays(1, &m_vertex_array_handle));
#endif
}

void Shader::set_buffer(const std::string &name,
                        VariableType dtype,
                        size_t ndim,
                        const size_t *shape,
                        const void *data) {
    auto it = m_buffers.find(name);
    if (it == m_buffers.end())
        throw std::runtime_error(
            "Shader::set_buffer(): could not find argument named \"" + name + "\"");

    Buffer &buf = m_buffers[name];

    bool mismatch = ndim != buf.ndim || dtype != buf.dtype;
    for (size_t i = (buf.type == UniformBuffer ? 0 : 1); i < ndim; ++i)
        mismatch |= shape[i] != buf.shape[i];

    if (mismatch) {
        Buffer arg;
        arg.type = buf.type;
        arg.ndim = ndim;
        for (size_t i = 0; i < 3; ++i)
            arg.shape[i] = i < arg.ndim ? shape[i] : 1;
        arg.dtype = dtype;
        throw std::runtime_error("Buffer::set_buffer(\"" + name +
                                 "\"): shape/dtype mismatch: expected " + buf.to_string() +
                                 ", got " + arg.to_string());
    }

    size_t size = type_size(dtype);
    for (size_t i = 0; i < 3; ++i) {
        buf.shape[i] = i < ndim ? shape[i] : 1;
        size *= buf.shape[i];
    }

    if (buf.type == UniformBuffer) {
        if (buf.buffer && buf.size != size) {
            delete[] (uint8_t *) buf.buffer;
            buf.buffer = nullptr;
        }
        if (!buf.buffer)
            buf.buffer = new uint8_t[size];
        memcpy(buf.buffer, data, size);
    } else {
        GLuint buffer_id = 0;
        if (buf.buffer) {
            buffer_id = (GLuint) ((uintptr_t) buf.buffer);
        } else {
            CHK(glGenBuffers(1, &buffer_id));
            buf.buffer = (void *) ((uintptr_t) buffer_id);
        }
        GLenum buf_type = (name == "indices")
            ? GL_ELEMENT_ARRAY_BUFFER : GL_ARRAY_BUFFER;
        CHK(glBindBuffer(buf_type, buffer_id));
        CHK(glBufferData(buf_type, size, data, GL_DYNAMIC_DRAW));
    }

    buf.dtype = dtype;
    buf.ndim  = ndim;
    buf.size  = size;
    buf.dirty = true;
}

void Shader::set_texture(const std::string &name, Texture *texture) {
    auto it = m_buffers.find(name);
    if (it == m_buffers.end())
        throw std::runtime_error(
            "Shader::set_texture(): could not find argument named \"" + name + "\"");
    Buffer &buf = m_buffers[name];
    if (!(buf.type == VertexTexture || buf.type == FragmentTexture))
        throw std::runtime_error(
            "Shader::set_texture(): argument named \"" + name + "\" is not a texture!");

    buf.buffer = (void *) ((uintptr_t) texture->texture_handle());
    buf.dirty  = true;
}

void Shader::begin() {
    int texture_unit = 0;

    CHK(glUseProgram(m_shader_handle));

#if defined(NANOGUI_USE_OPENGL)
    CHK(glBindVertexArray(m_vertex_array_handle));
#endif

    for (auto &[key, buf] : m_buffers) {
        bool indices = key == "indices";
        if (!buf.buffer) {
            if (!indices)
                fprintf(stderr,
                        "Shader::begin(): shader \"%s\" has an unbound "
                        "argument \"%s\"!\n",
                        m_name.c_str(), key.c_str());
            continue;
        }

        GLuint buffer_id = (GLuint) ((uintptr_t) buf.buffer);
        GLenum gl_type = 0;

#if defined(NANOGUI_USE_OPENGL)
        if (!buf.dirty && buf.type != VertexTexture && buf.type != FragmentTexture)
            continue;
#endif

        bool uniform_error = false;
        switch (buf.type) {
            case IndexBuffer:
                CHK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer_id));
                break;

            case VertexBuffer:
                CHK(glBindBuffer(GL_ARRAY_BUFFER, buffer_id));
                CHK(glEnableVertexAttribArray(buf.index));

                switch (buf.dtype) {
                    case VariableType::Int8:    gl_type = GL_BYTE;           break;
                    case VariableType::UInt8:   gl_type = GL_UNSIGNED_BYTE;  break;
                    case VariableType::Int16:   gl_type = GL_SHORT;          break;
                    case VariableType::UInt16:  gl_type = GL_UNSIGNED_SHORT; break;
                    case VariableType::Int32:   gl_type = GL_INT;            break;
                    case VariableType::UInt32:  gl_type = GL_UNSIGNED_INT;   break;
                    case VariableType::Float16: gl_type = GL_HALF_FLOAT;     break;
                    case VariableType::Float32: gl_type = GL_FLOAT;          break;
                    default:
                        throw std::runtime_error(
                            "Shader::begin(): unsupported vertex buffer type!");
                }

                if (buf.ndim != 2)
                    throw std::runtime_error("\"" + m_name + "\": vertex attribute \"" + key +
                                             "\" has an invalid shapeension (expected ndim=2, got " +
                                             std::to_string(buf.ndim) + ")");

                CHK(glVertexAttribPointer(buf.index, (GLint) buf.shape[1],
                                          gl_type, GL_FALSE, 0, nullptr));
                break;

            case VertexTexture:
            case FragmentTexture:
                CHK(glActiveTexture(GL_TEXTURE0 + texture_unit));
                CHK(glBindTexture(GL_TEXTURE_2D, (GLuint) ((uintptr_t) buf.buffer)));
                if (buf.dirty)
                    CHK(glUniform1i(buf.index, texture_unit));
                texture_unit++;
                break;

            case UniformBuffer:
                if (buf.ndim > 2)
                    throw std::runtime_error("\"" + m_name + "\": uniform attribute \"" + key +
                                             "\" has an invalid shapeension (expected ndim=0/1/2, got " +
                                             std::to_string(buf.ndim) + ")");
                switch (buf.dtype) {
                    case VariableType::Float32:
                        if (buf.ndim < 2) {
                            const float *v = (const float *) buf.buffer;
                            switch (buf.shape[0]) {
                                case 1: CHK(glUniform1f(buf.index, v[0])); break;
                                case 2: CHK(glUniform2f(buf.index, v[0], v[1])); break;
                                case 3: CHK(glUniform3f(buf.index, v[0], v[1], v[2])); break;
                                case 4: CHK(glUniform4f(buf.index, v[0], v[1], v[2], v[3])); break;
                                default: uniform_error = true; break;
                            }
                        } else if (buf.ndim == 2 && buf.shape[0] == buf.shape[1]) {
                            const float *v = (const float *) buf.buffer;
                            switch (buf.shape[0]) {
                                case 2: CHK(glUniformMatrix2fv(buf.index, 1, GL_FALSE, v)); break;
                                case 3: CHK(glUniformMatrix3fv(buf.index, 1, GL_FALSE, v)); break;
                                case 4: CHK(glUniformMatrix4fv(buf.index, 1, GL_FALSE, v)); break;
                                default: uniform_error = true; break;
                            }
                        } else {
                            uniform_error = true;
                        }
                        break;

#if defined(NANOGUI_USE_GLES)
                    case VariableType::UInt32:
#endif
                    case VariableType::Int32: {
                            const int32_t *v = (const int32_t *) buf.buffer;
                            if (buf.ndim < 2) {
                                switch (buf.shape[0]) {
                                    case 1: CHK(glUniform1i(buf.index, v[0])); break;
                                    case 2: CHK(glUniform2i(buf.index, v[0], v[1])); break;
                                    case 3: CHK(glUniform3i(buf.index, v[0], v[1], v[2])); break;
                                    case 4: CHK(glUniform4i(buf.index, v[0], v[1], v[2], v[3])); break;
                                    default: uniform_error = true; break;
                                }
                            } else {
                                uniform_error = true;
                            }
                        }
                        break;

#if defined(NANOGUI_USE_OPENGL)
                    case VariableType::UInt32: {
                            const uint32_t *v = (const uint32_t *) buf.buffer;
                            if (buf.ndim < 2) {
                                switch (buf.shape[0]) {
                                    case 1: CHK(glUniform1ui(buf.index, v[0])); break;
                                    case 2: CHK(glUniform2ui(buf.index, v[0], v[1])); break;
                                    case 3: CHK(glUniform3ui(buf.index, v[0], v[1], v[2])); break;
                                    case 4: CHK(glUniform4ui(buf.index, v[0], v[1], v[2], v[3])); break;
                                    default: uniform_error = true; break;
                                }
                            } else {
                                uniform_error = true;
                            }
                        }
                        break;
#endif

                    case VariableType::Bool: {
                            const uint8_t *v = (const uint8_t *) buf.buffer;
                            if (buf.ndim < 2) {
                                switch (buf.shape[0]) {
                                    case 1: CHK(glUniform1i(buf.index, v[0])); break;
                                    case 2: CHK(glUniform2i(buf.index, v[0], v[1])); break;
                                    case 3: CHK(glUniform3i(buf.index, v[0], v[1], v[2])); break;
                                    case 4: CHK(glUniform4i(buf.index, v[0], v[1], v[2], v[3])); break;
                                    default: uniform_error = true; break;
                                }
                            } else {
                                uniform_error = true;
                            }
                        }
                        break;

                    default:
                        uniform_error = true;
                        break;
                }

                if (uniform_error)
                    throw std::runtime_error("\"" + m_name + "\": uniform attribute \"" + key +
                                             "\" has an unsupported dtype/shape configuration: " + buf.to_string());
                break;

            default:
                throw std::runtime_error("\"" + m_name + "\": uniform attribute \"" + key +
                                         "\" has an unsupported dtype/shape configuration:" + buf.to_string());
        }

        buf.dirty = false;
    }

    if (m_blend_mode == BlendMode::AlphaBlend) {
        CHK(glEnable(GL_BLEND));
        CHK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    }

#if defined(NANOGUI_USE_OPENGL)
    if (m_uses_point_size)
        CHK(glEnable(GL_PROGRAM_POINT_SIZE));
#endif
}

void Shader::end() {
    if (m_blend_mode == BlendMode::AlphaBlend)
        CHK(glDisable(GL_BLEND));
#if defined(NANOGUI_USE_OPENGL)
    if (m_uses_point_size)
        CHK(glDisable(GL_PROGRAM_POINT_SIZE));
    CHK(glBindVertexArray(0));
#else
    for (const auto &[key, buf] : m_buffers) {
        if (buf.type != VertexBuffer)
            continue;
        CHK(glDisableVertexAttribArray(buf.index));
    }
#endif
    CHK(glUseProgram(0));
}

void Shader::draw_array(PrimitiveType primitive_type,
                        size_t offset, size_t count,
                        bool indexed) {
    GLenum primitive_type_gl;
    switch (primitive_type) {
        case PrimitiveType::Point:         primitive_type_gl = GL_POINTS;         break;
        case PrimitiveType::Line:          primitive_type_gl = GL_LINES;          break;
        case PrimitiveType::LineStrip:     primitive_type_gl = GL_LINE_STRIP;     break;
        case PrimitiveType::Triangle:      primitive_type_gl = GL_TRIANGLES;      break;
        case PrimitiveType::TriangleStrip: primitive_type_gl = GL_TRIANGLE_STRIP; break;
        default: throw std::runtime_error("Shader::draw_array(): invalid primitive type!");
    }

    if (!indexed)
        CHK(glDrawArrays(primitive_type_gl, (GLint) offset, (GLsizei) count));
    else
        CHK(glDrawElements(primitive_type_gl, (GLsizei) count, GL_UNSIGNED_INT,
                           (const void *) (offset * sizeof(uint32_t))));
}

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
