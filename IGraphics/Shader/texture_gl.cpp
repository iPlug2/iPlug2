#include "texture.h"
#include "opengl.h"
#include "opengl_check.h"
#include <memory>

#if !defined(GL_HALF_FLOAT)
#  define GL_HALF_FLOAT 0x140B
#endif
#if !defined(GL_DEPTH_STENCIL)
#  define GL_DEPTH_STENCIL 0x84F9
#endif
#if !defined(GL_R8)
#  define GL_R8 0x8229
#  define GL_RG8 0x822B
#endif
#if !defined(GL_R16)
#  define GL_R16 0x822A
#  define GL_RG16 0x822C
#endif
#if !defined(GL_R16F)
#  define GL_R16F 0x822D
#  define GL_RG16F 0x822F
#endif
#if !defined(GL_R32F)
#  define GL_R32F 0x822E
#  define GL_RG32F 0x8230
#endif

#if !defined(GL_RGB8)
#  define GL_RGB8 0x8051
#  define GL_RGBA8 0x8058
#  define GL_RGBA32F 0x8814
#  define GL_RGB32F 0x8815
#  define GL_RGBA16F 0x881A
#  define GL_RGB16F 0x881B
#endif

#if !defined(GL_RGB16)
#  define GL_RGB16 0x8054
#  define GL_RGBA16 0x805B
#endif

#if !defined(GL_DEPTH_COMPONENT24)
#  define GL_DEPTH_COMPONENT24 0x81A6
#endif

#if !defined(GL_DEPTH_COMPONENT32F)
#  define GL_DEPTH_COMPONENT32F 0x8CAC
#endif

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

static void gl_map_texture_format(Texture::PixelFormat &pixel_format,
                                  Texture::ComponentFormat &component_format,
                                  GLenum &pixel_format_gl,
                                  GLenum &component_format_gl,
                                  GLenum &internal_format_gl);

void Texture::init() {
#if defined(NANOGUI_USE_GLES)
    m_samples = 1;
#endif

    GLuint interpolation_mode_gl[2];
    for (int i = 0; i<2; ++i) {
        switch (i == 0 ? m_min_interpolation_mode : m_mag_interpolation_mode) {
            case InterpolationMode::Nearest:
                interpolation_mode_gl[i] = GL_NEAREST;
                break;

            case InterpolationMode::Bilinear:
                interpolation_mode_gl[i] = GL_LINEAR;
                break;

            case InterpolationMode::Trilinear:
                interpolation_mode_gl[i] = GL_LINEAR_MIPMAP_LINEAR;
                break;

            default: throw std::runtime_error("Texture::Texture(): invalid interpolation mode!");
        }
    }


    GLuint wrap_mode_gl = 0;
    switch (m_wrap_mode) {
        case WrapMode::Repeat:       wrap_mode_gl = GL_REPEAT; break;
        case WrapMode::ClampToEdge:  wrap_mode_gl = GL_CLAMP_TO_EDGE; break;
        case WrapMode::MirrorRepeat: wrap_mode_gl = GL_MIRRORED_REPEAT; break;
        default: throw std::runtime_error("Texture::Texture(): invalid wrap mode!");
    }

    GLenum pixel_format_gl,
           component_format_gl,
           internal_format_gl;

    gl_map_texture_format(m_pixel_format,
                          m_component_format,
                          pixel_format_gl,
                          component_format_gl,
                          internal_format_gl);

    (void) pixel_format_gl; (void) component_format_gl;

    GLenum tex_mode = m_samples > 1 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;

    if (m_flags & (uint8_t) TextureFlags::ShaderRead) {
        CHK(glGenTextures(1, &m_texture_handle));
        CHK(glBindTexture(tex_mode, m_texture_handle));
        CHK(glTexParameteri(tex_mode, GL_TEXTURE_MIN_FILTER, interpolation_mode_gl[0]));
        CHK(glTexParameteri(tex_mode, GL_TEXTURE_MAG_FILTER, interpolation_mode_gl[1]));
        CHK(glTexParameteri(tex_mode, GL_TEXTURE_WRAP_S, wrap_mode_gl));
        CHK(glTexParameteri(tex_mode, GL_TEXTURE_WRAP_T, wrap_mode_gl));

        if (m_flags & (uint8_t) TextureFlags::RenderTarget)
            upload(nullptr);
    } else if (m_flags & (uint8_t) TextureFlags::RenderTarget) {
        CHK(glGenRenderbuffers(1, &m_renderbuffer_handle));
        CHK(glBindRenderbuffer(GL_RENDERBUFFER, m_renderbuffer_handle));
#if defined(NANOGUI_USE_OPENGL)
        if (m_samples == 1)
            CHK(glRenderbufferStorage(GL_RENDERBUFFER, internal_format_gl,
                                      (GLsizei) m_size.x(), (GLsizei) m_size.y()));
        else
            CHK(glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_samples, internal_format_gl,
                                                 (GLsizei) m_size.x(), (GLsizei) m_size.y()));
#else
        CHK(glRenderbufferStorage(GL_RENDERBUFFER, internal_format_gl,
                                  (GLsizei) m_size.x(), (GLsizei) m_size.y()));
#endif
    } else {
        throw std::runtime_error(
            "Texture::Texture(): flags must either specify ShaderRead, RenderTarget, or both!");
    }
}

