#include "renderpass.h"
//#include "screen.h"
#include "texture.h"
#include "shader.h"
#include "metal.h"
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

RenderPass::RenderPass(std::vector<Object *> color_targets,
                       Object *depth_target,
                       Object *stencil_target,
                       Object *blit_target,
                       bool clear)
    : m_targets(color_targets.size() + 2), m_clear(clear),
      m_clear_color(color_targets.size()), m_viewport_offset(0),
      m_viewport_size(0), m_framebuffer_size(0),
      m_depth_test(DepthTest::Less), m_depth_write(true),
      m_cull_mode(CullMode::Back), m_blit_target(blit_target), m_active(false),
      m_command_buffer(nullptr), m_command_encoder(nullptr) {

    m_targets[0] = depth_target;
    m_targets[1] = stencil_target;
    for (size_t i = 0; i < color_targets.size(); ++i) {
        m_targets[i + 2] = color_targets[i];
        m_clear_color[i] = Color(0.f, 0.f, 0.f, 1.f);
    }
    m_clear_stencil = 0;
    m_clear_depth = 1.f;

    if (!m_targets[0].get()) {
        m_depth_write = false;
        m_depth_test = DepthTest::Always;
    }

    MTLRenderPassDescriptor *pass_descriptor =
        [MTLRenderPassDescriptor renderPassDescriptor];

    for (size_t i = 0; i < m_targets.size(); ++i) {
        Texture *texture = dynamic_cast<Texture *>(m_targets[i].get());
//        Screen *screen   = dynamic_cast<Screen *>(m_targets[i].get());

        if (texture) {
            if (!(texture->flags() & Texture::TextureFlags::RenderTarget))
                throw std::runtime_error("RenderPass::RenderPass(): target texture "
                                         "must be created with render_target=true!");
            m_framebuffer_size = max(m_framebuffer_size, texture->size());
//        } else if (screen) {
//            m_framebuffer_size = max(m_framebuffer_size, screen->framebuffer_size());
        } else if (m_targets[i].get()) {
            throw std::runtime_error("RenderPas::RenderPass(): invalid attachment type!");
        }
    }
    m_viewport_size = m_framebuffer_size;

    m_pass_descriptor = (__bridge_retained void *) pass_descriptor;

    for (size_t i = 0; i < color_targets.size(); ++i)
        set_clear_color(i, m_clear_color[i]);
    set_clear_depth(m_clear_depth);
    set_clear_stencil(m_clear_stencil);
}

RenderPass::~RenderPass() {
    (void) (__bridge_transfer MTLRenderPassDescriptor *) m_pass_descriptor;
}

