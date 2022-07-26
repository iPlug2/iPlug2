#include "renderpass.h"
#include "screen.h"
#include "opengl.h"
#include "texture.h"
#include "opengl_check.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

RenderPass::RenderPass(std::vector<Object *> color_targets,
                       Object *depth_target,
                       Object *stencil_target,
                       Object *blit_target,
                       bool clear)
    : m_targets(color_targets.size() + 2), m_clear(clear),
      m_clear_color(color_targets.size()), m_viewport_offset(0),
      m_viewport_size(0), m_framebuffer_size(0), m_depth_test(DepthTest::Less),
      m_depth_write(true), m_cull_mode(CullMode::Back), m_blit_target(blit_target),
      m_active(false), m_framebuffer_handle(0) {

    m_targets[0] = depth_target;
    m_targets[1] = stencil_target;
    for (size_t i = 0; i < color_targets.size(); ++i) {
        m_targets[i + 2] = color_targets[i];
        m_clear_color[i] = Color(0, 0, 0, 0);
    }
    m_clear_stencil = 0;
    m_clear_depth = 1.f;

    if (!m_targets[0].get()) {
        m_depth_write = false;
        m_depth_test = DepthTest::Always;
    }

    CHK(glGenFramebuffers(1, &m_framebuffer_handle));
    CHK(glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer_handle));

#if defined(NANOGUI_USE_OPENGL)
    std::vector<GLenum> draw_buffers;
#endif

    bool has_texture = false,
         has_screen  = false;

    for (size_t i = 0; i < m_targets.size(); ++i) {
        GLenum attachment_id;
        if (i == 0)
            attachment_id = GL_DEPTH_ATTACHMENT;
        else if (i == 1)
            attachment_id = GL_STENCIL_ATTACHMENT;
        else
            attachment_id = (GLenum) (GL_COLOR_ATTACHMENT0 + i - 2);

        Screen *screen = dynamic_cast<Screen *>(m_targets[i].get());
        Texture *texture = dynamic_cast<Texture *>(m_targets[i].get());
        if (screen) {
            m_framebuffer_size = max(m_framebuffer_size, screen->framebuffer_size());
#if defined(NANOGUI_USE_OPENGL)
            if (i >= 2)
                draw_buffers.push_back(GL_BACK_LEFT);
#endif
            has_screen = true;
        } else if (texture) {
            if (texture->flags() & Texture::TextureFlags::ShaderRead) {
                CHK(glFramebufferTexture2D(GL_FRAMEBUFFER, attachment_id, GL_TEXTURE_2D,
                                           texture->texture_handle(), 0));
            } else {
                CHK(glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment_id, GL_RENDERBUFFER,
                                              texture->renderbuffer_handle()));
            }
#if defined(NANOGUI_USE_OPENGL)
            if (i >= 2)
                draw_buffers.push_back(attachment_id);
#endif
            m_framebuffer_size = max(m_framebuffer_size, texture->size());
            has_texture = true;
        }
    }
    m_viewport_size = m_framebuffer_size;

    if (has_screen && !has_texture) {
        CHK(glDeleteFramebuffers(1, &m_framebuffer_handle));
        m_framebuffer_handle = 0;
    } else {
#if defined(NANOGUI_USE_OPENGL)
        CHK(glDrawBuffers((GLsizei) draw_buffers.size(), draw_buffers.data()));
#endif

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            const char *reason = "unknown";
            switch (status) {
                case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                    reason = "incomplete attachment";
                    break;

                case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                    reason = "incomplete, missing attachment";
                    break;

                case GL_FRAMEBUFFER_UNSUPPORTED:
                    reason = "unsupported";
                    break;

#if defined(NANOGUI_USE_OPENGL)
                case GL_FRAMEBUFFER_UNDEFINED:
                    reason = "undefined";
                    break;

                case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
                    reason = "incomplete draw buffer";
                    break;

                case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
                    reason = "incomplete read buffer";
                    break;

                case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
                    reason = "incomplete multisample";
                    break;

                case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
                    reason = "incomplete layer targets";
                    break;
#else
                case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
                    reason = "incomplete dimensions";
                    break;
#endif
            }
            throw std::runtime_error(
                "RenderPass::RenderPass(): framebuffer is marked as incomplete: " +
                std::string(reason));
        }
    }

    CHK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

RenderPass::~RenderPass() {
    CHK(glDeleteFramebuffers(1, &m_framebuffer_handle));
}