Texture::~Texture() {
    CHK(glDeleteTextures(1, &m_texture_handle));
    CHK(glDeleteRenderbuffers(1, &m_renderbuffer_handle));
}

void Texture::upload(const uint8_t *data) {
    if (m_samples > 1 && data != nullptr)
        throw std::runtime_error("Texture::upload(): only implemented for samples=1!");

    GLenum pixel_format_gl,
           component_format_gl,
           internal_format_gl;

    gl_map_texture_format(m_pixel_format,
                          m_component_format,
                          pixel_format_gl,
                          component_format_gl,
                          internal_format_gl);

    if (m_texture_handle != 0) {
        GLenum tex_mode = m_samples > 1 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
        CHK(glBindTexture(tex_mode, m_texture_handle));

        if (data)
            CHK(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));

#if defined(NANOGUI_USE_OPENGL)
        if (data) {
            CHK(glPixelStorei(GL_UNPACK_ROW_LENGTH, 0));
            CHK(glPixelStorei(GL_UNPACK_SKIP_ROWS, 0));
            CHK(glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0));
        }

        if (m_samples == 1)
            CHK(glTexImage2D(tex_mode, 0, internal_format_gl, (GLsizei) m_size.x(),
                             (GLsizei) m_size.y(), 0, pixel_format_gl, component_format_gl, data));
        else
            CHK(glTexImage2DMultisample(tex_mode, m_samples, internal_format_gl,
                                        (GLsizei) m_size.x(), (GLsizei) m_size.y(), false));
#else
        CHK(glTexImage2D(tex_mode, 0, internal_format_gl, (GLsizei) m_size.x(),
                         (GLsizei) m_size.y(), 0, pixel_format_gl, component_format_gl, data));
#endif

        if (!m_mipmap_manual && (m_min_interpolation_mode == InterpolationMode::Trilinear ||
            m_mag_interpolation_mode == InterpolationMode::Trilinear))
            generate_mipmap();
    } else {
#if defined(NANOGUI_USE_OPENGL)
        CHK(glBindRenderbuffer(GL_RENDERBUFFER, m_renderbuffer_handle));
        if (m_samples == 1)
            CHK(glRenderbufferStorage(GL_RENDERBUFFER, internal_format_gl,
                                      (GLsizei) m_size.x(), (GLsizei) m_size.y()));
        else
            CHK(glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_samples, internal_format_gl,
                                                 (GLsizei) m_size.x(), (GLsizei) m_size.y()));
#else
        CHK(glRenderbufferStorage(GL_RENDERBUFFER, internal_format_gl,
                                  (GLsizei) m_size.x(), (GLsizei) m_size.y()));
#endif
    }
}

void Texture::upload_sub_region(const uint8_t *data, const Vector2i& origin, const Vector2i& size) {
    if (m_samples > 1 && data != nullptr)
        throw std::runtime_error("Texture::upload_sub_region(): only implemented for samples=1!");

    GLenum pixel_format_gl,
           component_format_gl,
           internal_format_gl;

    gl_map_texture_format(m_pixel_format,
                          m_component_format,
                          pixel_format_gl,
                          component_format_gl,
                          internal_format_gl);

    if (m_texture_handle == 0)
        throw std::runtime_error("Texture::upload_sub_region(): not implemented for render targets!");

    if (origin.x() + size.x() > m_size.x() || origin.y() + size.y() > m_size.y())
        throw std::runtime_error("Texture::upload_sub_region(): out of bounds!");

    GLenum tex_mode = m_samples > 1 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
    CHK(glBindTexture(tex_mode, m_texture_handle));

    if (data)
        CHK(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));

