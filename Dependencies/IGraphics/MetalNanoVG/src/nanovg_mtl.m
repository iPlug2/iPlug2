// Copyright (c) 2017 Ollix
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// ---
// Author: olliwang@ollix.com (Olli Wang)

#include "nanovg_mtl.h"

#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#import <simd/simd.h>
#import <Metal/Metal.h>
#include <TargetConditionals.h>
#import <QuartzCore/QuartzCore.h>

#include "nanovg.h"

#if TARGET_OS_IOS == 1
#  include "mnvg_bitcode/ios_1_0.h"
#  include "mnvg_bitcode/ios_1_1.h"
#  include "mnvg_bitcode/ios_1_2.h"
#  include "mnvg_bitcode/ios_2_0.h"
#  include "mnvg_bitcode/ios_2_1.h"
#elif TARGET_OS_TV == 1
#  include "mnvg_bitcode/tvos.h"
#elif TARGET_OS_OSX == 1
#  include "mnvg_bitcode/macos_1_1.h"
#  include "mnvg_bitcode/macos_1_2.h"
#  include "mnvg_bitcode/macos_2_0.h"
#  include "mnvg_bitcode/macos_2_1.h"
#else
#  define MNVG_INVALID_TARGET
#endif

typedef enum MNVGvertexInputIndex {
  MNVG_VERTEX_INPUT_INDEX_VERTICES = 0,
  MNVG_VERTEX_INPUT_INDEX_VIEW_SIZE = 1,
} MNVGvertexInputIndex;

typedef enum MNVGshaderType {
  MNVG_SHADER_FILLGRAD,
  MNVG_SHADER_FILLIMG,
  MNVG_SHADER_IMG,
} MNVGshaderType;

enum MNVGcallType {
  MNVG_NONE = 0,
  MNVG_FILL,
  MNVG_CONVEXFILL,
  MNVG_STROKE,
  MNVG_TRIANGLES,
};

struct MNVGblend {
  MTLBlendFactor srcRGB;
  MTLBlendFactor dstRGB;
  MTLBlendFactor srcAlpha;
  MTLBlendFactor dstAlpha;
};
typedef struct MNVGblend MNVGblend;

struct MNVGcall {
  int type;
  int image;
  int pathOffset;
  int pathCount;
  int triangleOffset;
  int triangleCount;
  int indexOffset;
  int indexCount;
  int uniformOffset;
  MNVGblend blendFunc;
};
typedef struct MNVGcall MNVGcall;

struct MNVGpath {
  int fillOffset;
  int fillCount;
  int strokeOffset;
  int strokeCount;
};
typedef struct MNVGpath MNVGpath;

struct MNVGtexture {
  int id;
  id<MTLTexture> tex;
  id<MTLSamplerState> sampler;
  int type;
  int flags;
};
typedef struct MNVGtexture MNVGtexture;

struct MNVGfragUniforms {
  matrix_float3x3 scissorMat;
  matrix_float3x3 paintMat;
  vector_float4 innerCol;
  vector_float4 outerCol;
  vector_float2 scissorExt;
  vector_float2 scissorScale;
  vector_float2 extent;
  float radius;
  float feather;
  float strokeMult;
  float strokeThr;
  int texType;
  MNVGshaderType type;
};
typedef struct MNVGfragUniforms MNVGfragUniforms;

struct MNVGbuffers {
  BOOL isBusy;
  int image;
  id<MTLCommandBuffer> commandBuffer;
  id<MTLBuffer> viewSizeBuffer;
  id<MTLTexture> stencilTexture;
  MNVGcall* calls;
  int ccalls;
  int ncalls;
  MNVGpath* paths;
  int cpaths;
  int npaths;
  id<MTLBuffer> indexBuffer;
  uint32_t* indexes;
  int cindexes;
  int nindexes;
  id<MTLBuffer> vertBuffer;
  struct NVGvertex* verts;
  int cverts;
  int nverts;
  id<MTLBuffer> uniformBuffer;
  unsigned char* uniforms;
  int cuniforms;
  int nuniforms;
};
typedef struct MNVGbuffers MNVGbuffers;

struct MNVGcontext {
  id<MTLCommandQueue> commandQueue;
  CAMetalLayer* metalLayer;
  id <MTLRenderCommandEncoder> renderEncoder;

  int fragSize;
  int indexSize;
  int flags;
  vector_uint2 viewPortSize;
  MTLClearColor clearColor;
  BOOL clearBufferOnFlush;

  // Textures
  MNVGtexture* textures;
  int ntextures;
  int ctextures;
  int textureId;

  // Per frame buffers
  MNVGbuffers* buffers;
  MNVGbuffers* cbuffers;
  int maxBuffers;
  dispatch_semaphore_t semaphore;

  // Cached states.
  MNVGblend blendFunc;
  id<MTLDepthStencilState> defaultStencilState;
  id<MTLDepthStencilState> fillShapeStencilState;
  id<MTLDepthStencilState> fillAntiAliasStencilState;
  id<MTLDepthStencilState> fillStencilState;
  id<MTLDepthStencilState> strokeShapeStencilState;
  id<MTLDepthStencilState> strokeAntiAliasStencilState;
  id<MTLDepthStencilState> strokeClearStencilState;
  id<MTLFunction> fragmentFunction;
  id<MTLFunction> vertexFunction;
  MTLPixelFormat piplelinePixelFormat;
  id<MTLRenderPipelineState> pipelineState;
  id<MTLRenderPipelineState> stencilOnlyPipelineState;
  id<MTLSamplerState> pseudoSampler;
  id<MTLTexture> pseudoTexture;
  MTLVertexDescriptor* vertexDescriptor;
};
typedef struct MNVGcontext MNVGcontext;

const MTLResourceOptions kMetalBufferOptions = \
    (MTLResourceCPUCacheModeWriteCombined | MTLResourceStorageModeShared);

// Keeps the weak reference to the currently binded framebuffer.
MNVGframebuffer* s_framebuffer = NULL;

static int mtlnvg__maxi(int a, int b) { return a > b ? a : b; }

static MNVGcall* mtlnvg__allocCall(MNVGcontext* mtl) {
  MNVGcall* ret = NULL;
  MNVGbuffers* buffers = mtl->buffers;
  if (buffers->ncalls+1 > buffers->ccalls) {
    MNVGcall* calls;
    int ccalls = mtlnvg__maxi(buffers->ncalls + 1, 128) + buffers->ccalls / 2;
    calls = (MNVGcall*)realloc(buffers->calls, sizeof(MNVGcall) * ccalls);
    if (calls == NULL) return NULL;
    buffers->calls = calls;
    buffers->ccalls = ccalls;
  }
  ret = &buffers->calls[buffers->ncalls++];
  memset(ret, 0, sizeof(MNVGcall));
  return ret;
}

static int mtlnvg__allocFragUniforms(MNVGcontext* mtl, int n) {
  int ret = 0;
  MNVGbuffers* buffers = mtl->buffers;
  if (buffers->nuniforms + n > buffers->cuniforms) {
    int cuniforms = mtlnvg__maxi(buffers->nuniforms + n, 128) \
                    + buffers->cuniforms / 2;
    id<MTLBuffer> buffer = [mtl->metalLayer.device
        newBufferWithLength:(mtl->fragSize * cuniforms)
        options:kMetalBufferOptions];
    unsigned char* uniforms = [buffer contents];
    if (buffers->uniformBuffer != nil) {
      memcpy(uniforms, buffers->uniforms,
             mtl->fragSize * buffers->nuniforms);
      [buffers->uniformBuffer release];
    }
    buffers->uniformBuffer = buffer;
    buffers->uniforms = uniforms;
    buffers->cuniforms = cuniforms;
  }
  ret = buffers->nuniforms * mtl->fragSize;
  buffers->nuniforms += n;
  return ret;
}

static int mtlnvg__allocPaths(MNVGcontext* mtl, int n) {
  int ret = 0;
  MNVGbuffers* buffers = mtl->buffers;
  if (buffers->npaths + n > buffers->cpaths) {
    MNVGpath* paths;
    int cpaths = mtlnvg__maxi(buffers->npaths + n, 128) \
                 + buffers->cpaths / 2;
    paths = (MNVGpath*)realloc(buffers->paths, sizeof(MNVGpath) * cpaths);
    if (paths == NULL) return -1;
    buffers->paths = paths;
    buffers->cpaths = cpaths;
  }
  ret = buffers->npaths;
  buffers->npaths += n;
  return ret;
}

