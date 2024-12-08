/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include "INanoVGShaderControl.h"
#import <Metal/Metal.h>

using namespace iplug;
using namespace igraphics;

#if defined IGRAPHICS_METAL

id<MTLFunction> CompileMetalShader(id<MTLDevice> device,
                                   const std::string &name,
                                   const std::string &type_str,
                                   const std::string &src) {
  if (src.empty())
    return nil;
  
  id<MTLLibrary> library = nil;
  NSError *error = nil;
  std::string activity;
  if (src.size() > 4 && strncmp(src.data(), "MTLB", 4) == 0) {
    dispatch_data_t data = dispatch_data_create(
                                                src.data(), src.size(), NULL, DISPATCH_DATA_DESTRUCTOR_DEFAULT);
    library = [device newLibraryWithData:data error:&error];
    activity = "load";
  } else {
    NSString *str = [NSString stringWithUTF8String: src.c_str()];
    MTLCompileOptions *opts = [MTLCompileOptions new];
    library = [device newLibraryWithSource:str options:opts error:&error];
    activity = "compile";
  }
  if (error) {
    const char *error_shader = [[error description] UTF8String];
    throw std::runtime_error(
                             std::string("compile_metal_shader(): unable to ") + activity + " " +
                             type_str + " shader \"" + name + "\":\n\n" + error_shader);
  }
  
  NSArray<NSString *> *function_names = [library functionNames];
  if ([function_names count] != 1)
    throw std::runtime_error("compile_metal_shader(name=\"" + name +
                             "\"): library must contain exactly 1 shader!");
  NSString *function_name = [function_names objectAtIndex: 0];
  
  id<MTLFunction> function = [library newFunctionWithName: function_name];
  if (!function)
    throw std::runtime_error("compile_metal_shader(name=\"" + name +
                             "\"): function not found!");
  
  return function;
}

void INanoVGShaderControl::OnAttached()
{
  auto* pCtx = reinterpret_cast<NVGcontext*>(GetUI()->GetDrawContext());
  id<MTLDevice> device = (__bridge id<MTLDevice>) mnvgDevice(pCtx);
  std::string name = "MyNewShader";
  id<MTLFunction> fragmentFunc = CompileMetalShader(device, name, "fragment", std::string(mFragmentShaderStr.Get()));
  id<MTLFunction> vertexFunc = CompileMetalShader(device, name, "vertex", std::string(mVertexShaderStr.Get()));
  
  NSError* error;

  MTLRenderPipelineDescriptor* psd = [[MTLRenderPipelineDescriptor alloc] init];
  psd.label = @"Offscreen Render Pipeline";
  psd.sampleCount = 1;
  psd.vertexFunction =  vertexFunc;
  psd.fragmentFunction =  fragmentFunc;
  psd.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
  mRenderPipeline = [device newRenderPipelineStateWithDescriptor:psd error:&error];
}
#include <simd/simd.h>

typedef struct
{
  vector_float2 position;
  vector_float4 color;
} SimpleVertex;

void INanoVGShaderControl::Draw(IGraphics& g)
{
  PreDraw(g);

  auto* pCtx = static_cast<NVGcontext*>(g.GetDrawContext());
  
  auto w = mRECT.W() * g.GetTotalScale();
  auto h = mRECT.H() * g.GetTotalScale();

  if (mInvalidateFBO)
  {
    mInvalidateFBO = false;

    mFBO = nvgCreateFramebuffer(pCtx, w, h, 0);
    auto dstTex = static_cast<id<MTLTexture>>(mnvgImageHandle(pCtx, mFBO->image));
    
    MTLRenderPassDescriptor* rpd = [[MTLRenderPassDescriptor alloc] init];
    rpd.colorAttachments[0].texture = dstTex;
    rpd.colorAttachments[0].loadAction = MTLLoadActionClear;
    rpd.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 0);
    rpd.colorAttachments[0].storeAction = MTLStoreActionStore;
    
    mRenderPassDescriptor = (void*) rpd;
  }

  @autoreleasepool {

  auto commandQueue = static_cast<id<MTLCommandQueue>>(mnvgCommandQueue(pCtx));

  id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];

  commandBuffer.label = @"Command Buffer";

  {
//    static const float posAndTexCoords[] = {
//    //     x,     y,    u,   v
//          -1.0f, -1.0f, 0.0, 0.0,
//           1.0f, -1.0f, 1.0, 0.0,
//           1.0f,  1.0f, 1.0, 1.0,
//          -1.0f,  1.0f, 0.0, 1.0,
//    };
    
    static const SimpleVertex triVertices[] =
    {
      // Positions     ,  Colors
      { {  0.5,  -0.5 },  { 1.0, 0.0, 0.0, 1.0 } },
      { { -0.5,  -0.5 },  { 0.0, 1.0, 0.0, 1.0 } },
      { {  0.0,   0.5 },  { 0.0, 0.0, 1.0, 1.0 } },
    };
    
    id<MTLRenderCommandEncoder> renderEncoder =
      [commandBuffer renderCommandEncoderWithDescriptor:(MTLRenderPassDescriptor*) mRenderPassDescriptor];
  
    renderEncoder.label = @"Offscreen Render Pass";
    [renderEncoder setRenderPipelineState:(id<MTLRenderPipelineState>) mRenderPipeline];

    [renderEncoder setVertexBytes:&triVertices length:sizeof(triVertices) atIndex:0];

    [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];

    // End encoding commands for this render pass.
    [renderEncoder endEncoding];
  }

  [commandBuffer commit];
  [commandBuffer waitUntilCompleted];
    
//  DrawToFBO(w, h);
    
//  nvgEndFrame(vg);
//  glBindFramebuffer(GL_FRAMEBUFFER, mInitialFBO);
//  nvgBeginFrame(vg, static_cast<float>(g.WindowWidth()),
//                    static_cast<float>(g.WindowHeight()),
//                    static_cast<float>(g.GetScreenScale()));

  APIBitmap apibmp {mFBO->image, int(w), int(h), 1, 1.};
  IBitmap bmp {&apibmp, 1, false};
  
  g.DrawFittedBitmap(bmp, mRECT);
    
  PostDraw(g);

  } // @autoreleasepool
}

#endif