#if defined(NANOGUI_USE_OPENGL)
    if (data) {
        CHK(glPixelStorei(GL_UNPACK_ROW_LENGTH, 0));
        CHK(glPixelStorei(GL_UNPACK_SKIP_ROWS, 0));
        CHK(glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0));
    }
#endif

    CHK(glTexSubImage2D(tex_mode, 0, (GLsizei) origin.x(), (GLsizei) origin.y(), (GLsizei) size.x(),
                        (GLsizei) size.y(), pixel_format_gl, component_format_gl, data));

    if (!m_mipmap_manual && (m_min_interpolation_mode == InterpolationMode::Trilinear ||
        m_mag_interpolation_mode == InterpolationMode::Trilinear))
        generate_mipmap();
}

void Texture::download(uint8_t *data) {
#if defined(NANOGUI_USE_GLES)
    (void) data;
    throw std::runtime_error("Texture::download(): not supported on GLES 2!");
#else
    if (m_texture_handle == 0)
        throw std::runtime_error("Texture::download(): no texture handle!");
    else if (m_samples > 1)
        throw std::runtime_error("Texture::download(): only implemented for samples=1!");

    GLenum pixel_format_gl,
           component_format_gl,
           internal_format_gl;

    gl_map_texture_format(m_pixel_format,
                          m_component_format,
                          pixel_format_gl,
                          component_format_gl,
                          internal_format_gl);

    (void) internal_format_gl;
    CHK(glBindTexture(GL_TEXTURE_2D, m_texture_handle));
    CHK(glGetTexImage(GL_TEXTURE_2D, 0, pixel_format_gl, component_format_gl, data));

    if (m_flags & (uint8_t) TextureFlags::RenderTarget) {
        size_t stride = bytes_per_pixel() * m_size.x();
        std::unique_ptr<uint8_t[]> temp(new uint8_t[stride]);

        uint8_t *low = (uint8_t *) data,
                *high = low + (m_size.y() - 1) * stride;

        for (; low < high; low += stride, high -= stride) {
            memcpy(temp.get(), low, stride);
            memcpy(low, high, stride);
            memcpy(high, temp.get(), stride);
        }
    }
#endif
}

void Texture::resize(const Vector2i &size) {
    if (m_size == size)
        return;
    m_size = size;
    upload(nullptr);
}

void Texture::generate_mipmap() {
    GLenum tex_mode = m_samples > 1 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
    CHK(glBindTexture(tex_mode, m_texture_handle));
    CHK(glGenerateMipmap(tex_mode));
}