static MNVGtexture* mtlnvg__allocTexture(MNVGcontext* mtl) {
  MNVGtexture* tex = NULL;

  for (int i = 0; i < mtl->ntextures; i++) {
    if (mtl->textures[i].id == 0) {
      tex = &mtl->textures[i];
      break;
    }
  }
  if (tex == NULL) {
    if (mtl->ntextures + 1 > mtl->ctextures) {
      int ctextures = mtlnvg__maxi(mtl->ntextures + 1, 4) + mtl->ctextures / 2;
      MNVGtexture* textures = (MNVGtexture*)realloc(
          mtl->textures, sizeof(MNVGtexture) * ctextures);
      if (textures == NULL) return NULL;
      mtl->textures = textures;
      mtl->ctextures = ctextures;
    }
    tex = &mtl->textures[mtl->ntextures++];
  }
  memset(tex, 0, sizeof(MNVGtexture));
  tex->id = ++mtl->textureId;
  return tex;
}

static int mtlnvg__allocIndexes(MNVGcontext* mtl, int n) {
  int ret = 0;
  MNVGbuffers* buffers = mtl->buffers;
  if (buffers->nindexes + n > buffers->cindexes) {
    int cindexes = mtlnvg__maxi(buffers->nindexes + n, 4096) \
                   + buffers->cindexes / 2;
    id<MTLBuffer> buffer = [mtl->metalLayer.device
        newBufferWithLength:(mtl->indexSize * cindexes)
        options:kMetalBufferOptions];
    uint32_t* indexes = [buffer contents];
    if (buffers->indexBuffer != nil) {
      memcpy(indexes, buffers->indexes, mtl->indexSize * buffers->nindexes);
      [buffers->indexBuffer release];
    }
    buffers->indexBuffer = buffer;
    buffers->indexes = indexes;
    buffers->cindexes = cindexes;
  }
  ret = buffers->nindexes;
  buffers->nindexes += n;
  return ret;
}

static int mtlnvg__allocVerts(MNVGcontext* mtl, int n) {
  int ret = 0;
  MNVGbuffers* buffers = mtl->buffers;
  if (buffers->nverts + n > buffers->cverts) {
    int cverts = mtlnvg__maxi(buffers->nverts + n, 4096) \
                 + buffers->cverts / 2;
    id<MTLBuffer> buffer = [mtl->metalLayer.device
        newBufferWithLength:(sizeof(NVGvertex) * cverts)
        options:kMetalBufferOptions];
    NVGvertex* verts = [buffer contents];
    if (buffers->vertBuffer != nil) {
      memcpy(verts, buffers->verts, sizeof(NVGvertex) * buffers->nverts);
      [buffers->vertBuffer release];
    }
    buffers->vertBuffer = buffer;
    buffers->verts = verts;
    buffers->cverts = cverts;
  }
  ret = buffers->nverts;
  buffers->nverts += n;
  return ret;
}

static BOOL mtlnvg_convertBlendFuncFactor(int factor, MTLBlendFactor* result) {
  if (factor == NVG_ZERO)
    *result = MTLBlendFactorZero;
  else if (factor == NVG_ONE)
    *result = MTLBlendFactorOne;
  else if (factor == NVG_SRC_COLOR)
    *result = MTLBlendFactorSourceColor;
  else if (factor == NVG_ONE_MINUS_SRC_COLOR)
    *result = MTLBlendFactorOneMinusSourceColor;
  else if (factor == NVG_DST_COLOR)
    *result = MTLBlendFactorDestinationColor;
  else if (factor == NVG_ONE_MINUS_DST_COLOR)
    *result = MTLBlendFactorOneMinusDestinationColor;
  else if (factor == NVG_SRC_ALPHA)
    *result = MTLBlendFactorSourceAlpha;
  else if (factor == NVG_ONE_MINUS_SRC_ALPHA)
    *result = MTLBlendFactorOneMinusSourceAlpha;
  else if (factor == NVG_DST_ALPHA)
    *result = MTLBlendFactorDestinationAlpha;
  else if (factor == NVG_ONE_MINUS_DST_ALPHA)
    *result = MTLBlendFactorOneMinusDestinationAlpha;
  else if (factor == NVG_SRC_ALPHA_SATURATE)
    *result = MTLBlendFactorSourceAlphaSaturated;
  else
    return NO;
  return YES;
}

static MNVGblend mtlnvg__blendCompositeOperation(NVGcompositeOperationState op) {
  MNVGblend blend;
  if (!mtlnvg_convertBlendFuncFactor(op.srcRGB, &blend.srcRGB) ||
      !mtlnvg_convertBlendFuncFactor(op.dstRGB, &blend.dstRGB) ||
      !mtlnvg_convertBlendFuncFactor(op.srcAlpha, &blend.srcAlpha) ||
      !mtlnvg_convertBlendFuncFactor(op.dstAlpha, &blend.dstAlpha)) {
    blend.srcRGB = MTLBlendFactorOne;
    blend.dstRGB = MTLBlendFactorOneMinusSourceAlpha;
    blend.srcAlpha = MTLBlendFactorOne;
    blend.dstAlpha = MTLBlendFactorOneMinusSourceAlpha;
  }
  return blend;
}

static void mtlnvg__checkError(MNVGcontext* mtl, const char* str,
                               NSError* error) {
  if ((mtl->flags & NVG_DEBUG) == 0) return;
  if (error) {
    printf("Error occurs after %s: %s\n", str, [[error localizedDescription] UTF8String]);
  }
}

static MNVGtexture* mtlnvg__findTexture(MNVGcontext* mtl, int id) {
  for (int i = 0; i < mtl->ntextures; i++) {
    if (mtl->textures[i].id == id)
      return &mtl->textures[i];
  }
  return NULL;
}

static vector_float4 mtlnvg__premulColor(NVGcolor c)
{
  c.r *= c.a;
  c.g *= c.a;
  c.b *= c.a;
  return (vector_float4){c.r, c.g, c.b, c.a};
}

static void mtlnvg__xformToMat3x3(matrix_float3x3* m3, float* t) {
  *m3 = matrix_from_columns((vector_float3){t[0], t[1], 0.0f},
                            (vector_float3){t[2], t[3], 0.0f},
                            (vector_float3){t[4], t[5], 1.0f});
}

static int mtlnvg__convertPaint(MNVGcontext* mtl, MNVGfragUniforms* frag,
                                NVGpaint* paint, NVGscissor* scissor,
                                float width, float fringe, float strokeThr) {
  MNVGtexture* tex = NULL;
  float invxform[6];

  memset(frag, 0, sizeof(*frag));

  frag->innerCol = mtlnvg__premulColor(paint->innerColor);
  frag->outerCol = mtlnvg__premulColor(paint->outerColor);

  if (scissor->extent[0] < -0.5f || scissor->extent[1] < -0.5f) {
    frag->scissorMat = matrix_from_rows((vector_float3){0, 0, 0},
                                        (vector_float3){0, 0, 0},
                                        (vector_float3){0, 0, 0});
    frag->scissorExt.x = 1.0f;
    frag->scissorExt.y = 1.0f;
    frag->scissorScale.x = 1.0f;
    frag->scissorScale.y = 1.0f;
  } else {
    nvgTransformInverse(invxform, scissor->xform);
    mtlnvg__xformToMat3x3(&frag->scissorMat, invxform);
    frag->scissorExt.x = scissor->extent[0];
    frag->scissorExt.y = scissor->extent[1];
    frag->scissorScale.x = sqrtf(scissor->xform[0] * scissor->xform[0] + scissor->xform[2] * scissor->xform[2]) / fringe;
    frag->scissorScale.y = sqrtf(scissor->xform[1] * scissor->xform[1] + scissor->xform[3] * scissor->xform[3]) / fringe;
  }

  frag->extent = (vector_float2){paint->extent[0], paint->extent[1]};
  frag->strokeMult = (width * 0.5f + fringe * 0.5f) / fringe;
  frag->strokeThr = strokeThr;

  if (paint->image != 0) {
    tex = mtlnvg__findTexture(mtl, paint->image);
    if (tex == NULL) return 0;
    if (tex->flags & NVG_IMAGE_FLIPY) {
      float m1[6], m2[6];
      nvgTransformTranslate(m1, 0.0f, frag->extent.y * 0.5f);
      nvgTransformMultiply(m1, paint->xform);
      nvgTransformScale(m2, 1.0f, -1.0f);
      nvgTransformMultiply(m2, m1);
      nvgTransformTranslate(m1, 0.0f, -frag->extent.y * 0.5f);
      nvgTransformMultiply(m1, m2);
      nvgTransformInverse(invxform, m1);
    } else {
      nvgTransformInverse(invxform, paint->xform);
    }
    frag->type = MNVG_SHADER_FILLIMG;

    if (tex->type == NVG_TEXTURE_RGBA)
      frag->texType = (tex->flags & NVG_IMAGE_PREMULTIPLIED) ? 0 : 1;
    else
      frag->texType = 2;
  } else {
    frag->type = MNVG_SHADER_FILLGRAD;
    frag->radius = paint->radius;
    frag->feather = paint->feather;
    nvgTransformInverse(invxform, paint->xform);
  }

  mtlnvg__xformToMat3x3(&frag->paintMat, invxform);

  return 1;
}