void RenderPass::begin() {
#if !defined(NDEBUG)
    if (m_active)
        throw std::runtime_error("RenderPass::begin(): render pass is already active!");
#endif
    m_active = true;

    CHK(glGetIntegerv(GL_VIEWPORT, m_viewport_backup));
    CHK(glGetIntegerv(GL_SCISSOR_BOX, m_scissor_backup));
    GLboolean depth_write;
    CHK(glGetBooleanv(GL_DEPTH_WRITEMASK, &depth_write));
    m_depth_write_backup = depth_write;

    m_depth_test_backup = glIsEnabled(GL_DEPTH_TEST);
    m_scissor_test_backup = glIsEnabled(GL_SCISSOR_TEST);
    m_cull_face_backup = glIsEnabled(GL_CULL_FACE);
    m_blend_backup = glIsEnabled(GL_BLEND);

    CHK(glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer_handle));
    set_viewport(m_viewport_offset, m_viewport_size);

    if (m_clear) {
#if defined(NANOGUI_USE_OPENGL)
        for (size_t i = 0; i < m_targets.size(); ++i) {
            if (i == 0 && m_targets[0]) {
                if (m_targets[0] == m_targets[1])
                    CHK(glClearBufferfi(GL_DEPTH_STENCIL, 0, m_clear_depth, m_clear_stencil));
                else {
                    CHK(glClearBufferfv(GL_DEPTH, 0, &m_clear_depth));
                }
            }

            if (i >= 2)
                CHK(glClearBufferfv(GL_COLOR, (GLint) i - 2, m_clear_color[i - 2].v));
        }
#else
        GLenum what = 0;
        if (m_targets[0]) {
            CHK(glClearDepthf(m_clear_depth));
            what |= GL_DEPTH_BUFFER_BIT;
        }
        if (m_targets[1]) {
            CHK(glClearStencil(m_clear_stencil));
            what |= GL_STENCIL_BUFFER_BIT;
        }
        if (m_targets[2]) {
            CHK(glClearColor(m_clear_color[0].r(), m_clear_color[0].g(),
                             m_clear_color[0].b(), m_clear_color[0].w()));
            what |= GL_COLOR_BUFFER_BIT;
        }
        CHK(glClear(what));
#endif
    }

    set_depth_test(m_depth_test, m_depth_write);
    set_cull_mode(m_cull_mode);

    if (m_blend_backup)
        CHK(glDisable(GL_BLEND));
}

void RenderPass::end() {
#if !defined(NDEBUG)
    if (!m_active)
        throw std::runtime_error("RenderPass::end(): render pass is not active!");
#endif

    CHK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    if (m_blit_target)
        blit_to(Vector2i(0, 0), m_framebuffer_size, m_blit_target, Vector2i(0, 0));

    CHK(glViewport(m_viewport_backup[0], m_viewport_backup[1],
                   m_viewport_backup[2], m_viewport_backup[3]));
    CHK(glScissor(m_scissor_backup[0], m_scissor_backup[1],
                  m_scissor_backup[2], m_scissor_backup[3]));

    if (m_depth_test_backup)
        CHK(glEnable(GL_DEPTH_TEST));
    else
        CHK(glDisable(GL_DEPTH_TEST));

    CHK(glDepthMask(m_depth_write_backup));

    if (m_scissor_test_backup)
        CHK(glEnable(GL_SCISSOR_TEST));
    else
        CHK(glDisable(GL_SCISSOR_TEST));

    if (m_cull_face_backup)
        CHK(glEnable(GL_CULL_FACE));
    else
        CHK(glDisable(GL_CULL_FACE));

    if (m_blend_backup)
        CHK(glEnable(GL_BLEND));
    else
        CHK(glDisable(GL_BLEND));

    m_active = false;
}

void RenderPass::resize(const Vector2i &size) {
    for (size_t i = 0; i < m_targets.size(); ++i) {
        Texture *texture = dynamic_cast<Texture *>(m_targets[i].get());
        if (texture)
            texture->resize(size);
    }
    m_framebuffer_size = size;
    m_viewport_offset = Vector2i(0, 0);
    m_viewport_size = size;
}

void RenderPass::set_clear_color(size_t index, const Color &color) {
    m_clear_color.at(index) = color;
}

void RenderPass::set_clear_depth(float depth) {
    m_clear_depth = depth;
}

void RenderPass::set_clear_stencil(uint8_t stencil) {
    m_clear_stencil = stencil;
}

void RenderPass::set_viewport(const Vector2i &offset, const Vector2i &size) {
    m_viewport_offset = offset;
    m_viewport_size = size;

    if (m_active) {
        int ypos = m_framebuffer_size.y() - m_viewport_size.y() - m_viewport_offset.y();
        CHK(glViewport(m_viewport_offset.x(), ypos,
                       m_viewport_size.x(), m_viewport_size.y()));
        CHK(glScissor(m_viewport_offset.x(), ypos,
                      m_viewport_size.x(), m_viewport_size.y()));

        if (m_viewport_offset == Vector2i(0, 0) &&
            m_viewport_size == m_framebuffer_size)
            CHK(glDisable(GL_SCISSOR_TEST));
        else
            CHK(glEnable(GL_SCISSOR_TEST));
    }
}