void RenderPass::begin() {
#if !defined(NDEBUG)
    if (m_active)
        throw std::runtime_error("RenderPass::begin(): render pass is already active!");
#endif
    id<MTLCommandQueue> command_queue =
        (__bridge id<MTLCommandQueue>) metal_command_queue();

    id<MTLCommandBuffer> command_buffer = [command_queue commandBuffer];

    MTLRenderPassDescriptor *pass_descriptor =
        (__bridge MTLRenderPassDescriptor *) m_pass_descriptor;

    bool clear_manual = m_clear && (m_viewport_offset != Vector2i(0, 0) ||
                                    m_viewport_size != m_framebuffer_size);

    for (size_t i = 0; i < m_targets.size(); ++i) {
        Texture *texture = dynamic_cast<Texture *>(m_targets[i].get());
//        Screen *screen = dynamic_cast<Screen *>(m_targets[i].get());
        id<MTLTexture> texture_handle = nil;

        if (texture) {
            texture_handle = (__bridge id<MTLTexture>) texture->texture_handle();
//        } else if (screen) {
//            if (i < 2) {
//                Texture *screen_tex = screen->depth_stencil_texture();
//                if (screen_tex && (i == 0 ||
//                                  (i == 1 && (screen_tex->pixel_format() ==
//                                              Texture::PixelFormat::DepthStencil)))) {
//                    texture_handle = (__bridge id<MTLTexture>) screen_tex->texture_handle();
//                }
//            } else {
//                texture_handle = (__bridge id<MTLTexture>) screen->metal_texture();
//            }
        }

        MTLRenderPassAttachmentDescriptor *att;
        if (i == 0)
            att = pass_descriptor.depthAttachment;
        else if (i == 1)
            att = pass_descriptor.stencilAttachment;
        else
            att = pass_descriptor.colorAttachments[i-2];

        att.texture = texture_handle;

        att.loadAction = m_clear && ! clear_manual ?
            MTLLoadActionClear : MTLLoadActionLoad;

        RenderPass *blit_rp = dynamic_cast<RenderPass *>(m_blit_target.get());
        if (blit_rp && i < blit_rp->targets().size()) {
            const Texture *resolve_texture =
                dynamic_cast<Texture *>(blit_rp->targets()[i].get());
            if (resolve_texture == nullptr)
                continue;

            id<MTLTexture> resolve_texture_handle =
                (__bridge id<MTLTexture>) resolve_texture->texture_handle();

            if (resolve_texture_handle.pixelFormat != texture_handle.pixelFormat)
                throw std::runtime_error("RenderPass::begin(): 'blit_target' pixel format mismatch!");
            else if (resolve_texture_handle.width  != texture_handle.width ||
                     resolve_texture_handle.height != texture_handle.height)
                throw std::runtime_error("RenderPass::begin(): 'blit_target' size mismatch!");
            att.storeAction = MTLStoreActionMultisampleResolve;
            att.resolveTexture = resolve_texture_handle;
        } else {
            att.storeAction = MTLStoreActionStore;
        }
    }

//    Screen *blit_screen = dynamic_cast<Screen *>(m_blit_target.get());
//    if (blit_screen) {
//        id<MTLTexture> texture = (__bridge id<MTLTexture>) blit_screen->metal_texture();
//        if (texture.pixelFormat != pass_descriptor.colorAttachments[0].texture.pixelFormat)
//            throw std::runtime_error("RenderPass::begin(): 'blit_target' pixel format mismatch!");
//        else if (texture.width  != pass_descriptor.colorAttachments[0].texture.width ||
//                 texture.height != pass_descriptor.colorAttachments[0].texture.height)
//            throw std::runtime_error("RenderPass::begin(): 'blit_target' pixel format mismatch!");
//        pass_descriptor.colorAttachments[0].storeAction = MTLStoreActionMultisampleResolve;
//        pass_descriptor.colorAttachments[0].resolveTexture = texture;
//    }

    id<MTLRenderCommandEncoder> command_encoder =
        [command_buffer renderCommandEncoderWithDescriptor: pass_descriptor];

    [command_encoder setFrontFacingWinding: MTLWindingCounterClockwise];

    m_command_buffer = (__bridge_retained void *) command_buffer;
    m_command_encoder = (__bridge_retained void *) command_encoder;
    m_active = true;

    set_viewport(m_viewport_offset, m_viewport_size);

    if (clear_manual) {
        MTLDepthStencilDescriptor *depth_desc = [MTLDepthStencilDescriptor new];
        depth_desc.depthCompareFunction = MTLCompareFunctionAlways;
        depth_desc.depthWriteEnabled = m_targets[0].get() != nullptr;
        id<MTLDevice> device = (__bridge id<MTLDevice>) metal_device();
        id<MTLDepthStencilState> depth_state =
            [device newDepthStencilStateWithDescriptor: depth_desc];
        [command_encoder setDepthStencilState: depth_state];

        if (!m_clear_shader) {
            m_clear_shader = new Shader(
                this,

                "clear_shader",

                /* Vertex shader */
                R"(using namespace metal;

                struct VertexOut {
                    float4 position [[position]];
                };

                vertex VertexOut vertex_main(const device float2 *position,
                                             constant float &clear_depth,
                                             uint id [[vertex_id]]) {
                    VertexOut vert;
                    vert.position = float4(position[id], clear_depth, 1.f);
                    return vert;
                })",

                /* Fragment shader */
                R"(using namespace metal;

                struct VertexOut {
                    float4 position [[position]];
                };

                fragment float4 fragment_main(VertexOut vert [[stage_in]],
                                              constant float4 &clear_color) {
                    return clear_color;
                })"
            );

            const float positions[] = {
                -1.f, -1.f, 1.f, -1.f, -1.f, 1.f,
                 1.f, -1.f, 1.f,  1.f, -1.f, 1.f
            };

            m_clear_shader->set_buffer("position", VariableType::Float32, { 6, 2 }, positions);
        }

        m_clear_shader->set_uniform("clear_color", m_clear_color.at(0));
        m_clear_shader->set_uniform("clear_depth", m_clear_depth);
        m_clear_shader->begin();
        m_clear_shader->draw_array(Shader::PrimitiveType::Triangle, 0, 6, false);
        m_clear_shader->end();
    }


    set_depth_test(m_depth_test, m_depth_write);
    set_cull_mode(m_cull_mode);
}