static MNVGfragUniforms* mtlnvg__fragUniformPtr(MNVGbuffers* buffers, int i) {
  return (MNVGfragUniforms*)&buffers->uniforms[i];
}

static int mtlnvg__maxVertCount(const NVGpath* paths, int npaths,
                                int* indexCount) {
  int i, count = 0;
  if (indexCount != NULL)
    *indexCount = 0;
  for (i = 0; i < npaths; i++) {
    const int nfill = paths[i].nfill;
    if (nfill > 2) {
      count += nfill;
      if (indexCount != NULL)
        *indexCount += (nfill - 2) * 3;
    }
    count += paths[i].nstroke;
  }
  return count;
}

static id<MTLRenderCommandEncoder> mtlnvg__renderCommandEncoder(
    MNVGcontext* mtl, id <MTLCommandBuffer> commandBuffer,
    id<MTLTexture> colorTexture) {
  MNVGbuffers* buffers = mtl->buffers;

  MTLRenderPassDescriptor *descriptor = \
      [MTLRenderPassDescriptor renderPassDescriptor];
  descriptor.colorAttachments[0].clearColor = mtl->clearColor;
  descriptor.colorAttachments[0].loadAction = \
      mtl->clearBufferOnFlush ? MTLLoadActionClear : MTLLoadActionLoad;
  descriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
  descriptor.colorAttachments[0].texture = colorTexture;
  mtl->clearBufferOnFlush = NO;

  descriptor.stencilAttachment.clearStencil = 0;
  descriptor.stencilAttachment.loadAction = MTLLoadActionClear;
  descriptor.stencilAttachment.storeAction = MTLStoreActionDontCare;
  descriptor.stencilAttachment.texture = buffers->stencilTexture;

  id<MTLRenderCommandEncoder> encoder = [commandBuffer
      renderCommandEncoderWithDescriptor:descriptor];

  [encoder setCullMode:MTLCullModeBack];
  [encoder setFrontFacingWinding:MTLWindingCounterClockwise];
  [encoder setStencilReferenceValue:0];
  [encoder setViewport:(MTLViewport)
      {0.0, 0.0, mtl->viewPortSize.x, mtl->viewPortSize.y, 0.0, 1.0}];

  [encoder setVertexBuffer:buffers->vertBuffer
                    offset:0
                   atIndex:MNVG_VERTEX_INPUT_INDEX_VERTICES];

  [encoder setVertexBuffer:buffers->viewSizeBuffer
                   offset:0
                  atIndex:MNVG_VERTEX_INPUT_INDEX_VIEW_SIZE];

  [encoder setFragmentBuffer:mtl->buffers->uniformBuffer offset:0 atIndex:0];
  return encoder;
}

static void mtlnvg__setUniforms(MNVGcontext* mtl, int uniformOffset,
                                int image) {
  [mtl->renderEncoder setFragmentBufferOffset:uniformOffset atIndex:0];

  MNVGtexture* tex = (image == 0 ? NULL : mtlnvg__findTexture(mtl, image));
  if (tex != NULL) {
    [mtl->renderEncoder setFragmentTexture:tex->tex atIndex:0];
    [mtl->renderEncoder setFragmentSamplerState:tex->sampler atIndex:0];
  } else {
    [mtl->renderEncoder setFragmentTexture:mtl->pseudoTexture atIndex:0];
    [mtl->renderEncoder setFragmentSamplerState:mtl->pseudoSampler atIndex:0];
  }
}

static void mtlnvg__updateRenderPipelineStates(MNVGcontext* mtl,
                                               MNVGblend* blend,
                                               MTLPixelFormat pixelFormat) {
  if (mtl->pipelineState != nil &&
      mtl->stencilOnlyPipelineState != nil &&
      mtl->piplelinePixelFormat == pixelFormat &&
      mtl->blendFunc.srcRGB == blend->srcRGB &&
      mtl->blendFunc.dstRGB == blend->dstRGB &&
      mtl->blendFunc.srcAlpha == blend->srcAlpha &&
      mtl->blendFunc.dstAlpha == blend->dstAlpha) {
    return;
  }

  if (mtl->pipelineState != nil) {
    [mtl->pipelineState release];
    mtl->pipelineState = nil;
  }
  if (mtl->stencilOnlyPipelineState != nil) {
    [mtl->stencilOnlyPipelineState release];
    mtl->stencilOnlyPipelineState = nil;
  }

  MTLRenderPipelineDescriptor *pipelineStateDescriptor = \
      [MTLRenderPipelineDescriptor new];

  MTLRenderPipelineColorAttachmentDescriptor* colorAttachmentDescriptor = \
      pipelineStateDescriptor.colorAttachments[0];
  colorAttachmentDescriptor.pixelFormat = pixelFormat;
  pipelineStateDescriptor.stencilAttachmentPixelFormat = MTLPixelFormatStencil8;
  pipelineStateDescriptor.fragmentFunction = mtl->fragmentFunction;
  pipelineStateDescriptor.vertexFunction = mtl->vertexFunction;
  pipelineStateDescriptor.vertexDescriptor = mtl->vertexDescriptor;

  // Sets blending states.
  colorAttachmentDescriptor.blendingEnabled = YES;
  colorAttachmentDescriptor.sourceRGBBlendFactor = blend->srcRGB;
  colorAttachmentDescriptor.sourceAlphaBlendFactor = blend->srcAlpha;
  colorAttachmentDescriptor.destinationRGBBlendFactor = blend->dstRGB;
  colorAttachmentDescriptor.destinationAlphaBlendFactor = blend->dstAlpha;
  mtl->blendFunc.srcRGB = blend->srcRGB;
  mtl->blendFunc.srcAlpha = blend->srcAlpha;
  mtl->blendFunc.dstRGB = blend->dstRGB;
  mtl->blendFunc.dstAlpha = blend->dstAlpha;

  NSError* error;
  mtl->pipelineState = [mtl->metalLayer.device
      newRenderPipelineStateWithDescriptor:pipelineStateDescriptor
                                     error:&error];
  mtlnvg__checkError(mtl, "init pipeline state", error);

  pipelineStateDescriptor.fragmentFunction = nil;
  colorAttachmentDescriptor.writeMask = MTLColorWriteMaskNone;
  mtl->stencilOnlyPipelineState = [mtl->metalLayer.device
      newRenderPipelineStateWithDescriptor:pipelineStateDescriptor
                                     error:&error];
  mtlnvg__checkError(mtl, "init pipeline stencil only state", error);

  [pipelineStateDescriptor release];
  mtl->piplelinePixelFormat = pixelFormat;
}

// Re-creates stencil texture whenever the specified size is bigger.
static void mtlnvg__updateStencilTexture(MNVGcontext* mtl, vector_uint2* size) {
  if (mtl->buffers->stencilTexture != nil &&
      (mtl->buffers->stencilTexture.width < size->x ||
       mtl->buffers->stencilTexture.height < size->y)) {
    [mtl->buffers->stencilTexture release];
    mtl->buffers->stencilTexture = nil;
  }
  if (mtl->buffers->stencilTexture == nil) {
    MTLTextureDescriptor *stencilTextureDescriptor = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:MTLPixelFormatStencil8
        width:size->x
        height:size->y
        mipmapped:NO];
#if TARGET_OS_OSX == 1
    stencilTextureDescriptor.resourceOptions = MTLResourceStorageModePrivate;
#endif
    stencilTextureDescriptor.usage = MTLTextureUsageRenderTarget;
    mtl->buffers->stencilTexture = [mtl->metalLayer.device
        newTextureWithDescriptor:stencilTextureDescriptor];
  }
}

static void mtlnvg__vset(NVGvertex* vtx, float x, float y, float u, float v) {
  vtx->x = x;
  vtx->y = y;
  vtx->u = u;
  vtx->v = v;
}