static void gl_map_texture_format(Texture::PixelFormat &pixel_format,
                                  Texture::ComponentFormat &component_format,
                                  GLenum &pixel_format_gl,
                                  GLenum &component_format_gl,
                                  GLenum &internal_format_gl) {
    using PixelFormat = Texture::PixelFormat;
    using ComponentFormat = Texture::ComponentFormat;

    if (pixel_format == PixelFormat::BGR)
        pixel_format = PixelFormat::RGB;
    else if (pixel_format == PixelFormat::BGRA)
        pixel_format = PixelFormat::RGBA;

    pixel_format_gl = component_format_gl = internal_format_gl = 0;

    switch (pixel_format) {
        case PixelFormat::R:
#if defined(NANOGUI_USE_OPENGL)
            pixel_format_gl = GL_RED;
#else
            pixel_format_gl = GL_LUMINANCE;
#endif

            switch (component_format) {
                case ComponentFormat::UInt8:   internal_format_gl = GL_R8;        break;
                case ComponentFormat::UInt16:  internal_format_gl = GL_R16;       break;
                case ComponentFormat::Float16: internal_format_gl = GL_R16F;      break;
                case ComponentFormat::Float32: internal_format_gl = GL_R32F;      break;
#if defined(NANOGUI_USE_OPENGL)
                case ComponentFormat::Int8:    internal_format_gl = GL_R8_SNORM;  break;
                case ComponentFormat::Int16:   internal_format_gl = GL_R16_SNORM; break;
#endif
                default:
                    break;
            }
            break;

        case PixelFormat::RA:
#if defined(NANOGUI_USE_OPENGL)
            pixel_format_gl = GL_RG;
#else
            pixel_format_gl = GL_LUMINANCE_ALPHA;
#endif

            switch (component_format) {
                case ComponentFormat::UInt8:   internal_format_gl = GL_RG8;        break;
                case ComponentFormat::UInt16:  internal_format_gl = GL_RG16;       break;
                case ComponentFormat::Float16: internal_format_gl = GL_RG16F;      break;
                case ComponentFormat::Float32: internal_format_gl = GL_RG32F;      break;
#if defined(NANOGUI_USE_OPENGL)
                case ComponentFormat::Int8:    internal_format_gl = GL_RG8_SNORM;  break;
                case ComponentFormat::Int16:   internal_format_gl = GL_RG16_SNORM; break;
#endif
                default:
                    break;
            }
            break;

        case PixelFormat::RGB:
            pixel_format_gl = GL_RGB;

            switch (component_format) {
                case ComponentFormat::UInt8:   internal_format_gl = GL_RGB8;        break;
                case ComponentFormat::UInt16:  internal_format_gl = GL_RGB16;       break;
                case ComponentFormat::Float16: internal_format_gl = GL_RGB16F;      break;
                case ComponentFormat::Float32: internal_format_gl = GL_RGB32F;      break;
#if defined(NANOGUI_USE_OPENGL)
                case ComponentFormat::Int8:    internal_format_gl = GL_RGB8_SNORM;  break;
                case ComponentFormat::Int16:   internal_format_gl = GL_RGB16_SNORM; break;
#endif
                default:
                    break;
            }
            break;

        case PixelFormat::RGBA:
            pixel_format_gl = GL_RGBA;

            switch (component_format) {
                case ComponentFormat::UInt8:   internal_format_gl = GL_RGBA8;        break;
                case ComponentFormat::UInt16:  internal_format_gl = GL_RGBA16;       break;
                case ComponentFormat::Float16: internal_format_gl = GL_RGBA16F;      break;
                case ComponentFormat::Float32: internal_format_gl = GL_RGBA32F;      break;
#if defined(NANOGUI_USE_OPENGL)
                case ComponentFormat::Int8:    internal_format_gl = GL_RGBA8_SNORM;  break;
                case ComponentFormat::Int16:   internal_format_gl = GL_RGBA16_SNORM; break;
#endif
                default:
                    break;
            }
            break;

        case PixelFormat::Depth:
            pixel_format_gl = GL_DEPTH_COMPONENT;

            switch (component_format) {
                case ComponentFormat::Int8:
                case ComponentFormat::UInt8:
                case ComponentFormat::Int16:
                case ComponentFormat::UInt16:
                case ComponentFormat::Int32:
                case ComponentFormat::UInt32:
                    component_format = ComponentFormat::UInt32;
                    internal_format_gl = GL_DEPTH_COMPONENT16;
                    break;

                case ComponentFormat::Float16:
                case ComponentFormat::Float32:
                    component_format = ComponentFormat::Float32;
                    internal_format_gl = GL_DEPTH_COMPONENT32F;
                    break;

                default:
                    break;
            }
            break;

        case PixelFormat::DepthStencil:
            pixel_format_gl = GL_DEPTH_STENCIL;

            switch (component_format) {
                case ComponentFormat::Int8:
                case ComponentFormat::UInt8:
                case ComponentFormat::Int16:
                case ComponentFormat::UInt16:
                case ComponentFormat::Int32:
                case ComponentFormat::UInt32:
                    component_format = ComponentFormat::UInt32;
                    internal_format_gl = GL_DEPTH_COMPONENT24;
                    break;

                case ComponentFormat::Float16:
                case ComponentFormat::Float32:
                    component_format = ComponentFormat::Float32;
                    internal_format_gl = GL_DEPTH_COMPONENT32F;
                    break;

                default:
                    break;
            }
            break;

        default:
            break;
    }

    switch (component_format) {
        case ComponentFormat::Int8:    component_format_gl = GL_BYTE;           break;
        case ComponentFormat::UInt8:   component_format_gl = GL_UNSIGNED_BYTE;  break;
        case ComponentFormat::Int16:   component_format_gl = GL_SHORT;          break;
        case ComponentFormat::UInt16:  component_format_gl = GL_UNSIGNED_SHORT; break;
        case ComponentFormat::Float16: component_format_gl = GL_HALF_FLOAT;     break;
        case ComponentFormat::Float32: component_format_gl = GL_FLOAT;          break;
        default:
            break;
    }

    if (component_format_gl == 0)
        throw std::runtime_error("gl_map_texture_format(): invalid component format!");
    if (pixel_format_gl == 0)
        throw std::runtime_error("gl_map_texture_format(): invalid pixel format!");
    if (internal_format_gl == 0)
        throw std::runtime_error("gl_map_texture_format(): component format unsupported "
                                 "for the given pixel format!");
}


END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