void RenderPass::end() {
#if !defined(NDEBUG)
    if (!m_active)
        throw std::runtime_error("RenderPass::end(): render pass is not active!");
#endif
    id<MTLCommandBuffer> command_buffer =
        (__bridge_transfer id<MTLCommandBuffer>) m_command_buffer;
    id<MTLRenderCommandEncoder> command_encoder =
        (__bridge_transfer id<MTLRenderCommandEncoder>) m_command_encoder;
    [command_encoder endEncoding];
    [command_buffer commit];
    m_command_encoder = nullptr;
    m_command_buffer = nullptr;
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

    MTLRenderPassDescriptor *pass_descriptor =
        (__bridge MTLRenderPassDescriptor *) m_pass_descriptor;

    pass_descriptor.colorAttachments[index].clearColor =
        MTLClearColorMake(color.r(), color.g(), color.b(), color.w());
}

void RenderPass::set_clear_depth(float depth) {
    m_clear_depth = depth;

    MTLRenderPassDescriptor *pass_descriptor =
        (__bridge MTLRenderPassDescriptor *) m_pass_descriptor;
    pass_descriptor.depthAttachment.clearDepth = depth;
}

void RenderPass::set_clear_stencil(uint8_t stencil) {
    m_clear_stencil = stencil;

    MTLRenderPassDescriptor *pass_descriptor =
        (__bridge MTLRenderPassDescriptor *) m_pass_descriptor;
    pass_descriptor.stencilAttachment.clearStencil = stencil;
}

void RenderPass::set_viewport(const Vector2i &offset, const Vector2i &size) {
    m_viewport_offset = offset;
    m_viewport_size = size;
    if (m_active) {
        id<MTLRenderCommandEncoder> command_encoder =
            (__bridge id<MTLRenderCommandEncoder>) m_command_encoder;
        [command_encoder setViewport: (MTLViewport) {
          (double) offset.x(), (double) offset.y(),
          (double) size.x(),   (double) size.y(),
          0.0, 1.0 }
        ];
        Vector2i scissor_size = max(min(offset + size, m_framebuffer_size) - offset, Vector2i(0));
        Vector2i scissor_offset = max(min(offset, m_framebuffer_size), Vector2i(0));
        [command_encoder setScissorRect: (MTLScissorRect) {
          (NSUInteger) scissor_offset.x(), (NSUInteger) scissor_offset.y(),
          (NSUInteger) scissor_size.x(),   (NSUInteger) scissor_size.y() }
        ];
    }
}

void RenderPass::set_depth_test(DepthTest depth_test, bool depth_write) {
    m_depth_test = depth_test;
    m_depth_write = depth_write;
    if (m_active) {
        MTLDepthStencilDescriptor *depth_desc = [MTLDepthStencilDescriptor new];
        if (m_targets[0]) {
            MTLCompareFunction func;
            switch (depth_test) {
                case DepthTest::Never:        func = MTLCompareFunctionNever; break;
                case DepthTest::Less:         func = MTLCompareFunctionLess; break;
                case DepthTest::Equal:        func = MTLCompareFunctionEqual; break;
                case DepthTest::LessEqual:    func = MTLCompareFunctionLessEqual; break;
                case DepthTest::Greater:      func = MTLCompareFunctionGreater; break;
                case DepthTest::NotEqual:     func = MTLCompareFunctionNotEqual; break;
                case DepthTest::GreaterEqual: func = MTLCompareFunctionGreater; break;
                case DepthTest::Always:       func = MTLCompareFunctionAlways; break;
                default:
                    throw std::runtime_error(
                        "Shader::set_depth_test(): invalid depth test mode!");
            }
            depth_desc.depthCompareFunction = func;
            depth_desc.depthWriteEnabled = depth_write;
        } else {
            depth_desc.depthCompareFunction = MTLCompareFunctionAlways;
            depth_desc.depthWriteEnabled = NO;
        }

        id<MTLDevice> device = (__bridge id<MTLDevice>) metal_device();
        id<MTLDepthStencilState> depth_state =
            [device newDepthStencilStateWithDescriptor: depth_desc];
        id<MTLRenderCommandEncoder> command_encoder =
            (__bridge id<MTLRenderCommandEncoder>) m_command_encoder;
        [command_encoder setDepthStencilState: depth_state];
    }
}