static void mtlnvg__fill(MNVGcontext* mtl, MNVGcall* call) {
  MNVGpath* paths = &mtl->buffers->paths[call->pathOffset];
  int i, npaths = call->pathCount;

  // Draws shapes.
  const int kIndexBufferOffset = call->indexOffset * mtl->indexSize;
  [mtl->renderEncoder setCullMode:MTLCullModeNone];
  [mtl->renderEncoder setDepthStencilState:mtl->fillShapeStencilState];
  [mtl->renderEncoder setRenderPipelineState:mtl->stencilOnlyPipelineState];
  if (call->indexCount > 0) {
    [mtl->renderEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                                   indexCount:call->indexCount
                                    indexType:MTLIndexTypeUInt32
                                  indexBuffer:mtl->buffers->indexBuffer
                            indexBufferOffset:kIndexBufferOffset];
  }

  // Restores states.
  [mtl->renderEncoder setCullMode:MTLCullModeBack];
  [mtl->renderEncoder setRenderPipelineState:mtl->pipelineState];

  // Draws anti-aliased fragments.
  mtlnvg__setUniforms(mtl, call->uniformOffset, call->image);
  if (mtl->flags & NVG_ANTIALIAS) {
    [mtl->renderEncoder setDepthStencilState:mtl->fillAntiAliasStencilState];
    for (i = 0; i < npaths; i++) {
      [mtl->renderEncoder drawPrimitives:MTLPrimitiveTypeTriangleStrip
                             vertexStart:paths[i].strokeOffset
                             vertexCount:paths[i].strokeCount];
    }
  }

  // Draws fill.
  [mtl->renderEncoder setDepthStencilState:mtl->fillStencilState];
  [mtl->renderEncoder drawPrimitives:MTLPrimitiveTypeTriangleStrip
                         vertexStart:call->triangleOffset
                         vertexCount:call->triangleCount];

  [mtl->renderEncoder setDepthStencilState:mtl->defaultStencilState];
}

static void mtlnvg__convexFill(MNVGcontext* mtl, MNVGcall* call) {
  MNVGpath* paths = &mtl->buffers->paths[call->pathOffset];
  int i, npaths = call->pathCount;

  const int kIndexBufferOffset = call->indexOffset * mtl->indexSize;
  mtlnvg__setUniforms(mtl, call->uniformOffset, call->image);
  [mtl->renderEncoder setRenderPipelineState:mtl->pipelineState];
  if (call->indexCount > 0) {
    [mtl->renderEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                                   indexCount:call->indexCount
                                    indexType:MTLIndexTypeUInt32
                                  indexBuffer:mtl->buffers->indexBuffer
                            indexBufferOffset:kIndexBufferOffset];
  }

  // Draw fringes
  for (i = 0; i < npaths; i++) {
    if (paths[i].strokeCount > 0) {
      [mtl->renderEncoder drawPrimitives:MTLPrimitiveTypeTriangleStrip
                             vertexStart:paths[i].strokeOffset
                             vertexCount:paths[i].strokeCount];
    }
  }
}

static void mtlnvg__stroke(MNVGcontext* mtl, MNVGcall* call) {
  MNVGpath* paths = &mtl->buffers->paths[call->pathOffset];
  int i, npaths = call->pathCount;

  if (mtl->flags & NVG_STENCIL_STROKES) {
    // Fills the stroke base without overlap.
    mtlnvg__setUniforms(mtl, call->uniformOffset + mtl->fragSize, call->image);
    [mtl->renderEncoder setDepthStencilState:mtl->strokeShapeStencilState];
    [mtl->renderEncoder setRenderPipelineState:mtl->pipelineState];
    for (i = 0; i < npaths; i++) {
      [mtl->renderEncoder drawPrimitives:MTLPrimitiveTypeTriangleStrip
                             vertexStart:paths[i].strokeOffset
                             vertexCount:paths[i].strokeCount];
    }

    // Draws anti-aliased fragments.
    mtlnvg__setUniforms(mtl, call->uniformOffset, call->image);
    [mtl->renderEncoder setDepthStencilState:mtl->strokeAntiAliasStencilState];
    for (i = 0; i < npaths; i++) {
      [mtl->renderEncoder drawPrimitives:MTLPrimitiveTypeTriangleStrip
                             vertexStart:paths[i].strokeOffset
                             vertexCount:paths[i].strokeCount];
    }

    // Clears stencil buffer.
    [mtl->renderEncoder setDepthStencilState:mtl->strokeClearStencilState];
    [mtl->renderEncoder setRenderPipelineState:mtl->stencilOnlyPipelineState];
    for (i = 0; i < npaths; i++) {
      [mtl->renderEncoder drawPrimitives:MTLPrimitiveTypeTriangleStrip
                             vertexStart:paths[i].strokeOffset
                             vertexCount:paths[i].strokeCount];
    }

    [mtl->renderEncoder setDepthStencilState:mtl->defaultStencilState];
  } else {
    // Draws strokes.
    mtlnvg__setUniforms(mtl, call->uniformOffset, call->image);
    [mtl->renderEncoder setRenderPipelineState:mtl->pipelineState];
    for (i = 0; i < npaths; i++) {
      [mtl->renderEncoder drawPrimitives:MTLPrimitiveTypeTriangleStrip
                             vertexStart:paths[i].strokeOffset
                             vertexCount:paths[i].strokeCount];
    }
  }
}

static void mtlnvg__triangles(MNVGcontext* mtl, MNVGcall* call) {
  mtlnvg__setUniforms(mtl, call->uniformOffset, call->image);
  [mtl->renderEncoder setRenderPipelineState:mtl->pipelineState];
  [mtl->renderEncoder drawPrimitives:MTLPrimitiveTypeTriangle
                         vertexStart:call->triangleOffset
                         vertexCount:call->triangleCount];
}

static void mtlnvg__renderCancel(void* uptr) {
  MNVGcontext* mtl = (MNVGcontext*)uptr;
  MNVGbuffers* buffers = mtl->buffers;
  buffers->nindexes = 0;
  buffers->nverts = 0;
  buffers->npaths = 0;
  buffers->ncalls = 0;
  buffers->nuniforms = 0;
}

static int mtlnvg__renderCreateTexture(void* uptr, int type, int width,
                                       int height, int imageFlags,
                                       const unsigned char* data) {
  MNVGcontext* mtl = (MNVGcontext*)uptr;
  MNVGtexture* tex = mtlnvg__allocTexture(mtl);

  if (tex == NULL) return 0;

  MTLPixelFormat pixelFormat = MTLPixelFormatRGBA8Unorm;
  if (type == NVG_TEXTURE_ALPHA) {
    pixelFormat = MTLPixelFormatR8Unorm;
  }

  tex->type = type;
  tex->flags = imageFlags;

  MTLTextureDescriptor *textureDescriptor = [MTLTextureDescriptor
      texture2DDescriptorWithPixelFormat:pixelFormat
      width:width
      height:height
      mipmapped:(imageFlags & NVG_IMAGE_GENERATE_MIPMAPS ? YES : NO)];
#if TARGET_OS_OSX == 1
  textureDescriptor.resourceOptions = MTLResourceStorageModePrivate;
  textureDescriptor.storageMode = MTLStorageModeManaged;
#endif
  textureDescriptor.usage = MTLTextureUsageShaderRead
                            | MTLTextureUsageRenderTarget
                            | MTLTextureUsageShaderWrite;
  tex->tex = [mtl->metalLayer.device
      newTextureWithDescriptor:textureDescriptor];

  if (data != NULL) {
    NSUInteger bytesPerRow;
    if (tex->type == NVG_TEXTURE_RGBA) {
      bytesPerRow = width * 4;
    } else {
      bytesPerRow = width;
    }

    [tex->tex replaceRegion:MTLRegionMake2D(0, 0, width, height)
                mipmapLevel:0
                  withBytes:data
                bytesPerRow:bytesPerRow];

    if (imageFlags & NVG_IMAGE_GENERATE_MIPMAPS) {
      @autoreleasepool {
        id<MTLCommandBuffer> commandBuffer = [mtl->commandQueue commandBuffer];
        id<MTLBlitCommandEncoder> encoder = [commandBuffer blitCommandEncoder];
        [encoder generateMipmapsForTexture:tex->tex];
        [encoder endEncoding];
        [commandBuffer commit];
        [commandBuffer waitUntilCompleted];
      }
    }
  }

  MTLSamplerDescriptor* samplerDescriptor = [MTLSamplerDescriptor new];
  if (imageFlags & NVG_IMAGE_NEAREST) {
    samplerDescriptor.minFilter = MTLSamplerMinMagFilterNearest;
    samplerDescriptor.magFilter = MTLSamplerMinMagFilterNearest;
    if (imageFlags & NVG_IMAGE_GENERATE_MIPMAPS)
      samplerDescriptor.mipFilter = MTLSamplerMipFilterNearest;
  } else {
    samplerDescriptor.minFilter = MTLSamplerMinMagFilterLinear;
    samplerDescriptor.magFilter = MTLSamplerMinMagFilterLinear;
    if (imageFlags & NVG_IMAGE_GENERATE_MIPMAPS)
      samplerDescriptor.mipFilter = MTLSamplerMipFilterLinear;
  }

  if (imageFlags & NVG_IMAGE_REPEATX) {
    samplerDescriptor.sAddressMode = MTLSamplerAddressModeRepeat;
  } else {
    samplerDescriptor.sAddressMode = MTLSamplerAddressModeClampToEdge;
  }

  if (imageFlags & NVG_IMAGE_REPEATY) {
    samplerDescriptor.tAddressMode = MTLSamplerAddressModeRepeat;
  } else {
    samplerDescriptor.tAddressMode = MTLSamplerAddressModeClampToEdge;
  }

  tex->sampler = [mtl->metalLayer.device
      newSamplerStateWithDescriptor:samplerDescriptor];
  [samplerDescriptor release];

  return tex->id;
}