void RenderPass::set_depth_test(DepthTest depth_test, bool depth_write) {
    m_depth_test = depth_test;
    m_depth_write = depth_write;

    if (m_active) {
        if (m_targets[0] && depth_test != DepthTest::Always) {
            GLenum func;
            switch (depth_test) {
                case DepthTest::Never:        func = GL_NEVER;    break;
                case DepthTest::Less:         func = GL_LESS;     break;
                case DepthTest::Equal:        func = GL_EQUAL;    break;
                case DepthTest::LessEqual:    func = GL_LEQUAL;   break;
                case DepthTest::Greater:      func = GL_GREATER;  break;
                case DepthTest::NotEqual:     func = GL_NOTEQUAL; break;
                case DepthTest::GreaterEqual: func = GL_GEQUAL;   break;
                default:
                    throw std::runtime_error("Shader::set_depth_test(): invalid depth test mode!");
            }
            CHK(glEnable(GL_DEPTH_TEST));
            CHK(glDepthFunc(func));
        } else {
            CHK(glDisable(GL_DEPTH_TEST));
        }
        CHK(glDepthMask(depth_write ? GL_TRUE : GL_FALSE));
    }
}

void RenderPass::set_cull_mode(CullMode cull_mode) {
    m_cull_mode = cull_mode;

    if (m_active) {
        if (cull_mode == CullMode::Disabled) {
            CHK(glDisable(GL_CULL_FACE));
        } else {
            CHK(glEnable(GL_CULL_FACE));
            if (cull_mode == CullMode::Front)
                CHK(glCullFace(GL_FRONT));
            else if (cull_mode == CullMode::Back)
                CHK(glCullFace(GL_BACK));
            else
                throw std::runtime_error("Shader::set_cull_mode(): invalid cull mode!");
        }
    }
}

void RenderPass::blit_to(const Vector2i &src_offset,
                         const Vector2i &src_size,
                         Object *dst,
                         const Vector2i &dst_offset) {
#if defined(NANOGUI_USE_GLES) && NANOGUI_GLES_VERSION == 2
    (void) src_offset; (void) src_size; (void) dst; (void) src_offset;
    throw std::runtime_error("RenderPass::blit_to(): not supported on GLES 2!");
#else
    Screen *screen = dynamic_cast<Screen *>(dst);
    RenderPass *rp = dynamic_cast<RenderPass *>(dst);

    GLuint target_id;
    GLenum what = 0;

    if (screen) {
        target_id = 0;
        what = GL_COLOR_BUFFER_BIT;
        if (screen->has_depth_buffer() && m_targets[0])
            what |= GL_STENCIL_BUFFER_BIT;
        if (screen->has_stencil_buffer() && m_targets[1])
            what |= GL_STENCIL_BUFFER_BIT;
    } else if (rp) {
        target_id = rp->framebuffer_handle();
        if (rp->targets().size() > 0 && rp->targets()[0] && m_targets[0])
            what |= GL_DEPTH_BUFFER_BIT;
        if (rp->targets().size() > 1 && rp->targets()[1] && m_targets[1])
            what |= GL_STENCIL_BUFFER_BIT;
        if (rp->targets().size() > 2 && rp->targets()[2] && m_targets[2])
            what |= GL_COLOR_BUFFER_BIT;
    } else {
        throw std::runtime_error(
            "RenderPass::blit_to(): 'dst' must either be a RenderPass or a Screen instance.");
    }
    #if defined(NANOGUI_USE_GLES)
        what = GL_COLOR_BUFFER_BIT;
    #endif

    CHK(glBindFramebuffer(GL_READ_FRAMEBUFFER, m_framebuffer_handle));
    CHK(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target_id));

    if (target_id == 0) {
        #if defined(NANOGUI_USE_OPENGL)
            CHK(glDrawBuffer(GL_BACK));
        #else
            GLenum buf = GL_BACK;
            CHK(glDrawBuffers(1, &buf));
        #endif
    }

    Vector2i src_end = src_offset + src_size,
             dst_end = dst_offset + src_size;

    CHK(glBlitFramebuffer((GLsizei) src_offset.x(), (GLsizei) src_offset.y(),
                          (GLsizei) src_end.x(), (GLsizei) src_end.y(),
                          (GLsizei) dst_offset.x(), (GLsizei) dst_offset.y(),
                          (GLsizei) dst_end.x(), (GLsizei) dst_end.y(),
                          what, GL_NEAREST));

    CHK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
#endif
}

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