void RenderPass::set_cull_mode(CullMode cull_mode) {
    m_cull_mode = cull_mode;
    if (m_active) {
        MTLCullMode cull_mode_mtl;
        switch (cull_mode) {
            case CullMode::Front:    cull_mode_mtl = MTLCullModeFront; break;
            case CullMode::Back:     cull_mode_mtl = MTLCullModeBack; break;
            case CullMode::Disabled: cull_mode_mtl = MTLCullModeNone; break;
            default: throw std::runtime_error("Shader::set_cull_mode(): invalid cull mode!");
        }
        id<MTLRenderCommandEncoder> command_encoder =
            (__bridge id<MTLRenderCommandEncoder>) m_command_encoder;
        [command_encoder setCullMode: cull_mode_mtl];
    }
}

void RenderPass::blit_to(const Vector2i &src_offset,
                         const Vector2i &src_size,
                         Object *dst,
                         const Vector2i &dst_offset) {

//    Screen *screen = dynamic_cast<Screen *>(dst);
    RenderPass *rp = dynamic_cast<RenderPass *>(dst);
    std::vector<void *> dst_textures;
    dst_textures.reserve(3);

//    if (screen) {
//        Texture *depth_stencil_texture = screen->depth_stencil_texture();
//        if (depth_stencil_texture)
//            dst_textures.push_back(depth_stencil_texture->texture_handle());
//        else
//            dst_textures.push_back(nullptr);
//        dst_textures.push_back(nullptr);
//        dst_textures.push_back(screen->metal_texture());
//    } else
      if (rp) {
        std::vector<ref<Object>>& targets = rp->targets();
        for (size_t i = 0; i < targets.size(); ++i) {
            Texture *texture = dynamic_cast<Texture *>(targets[i].get());
            if (texture)
                dst_textures.push_back(texture->texture_handle());
            else
                dst_textures.push_back(nullptr);
        }
    } else {
        throw std::runtime_error(
            "RenderPass::blit_to(): 'dst' must either be a RenderPass or a Screen instance.");
    }

    id<MTLCommandQueue> command_queue =
        (__bridge id<MTLCommandQueue>) metal_command_queue();
    id<MTLCommandBuffer> command_buffer = [command_queue commandBuffer];
    id<MTLBlitCommandEncoder> command_encoder =
        [command_buffer blitCommandEncoder];

    for (size_t i = 0; i < std::min(dst_textures.size(), m_targets.size()); ++i) {
        Texture *texture = dynamic_cast<Texture *>(m_targets[i].get());
        id<MTLTexture> src_texture =
            (__bridge id<MTLTexture>) (texture ? texture->texture_handle()
                                               : nullptr);
        id<MTLTexture> dst_texture = (__bridge id<MTLTexture>) dst_textures[i];

        if (src_texture == nil || dst_texture == nil || i == 1)
            continue;

        [command_encoder
              copyFromTexture: src_texture
                  sourceSlice: 0
                  sourceLevel: 0
                 sourceOrigin: MTLOriginMake((NSUInteger) src_offset.x(),
                                             (NSUInteger) src_offset.y(), 0)
                   sourceSize: MTLSizeMake((NSUInteger) src_size.x(),
                                           (NSUInteger) src_size.y(), 1)
                    toTexture: dst_texture
             destinationSlice: 0
             destinationLevel: 0
            destinationOrigin: MTLOriginMake((NSUInteger) dst_offset.x(),
                                             (NSUInteger) dst_offset.y(), 0)];
    }

    [command_encoder endEncoding];
    [command_buffer commit];
}

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