static int mtlnvg__renderCreate(void* uptr) {
  MNVGcontext* mtl = (MNVGcontext*)uptr;

  if (mtl->metalLayer.device == nil) {
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    mtl->metalLayer.device = device;
    [device release];
  }
  mtl->metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
#if TARGET_OS_OSX == 1
  mtl->metalLayer.opaque = NO;
#endif

  // Loads shaders from pre-compiled metal library..
  NSError* error;
  id<MTLDevice> device = mtl->metalLayer.device;
#ifdef MNVG_INVALID_TARGET
  id<MTLLibrary> library = nil;
  return 0;
#endif

  bool creates_pseudo_texture = false;
  unsigned char* metal_library_bitcode;
  unsigned int metal_library_bitcode_len;
  NSOperatingSystemVersion os_version = [[NSProcessInfo processInfo]
      operatingSystemVersion];
#if TARGET_OS_IOS == 1
  if (os_version.majorVersion < 8) {
    return 0;
  } else if (os_version.majorVersion == 8) {
    creates_pseudo_texture = true;
    metal_library_bitcode = mnvg_bitcode_ios_1_0;
    metal_library_bitcode_len = mnvg_bitcode_ios_1_0_len;
  } else if (os_version.majorVersion == 9) {
    creates_pseudo_texture = true;
    metal_library_bitcode = mnvg_bitcode_ios_1_1;
    metal_library_bitcode_len = mnvg_bitcode_ios_1_1_len;
  } else if (os_version.majorVersion == 10) {
    metal_library_bitcode = mnvg_bitcode_ios_1_2;
    metal_library_bitcode_len = mnvg_bitcode_ios_1_2_len;
  } else if (os_version.majorVersion == 11) {
    metal_library_bitcode = mnvg_bitcode_ios_2_0;
    metal_library_bitcode_len = mnvg_bitcode_ios_2_0_len;
  } else {
    metal_library_bitcode = mnvg_bitcode_ios_2_1;
    metal_library_bitcode_len = mnvg_bitcode_ios_2_1_len;
  }
#elif TARGET_OS_TV == 1
  metal_library_bitcode = mnvg_bitcode_tvos;
  metal_library_bitcode_len = mnvg_bitcode_tvos_len;
#elif TARGET_OS_OSX == 1
  if (os_version.majorVersion < 10) {
    return 0;
  }
  if (os_version.majorVersion == 10 && os_version.minorVersion < 11) {
    return 0;
  }
  if (os_version.minorVersion == 11) {
    metal_library_bitcode = mnvg_bitcode_macos_1_1;
    metal_library_bitcode_len = mnvg_bitcode_macos_1_1_len;
  } else if (os_version.minorVersion == 12) {
    metal_library_bitcode = mnvg_bitcode_macos_1_2;
    metal_library_bitcode_len = mnvg_bitcode_macos_1_2_len;
  } else if (os_version.minorVersion == 13) {
    metal_library_bitcode = mnvg_bitcode_macos_1_2;
    metal_library_bitcode_len = mnvg_bitcode_macos_1_2_len;
  } else {
    metal_library_bitcode = mnvg_bitcode_macos_2_0;
    metal_library_bitcode_len = mnvg_bitcode_macos_2_0_len;
  }
  creates_pseudo_texture = true;
#endif

  dispatch_data_t data = dispatch_data_create(metal_library_bitcode,
                                              metal_library_bitcode_len,
                                              NULL,
                                              DISPATCH_DATA_DESTRUCTOR_DEFAULT);
  id<MTLLibrary> library = [device newLibraryWithData:data error:&error];
  [data release];

  mtlnvg__checkError(mtl, "init library", error);
  if (library == nil) {
    return 0;
  }

  mtl->vertexFunction = [library newFunctionWithName:@"vertexShader"];
  if (mtl->flags & NVG_ANTIALIAS) {
    if (mtl->flags & NVG_STENCIL_STROKES) {
      mtl->fragmentFunction = \
          [library newFunctionWithName:@"fragmentShaderStencilStrokesAA"];
    } else {
      mtl->fragmentFunction = [library newFunctionWithName:@"fragmentShaderAA"];
    }
  } else {
    mtl->fragmentFunction = [library newFunctionWithName:@"fragmentShader"];
  }
  [library release];

  mtl->commandQueue = [device newCommandQueue];

  // Initializes the number of available buffers.
  if (mtl->flags & NVG_TRIPLE_BUFFER) {
    mtl->maxBuffers = 3;
  } else if (mtl->flags & NVG_DOUBLE_BUFFER) {
    mtl->maxBuffers = 2;
  } else {
    mtl->maxBuffers = 1;
  }
  const int kBufferSize = sizeof(MNVGbuffers) * mtl->maxBuffers;
  mtl->cbuffers = malloc(kBufferSize);
  memset(mtl->cbuffers, 0, kBufferSize);
  mtl->buffers = mtl->cbuffers;
  mtl->buffers->isBusy = YES;
  
//  mtl->semaphore = dispatch_semaphore_create(mtl->maxBuffers - 1);
//  https://lists.apple.com/archives/cocoa-dev/2014/Apr/msg00484.html
  mtl->semaphore = dispatch_semaphore_create(0);
  for (int i=0; i<mtl->maxBuffers - 1; i++) {
    dispatch_semaphore_signal(mtl->semaphore);
  }
  
  // Initializes vertex descriptor.
  mtl->vertexDescriptor = [MTLVertexDescriptor vertexDescriptor];
  [mtl->vertexDescriptor retain];
  mtl->vertexDescriptor.attributes[0].format = MTLVertexFormatFloat2;
  mtl->vertexDescriptor.attributes[0].bufferIndex = 0;
  mtl->vertexDescriptor.attributes[0].offset = offsetof(NVGvertex, x);

  mtl->vertexDescriptor.attributes[1].format = MTLVertexFormatFloat2;
  mtl->vertexDescriptor.attributes[1].bufferIndex = 0;
  mtl->vertexDescriptor.attributes[1].offset = offsetof(NVGvertex, u);

  mtl->vertexDescriptor.layouts[0].stride = sizeof(NVGvertex);
  mtl->vertexDescriptor.layouts[0].stepFunction = \
      MTLVertexStepFunctionPerVertex;

  // Initializes default sampler descriptor.
  MTLSamplerDescriptor* samplerDescriptor = [MTLSamplerDescriptor new];
  mtl->pseudoSampler = [mtl->metalLayer.device
      newSamplerStateWithDescriptor:samplerDescriptor];
  [samplerDescriptor release];

  // Initializes pseudo texture for macOS.
  if (creates_pseudo_texture) {
    const int kPseudoTextureImage = mtlnvg__renderCreateTexture(
        mtl, NVG_TEXTURE_ALPHA, 1, 1, 0, NULL);
    MNVGtexture* tex = mtlnvg__findTexture(mtl, kPseudoTextureImage);
    mtl->pseudoTexture = tex->tex;
  }

  // Initializes default blend states.
  mtl->blendFunc.srcRGB = MTLBlendFactorOne;
  mtl->blendFunc.srcAlpha = MTLBlendFactorOne;
  mtl->blendFunc.dstRGB = MTLBlendFactorOneMinusSourceAlpha;
  mtl->blendFunc.dstAlpha = MTLBlendFactorOneMinusSourceAlpha;

  // Initializes stencil states.
  MTLDepthStencilDescriptor* stencilDescriptor = \
      [MTLDepthStencilDescriptor new];

  // Default stencil state.
  mtl->defaultStencilState = [device
      newDepthStencilStateWithDescriptor:stencilDescriptor];

  // Fill shape stencil.
  MTLStencilDescriptor* frontFaceStencilDescriptor = [MTLStencilDescriptor new];
  frontFaceStencilDescriptor.stencilCompareFunction = MTLCompareFunctionAlways;
  frontFaceStencilDescriptor.depthStencilPassOperation = \
      MTLStencilOperationIncrementWrap;

  MTLStencilDescriptor* backFaceStencilDescriptor = [MTLStencilDescriptor new];
  backFaceStencilDescriptor.stencilCompareFunction = MTLCompareFunctionAlways;
  backFaceStencilDescriptor.depthStencilPassOperation = \
      MTLStencilOperationDecrementWrap;

  stencilDescriptor.depthCompareFunction = MTLCompareFunctionAlways;
  stencilDescriptor.backFaceStencil = backFaceStencilDescriptor;
  stencilDescriptor.frontFaceStencil = frontFaceStencilDescriptor;
  mtl->fillShapeStencilState = [device
      newDepthStencilStateWithDescriptor:stencilDescriptor];

  // Fill anti-aliased stencil.
  frontFaceStencilDescriptor.stencilCompareFunction = MTLCompareFunctionEqual;
  frontFaceStencilDescriptor.stencilFailureOperation = MTLStencilOperationKeep;
  frontFaceStencilDescriptor.depthFailureOperation = MTLStencilOperationKeep;
  frontFaceStencilDescriptor.depthStencilPassOperation = \
      MTLStencilOperationZero;

  stencilDescriptor.backFaceStencil = nil;
  stencilDescriptor.frontFaceStencil = frontFaceStencilDescriptor;
  mtl->fillAntiAliasStencilState = [device
      newDepthStencilStateWithDescriptor:stencilDescriptor];

  // Fill stencil.
  frontFaceStencilDescriptor.stencilCompareFunction = \
      MTLCompareFunctionNotEqual;
  frontFaceStencilDescriptor.stencilFailureOperation = MTLStencilOperationZero;
  frontFaceStencilDescriptor.depthFailureOperation = MTLStencilOperationZero;
  frontFaceStencilDescriptor.depthStencilPassOperation = \
      MTLStencilOperationZero;

  stencilDescriptor.backFaceStencil = nil;
  stencilDescriptor.frontFaceStencil = frontFaceStencilDescriptor;
  mtl->fillStencilState = [device
      newDepthStencilStateWithDescriptor:stencilDescriptor];

  // Stroke shape stencil.
  frontFaceStencilDescriptor.stencilCompareFunction = MTLCompareFunctionEqual;
  frontFaceStencilDescriptor.stencilFailureOperation = MTLStencilOperationKeep;
  frontFaceStencilDescriptor.depthFailureOperation = MTLStencilOperationKeep;
  frontFaceStencilDescriptor.depthStencilPassOperation = \
      MTLStencilOperationIncrementClamp;

  stencilDescriptor.backFaceStencil = nil;
  stencilDescriptor.frontFaceStencil = frontFaceStencilDescriptor;
  mtl->strokeShapeStencilState = [device
      newDepthStencilStateWithDescriptor:stencilDescriptor];

  // Stroke anti-aliased stencil.
  frontFaceStencilDescriptor.depthStencilPassOperation = \
      MTLStencilOperationKeep;

  stencilDescriptor.backFaceStencil = nil;
  stencilDescriptor.frontFaceStencil = frontFaceStencilDescriptor;
  mtl->strokeAntiAliasStencilState = [device
      newDepthStencilStateWithDescriptor:stencilDescriptor];

  // Stroke clear stencil.
  frontFaceStencilDescriptor.stencilCompareFunction = MTLCompareFunctionAlways;
  frontFaceStencilDescriptor.stencilFailureOperation = MTLStencilOperationZero;
  frontFaceStencilDescriptor.depthFailureOperation = MTLStencilOperationZero;
  frontFaceStencilDescriptor.depthStencilPassOperation = \
      MTLStencilOperationZero;

  stencilDescriptor.backFaceStencil = nil;
  stencilDescriptor.frontFaceStencil = frontFaceStencilDescriptor;
  mtl->strokeClearStencilState = [device
      newDepthStencilStateWithDescriptor:stencilDescriptor];

  [frontFaceStencilDescriptor release];
  [backFaceStencilDescriptor release];
  [stencilDescriptor release];

  mtl->clearBufferOnFlush = NO;

  return 1;
}

static void mtlnvg__renderDelete(void* uptr) {
  MNVGcontext* mtl = (MNVGcontext*)uptr;
  dispatch_release(mtl->semaphore);

  [mtl->commandQueue release];
  [mtl->defaultStencilState release];
  [mtl->fragmentFunction release];
  [mtl->vertexDescriptor release];
  [mtl->vertexFunction release];
  [mtl->fillShapeStencilState release];
  [mtl->fillAntiAliasStencilState release];
  [mtl->fillStencilState release];
  [mtl->strokeShapeStencilState release];
  [mtl->strokeAntiAliasStencilState release];
  [mtl->strokeClearStencilState release];
  [mtl->pipelineState release];
  [mtl->stencilOnlyPipelineState release];
  [mtl->pseudoSampler release];

  for (int i = 0; i < mtl->maxBuffers; ++i) {
    MNVGbuffers* buffers = &(mtl->cbuffers[i]);
    [buffers->viewSizeBuffer release];
    [buffers->stencilTexture release];
    [buffers->indexBuffer release];
    [buffers->vertBuffer release];
    [buffers->uniformBuffer release];
    free(buffers->paths);
    free(buffers->calls);
  }
  free(mtl->cbuffers);

  for (int i = 0; i < mtl->ntextures; i++) {
    if (mtl->textures[i].tex != nil &&
        (mtl->textures[i].flags & NVG_IMAGE_NODELETE) == 0) {
      [mtl->textures[i].tex release];
      [mtl->textures[i].sampler release];
    }
  }
  free(mtl->textures);

  [mtl->metalLayer.device release];
  mtl->metalLayer.device = nil;
  mtl->metalLayer = nil;
  free(mtl);
}

static int mtlnvg__renderDeleteTexture(void* uptr, int image) {
  MNVGcontext* mtl = (MNVGcontext*)uptr;
  for (int i = 0; i < mtl->ntextures; i++) {
    if (mtl->textures[i].id == image) {
      if (mtl->textures[i].tex != nil &&
          (mtl->textures[i].flags & NVG_IMAGE_NODELETE) == 0) {
        [mtl->textures[i].tex release];
        [mtl->textures[i].sampler release];
      }
      memset(&mtl->textures[i], 0, sizeof(MNVGtexture));
      return 1;
    }
  }
  return 0;
}

static void mtlnvg__renderFill(void* uptr, NVGpaint* paint,
                              NVGcompositeOperationState compositeOperation,
                              NVGscissor* scissor, float fringe,
                              const float* bounds, const NVGpath* paths,
                              int npaths) {
  MNVGcontext* mtl = (MNVGcontext*)uptr;
  MNVGcall* call = mtlnvg__allocCall(mtl);
  NVGvertex* quad;
  int i, maxindexes, maxverts, indexOffset, vertOffset, hubVertOffset;

  if (call == NULL) return;

  call->type = MNVG_FILL;
  call->triangleCount = 4;
  call->pathOffset = mtlnvg__allocPaths(mtl, npaths);
  if (call->pathOffset == -1) goto error;
  call->pathCount = npaths;
  call->image = paint->image;
  call->blendFunc = mtlnvg__blendCompositeOperation(compositeOperation);

  if (npaths == 1 && paths[0].convex)
  {
    call->type = MNVG_CONVEXFILL;
    call->triangleCount = 0;  // Bounding box fill quad not needed for convex fill
  }

  // Allocate vertices for all the paths.
  maxverts = mtlnvg__maxVertCount(paths, npaths, &maxindexes)
             + call->triangleCount;
  vertOffset = mtlnvg__allocVerts(mtl, maxverts);
  if (vertOffset == -1) goto error;

  indexOffset = mtlnvg__allocIndexes(mtl, maxindexes);
  if (indexOffset == -1) goto error;
  call->indexOffset = indexOffset;
  call->indexCount = maxindexes;
  uint32_t* index = &mtl->buffers->indexes[indexOffset];

  for (i = 0; i < npaths; i++) {
    MNVGpath* copy = &mtl->buffers->paths[call->pathOffset + i];
    const NVGpath* path = &paths[i];
    memset(copy, 0, sizeof(MNVGpath));
    if (path->nfill > 2) {
      copy->fillOffset = vertOffset;
      copy->fillCount = path->nfill;
      memcpy(&mtl->buffers->verts[vertOffset], path->fill,
             sizeof(NVGvertex) * path->nfill);

      hubVertOffset = vertOffset++;
      for (int j = 2; j < path->nfill; j++) {
        *index++ = hubVertOffset;
        *index++ = vertOffset++;
        *index++ = vertOffset;
      }
      vertOffset++;
    }
    if (path->nstroke > 0) {
      copy->strokeOffset = vertOffset;
      copy->strokeCount = path->nstroke;
      memcpy(&mtl->buffers->verts[vertOffset], path->stroke,
             sizeof(NVGvertex) * path->nstroke);
      vertOffset += path->nstroke;
    }
  }

  // Setup uniforms for draw calls
  if (call->type == MNVG_FILL) {
    // Quad
    call->triangleOffset = vertOffset;
    quad = &mtl->buffers->verts[call->triangleOffset];
    mtlnvg__vset(&quad[0], bounds[2], bounds[3], 0.5f, 1.0f);
    mtlnvg__vset(&quad[1], bounds[2], bounds[1], 0.5f, 1.0f);
    mtlnvg__vset(&quad[2], bounds[0], bounds[3], 0.5f, 1.0f);
    mtlnvg__vset(&quad[3], bounds[0], bounds[1], 0.5f, 1.0f);
  }

  // Fill shader
  call->uniformOffset = mtlnvg__allocFragUniforms(mtl, 1);
  if (call->uniformOffset == -1) goto error;
  mtlnvg__convertPaint(mtl,
                       mtlnvg__fragUniformPtr(mtl->buffers,
                                              call->uniformOffset),
                       paint, scissor, fringe, fringe, -1.0f);

  return;

error:
  // We get here if call alloc was ok, but something else is not.
  // Roll back the last call to prevent drawing it.
  if (mtl->buffers->ncalls > 0) mtl->buffers->ncalls--;
}

static void mtlnvg__renderFlush(void* uptr) {
  MNVGcontext* mtl = (MNVGcontext*)uptr;
  MNVGbuffers* buffers = mtl->buffers;
  id <MTLCommandBuffer> commandBuffer = [mtl->commandQueue commandBuffer];
  id<MTLTexture> colorTexture = nil;;
  vector_uint2 textureSize;

  mtl->metalLayer.drawableSize = CGSizeMake(mtl->viewPortSize.x,
                                            mtl->viewPortSize.y);

  buffers->commandBuffer = commandBuffer;
  @autoreleasepool {
    [buffers->commandBuffer enqueue];
    [buffers->commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> buffer) {
        buffers->isBusy = NO;
        buffers->commandBuffer = nil;
        buffers->image = 0;
        buffers->nindexes = 0;
        buffers->nverts = 0;
        buffers->npaths = 0;
        buffers->ncalls = 0;
        buffers->nuniforms = 0;
        dispatch_semaphore_signal(mtl->semaphore);
    }];

    if (s_framebuffer == NULL ||
        nvgInternalParams(s_framebuffer->ctx)->userPtr != mtl) {
      textureSize = mtl->viewPortSize;
    } else {  // renders in framebuffer
      buffers->image = s_framebuffer->image;
      MNVGtexture* tex = mtlnvg__findTexture(mtl, s_framebuffer->image);
      colorTexture = tex->tex;
      textureSize = (vector_uint2){(uint)colorTexture.width,
                                   (uint)colorTexture.height};
    }
    if (textureSize.x == 0 || textureSize.y == 0) return;
    mtlnvg__updateStencilTexture(mtl, &textureSize);

    id<CAMetalDrawable> drawable = nil;
    if (colorTexture == nil) {
      drawable = mtl->metalLayer.nextDrawable;
      colorTexture = drawable.texture;
    }
    mtl->renderEncoder = mtlnvg__renderCommandEncoder(mtl,
                                                      buffers->commandBuffer,
                                                      colorTexture);
    for (int i = 0; i < buffers->ncalls; i++) {
      MNVGcall* call = &buffers->calls[i];

      MNVGblend* blend = &call->blendFunc;
      mtlnvg__updateRenderPipelineStates(mtl, blend, colorTexture.pixelFormat);

      if (call->type == MNVG_FILL)
        mtlnvg__fill(mtl, call);
      else if (call->type == MNVG_CONVEXFILL)
        mtlnvg__convexFill(mtl, call);
      else if (call->type == MNVG_STROKE)
        mtlnvg__stroke(mtl, call);
      else if (call->type == MNVG_TRIANGLES)
        mtlnvg__triangles(mtl, call);
    }

    [mtl->renderEncoder endEncoding];
    mtl->renderEncoder = nil;
    if (drawable) {
      [buffers->commandBuffer presentDrawable:drawable];
    }
    [buffers->commandBuffer commit];
  }  // @autoreleasepool

  dispatch_semaphore_wait(mtl->semaphore, DISPATCH_TIME_FOREVER);
  for (int i = 0; i < mtl->maxBuffers; ++i) {
    buffers = &mtl->cbuffers[i];
    if (!buffers->isBusy) {
      buffers->isBusy = YES;
      mtl->buffers = buffers;
      break;
    }
  }
}

static int mtlnvg__renderGetTextureSize(void* uptr, int image, int* w, int* h) {
  MNVGcontext* mtl = (MNVGcontext*)uptr;
  MNVGtexture* tex = mtlnvg__findTexture(mtl, image);
  if (tex == NULL) return 0;
  *w = (int)tex->tex.width;
  *h = (int)tex->tex.height;
  return 1;
}

static void mtlnvg__renderStroke(void* uptr, NVGpaint* paint,
                                 NVGcompositeOperationState compositeOperation,
                                 NVGscissor* scissor, float fringe,
                                 float strokeWidth, const NVGpath* paths,
                                 int npaths) {
  MNVGcontext* mtl = (MNVGcontext*)uptr;
  MNVGcall* call = mtlnvg__allocCall(mtl);
  int i, maxverts, offset;

  if (call == NULL) return;

  call->type = MNVG_STROKE;
  call->pathOffset = mtlnvg__allocPaths(mtl, npaths);
  if (call->pathOffset == -1) goto error;
  call->pathCount = npaths;
  call->image = paint->image;
  call->blendFunc = mtlnvg__blendCompositeOperation(compositeOperation);

  // Allocate vertices for all the paths.
  maxverts = mtlnvg__maxVertCount(paths, npaths, NULL);
  offset = mtlnvg__allocVerts(mtl, maxverts);
  if (offset == -1) goto error;

  for (i = 0; i < npaths; i++) {
    MNVGpath* copy = &mtl->buffers->paths[call->pathOffset + i];
    const NVGpath* path = &paths[i];
    memset(copy, 0, sizeof(MNVGpath));
    if (path->nstroke > 0) {
      copy->strokeOffset = offset;
      copy->strokeCount = path->nstroke;
      memcpy(&mtl->buffers->verts[offset], path->stroke,
             sizeof(NVGvertex) * path->nstroke);
      offset += path->nstroke;
    }
  }

  if (mtl->flags & NVG_STENCIL_STROKES) {
    // Fill shader
    call->uniformOffset = mtlnvg__allocFragUniforms(mtl, 2);
    if (call->uniformOffset == -1) goto error;
    mtlnvg__convertPaint(mtl,
                         mtlnvg__fragUniformPtr(mtl->buffers,
                                                call->uniformOffset),
                         paint, scissor, strokeWidth, fringe, -1.0f);
    MNVGfragUniforms* frag = \
        mtlnvg__fragUniformPtr(mtl->buffers,
                               call->uniformOffset + mtl->fragSize);
    mtlnvg__convertPaint(mtl, frag, paint, scissor, strokeWidth, fringe,
                         1.0f - 0.5f / 255.0f);
  } else {
    // Fill shader
    call->uniformOffset = mtlnvg__allocFragUniforms(mtl, 1);
    if (call->uniformOffset == -1) goto error;
    mtlnvg__convertPaint(mtl,
                         mtlnvg__fragUniformPtr(mtl->buffers,
                                                call->uniformOffset),
                         paint, scissor, strokeWidth, fringe, -1.0f);
  }

  return;

error:
  // We get here if call alloc was ok, but something else is not.
  // Roll back the last call to prevent drawing it.
  if (mtl->buffers->ncalls > 0) mtl->buffers->ncalls--;
}

static void mtlnvg__renderTriangles(
    void* uptr, NVGpaint* paint, NVGcompositeOperationState compositeOperation,
    NVGscissor* scissor, const NVGvertex* verts, int nverts) {
  MNVGcontext* mtl = (MNVGcontext*)uptr;
  MNVGcall* call = mtlnvg__allocCall(mtl);
  MNVGfragUniforms* frag;

  if (call == NULL) return;

  call->type = MNVG_TRIANGLES;
  call->image = paint->image;
  call->blendFunc = mtlnvg__blendCompositeOperation(compositeOperation);

  // Allocate vertices for all the paths.
  call->triangleOffset = mtlnvg__allocVerts(mtl, nverts);
  if (call->triangleOffset == -1) goto error;
  call->triangleCount = nverts;

  memcpy(&mtl->buffers->verts[call->triangleOffset], verts,
         sizeof(NVGvertex) * nverts);

  // Fill shader
  call->uniformOffset = mtlnvg__allocFragUniforms(mtl, 1);
  if (call->uniformOffset == -1) goto error;
  frag = mtlnvg__fragUniformPtr(mtl->buffers, call->uniformOffset);
  mtlnvg__convertPaint(mtl, frag, paint, scissor, 1.0f, 1.0f, -1.0f);
  frag->type = MNVG_SHADER_IMG;

  return;

error:
  // We get here if call alloc was ok, but something else is not.
  // Roll back the last call to prevent drawing it.
  if (mtl->buffers->ncalls > 0) mtl->buffers->ncalls--;
}

static int mtlnvg__renderUpdateTexture(void* uptr, int image, int x, int y,
                                       int w, int h,
                                       const unsigned char* data) {
  MNVGcontext* mtl = (MNVGcontext*)uptr;
  MNVGtexture* tex = mtlnvg__findTexture(mtl, image);

  if (tex == NULL) return 0;
  id<MTLTexture> texture = tex->tex;

  unsigned char* bytes;
  NSUInteger bytesPerRow;
  if (tex->type == NVG_TEXTURE_RGBA) {
    bytesPerRow = tex->tex.width * 4;
    bytes = (unsigned char*)data + y * bytesPerRow + x * 4;
  } else {
    bytesPerRow = tex->tex.width;
    bytes = (unsigned char*)data + y * bytesPerRow + x;
  }
  [texture replaceRegion:MTLRegionMake2D(x, y, w, h)
             mipmapLevel:0
               withBytes:bytes
             bytesPerRow:bytesPerRow];

  return 1;
}

static void mtlnvg__renderViewport(void* uptr, float width, float height,
                                   float devicePixelRatio) {
  MNVGcontext* mtl = (MNVGcontext*)uptr;
  mtl->viewPortSize = (vector_uint2){width * devicePixelRatio,
                                     height * devicePixelRatio};

  // Initializes view size buffer for vertex function.
  if (mtl->buffers->viewSizeBuffer == nil) {
    mtl->buffers->viewSizeBuffer = [mtl->metalLayer.device
        newBufferWithLength:sizeof(vector_float2)
        options:kMetalBufferOptions];
  }
  float* viewSize = (float*)[mtl->buffers->viewSizeBuffer contents];
  viewSize[0] = width;
  viewSize[1] = height;
}

NVGcontext* nvgCreateMTL(void* metalLayer, int flags) {
#if TARGET_OS_SIMULATOR == 1
  printf("Metal is not supported for iPhone Simulator.\n");
  return NULL;
#elif defined(MNVG_INVALID_TARGET)
  printf("Metal is only supported on iOS, macOS, and tvOS.\n");
  return NULL;
#endif

  NVGparams params;
  NVGcontext* ctx = NULL;
  MNVGcontext* mtl = (MNVGcontext *)malloc(sizeof(MNVGcontext));
  if (mtl == NULL) goto error;
  memset(mtl, 0, sizeof(MNVGcontext));

  memset(&params, 0, sizeof(params));
  params.renderCreate = mtlnvg__renderCreate;
  params.renderCreateTexture = mtlnvg__renderCreateTexture;
  params.renderDeleteTexture = mtlnvg__renderDeleteTexture;
  params.renderUpdateTexture = mtlnvg__renderUpdateTexture;
  params.renderGetTextureSize = mtlnvg__renderGetTextureSize;
  params.renderViewport = mtlnvg__renderViewport;
  params.renderCancel = mtlnvg__renderCancel;
  params.renderFlush = mtlnvg__renderFlush;
  params.renderFill = mtlnvg__renderFill;
  params.renderStroke = mtlnvg__renderStroke;
  params.renderTriangles = mtlnvg__renderTriangles;
  params.renderDelete = mtlnvg__renderDelete;
  params.userPtr = mtl;
  params.edgeAntiAlias = flags & NVG_ANTIALIAS ? 1 : 0;

  mtl->flags = flags;
#if TARGET_OS_OSX == 1
  mtl->fragSize = 256;
#else
  mtl->fragSize = sizeof(MNVGfragUniforms);
#endif
  mtl->indexSize = 4;  // MTLIndexTypeUInt32
  mtl->metalLayer = (__bridge CAMetalLayer*)metalLayer;

  ctx = nvgCreateInternal(&params);
  if (ctx == NULL) goto error;
  return ctx;

error:
  // 'mtl' is freed by nvgDeleteInternal.
  if (ctx != NULL) nvgDeleteInternal(ctx);
  return NULL;
}

void nvgDeleteMTL(NVGcontext* ctx) {
  nvgDeleteInternal(ctx);
}

void mnvgBindFramebuffer(MNVGframebuffer* framebuffer) {
  s_framebuffer = framebuffer;
}

MNVGframebuffer* mnvgCreateFramebuffer(NVGcontext* ctx, int width,
                                       int height, int imageFlags) {
  MNVGframebuffer* framebuffer = \
      (MNVGframebuffer*)malloc(sizeof(MNVGframebuffer));
  if (framebuffer == NULL)
    return NULL;

  memset(framebuffer, 0, sizeof(MNVGframebuffer));
  framebuffer->image = nvgCreateImageRGBA(ctx, width, height,
                                          imageFlags | NVG_IMAGE_PREMULTIPLIED,
                                          NULL);
  framebuffer->ctx = ctx;
  return framebuffer;
}

void mnvgDeleteFramebuffer(MNVGframebuffer* framebuffer) {
  if (framebuffer == NULL) return;
  if (framebuffer->image > 0) {
    nvgDeleteImage(framebuffer->ctx, framebuffer->image);
  }
  free(framebuffer);
}

void mnvgClearWithColor(NVGcontext* ctx, NVGcolor color) {
  MNVGcontext* mtl = (MNVGcontext*)nvgInternalParams(ctx)->userPtr;
  float alpha = (float)color.a;
  mtl->clearColor = MTLClearColorMake((float)color.r * alpha,
                                      (float)color.g * alpha,
                                      (float)color.b * alpha,
                                      (float)color.a);
  mtl->clearBufferOnFlush = YES;
}

void* mnvgCommandQueue(NVGcontext* ctx) {
  MNVGcontext* mtl = (MNVGcontext*)nvgInternalParams(ctx)->userPtr;
  return (__bridge void*)mtl->commandQueue;
}

void* mnvgDevice(NVGcontext* ctx) {
  MNVGcontext* mtl = (MNVGcontext*)nvgInternalParams(ctx)->userPtr;
  return (__bridge void*)mtl->metalLayer.device;
}

void* mnvgImageHandle(NVGcontext* ctx, int image) {
  MNVGcontext* mtl = (MNVGcontext*)nvgInternalParams(ctx)->userPtr;
  MNVGtexture* tex = mtlnvg__findTexture(mtl, image);
  if (tex == NULL) return NULL;

  // Makes sure the command execution for the image has been done.
  for (int i = 0; i < mtl->maxBuffers; ++i) {
    MNVGbuffers* buffers = &mtl->cbuffers[i];
    if (buffers->isBusy && buffers->image == image && buffers->commandBuffer) {
      [buffers->commandBuffer waitUntilCompleted];
      break;
    }
  }

  return (__bridge void*)tex->tex;
}

void mnvgReadPixels(NVGcontext* ctx, int image, int x, int y, int width,
                    int height, void* data) {
  MNVGcontext* mtl = (MNVGcontext*)nvgInternalParams(ctx)->userPtr;
  MNVGtexture* tex = mtlnvg__findTexture(mtl, image);
  if (tex == NULL) return;

  NSUInteger bytesPerRow;
  if (tex->type == NVG_TEXTURE_RGBA) {
    bytesPerRow = tex->tex.width * 4;
  } else {
    bytesPerRow = tex->tex.width;
  }

  // Makes sure the command execution for the image has been done.
  for (int i = 0; i < mtl->maxBuffers; ++i) {
    MNVGbuffers* buffers = &mtl->cbuffers[i];
    if (buffers->isBusy && buffers->image == image && buffers->commandBuffer) {
      [buffers->commandBuffer waitUntilCompleted];
      break;
    }
  }

  [tex->tex getBytes:data
         bytesPerRow:bytesPerRow
          fromRegion:MTLRegionMake2D(x, y, width, height)
         mipmapLevel:0];
}
