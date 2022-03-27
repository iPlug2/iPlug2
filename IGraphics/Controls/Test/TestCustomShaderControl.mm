#if defined IGRAPHICS_NANOVG && defined IGRAPHICS_METAL

#include "TestCustomShaderControl.h"
#include "nanovg_mtl.h"
#import <Metal/Metal.h>
#import "ShaderTypes.h"

TestCustomShaderControl::~TestCustomShaderControl()
{
  CleanUp();
}

void TestCustomShaderControl::CleanUp()
{
  if (mFBO)
    nvgDeleteFramebuffer(mFBO);
  
  if (mRenderPassDescriptor)
  {
    MTLRenderPassDescriptor* rpd = (MTLRenderPassDescriptor*) mRenderPassDescriptor;
    [rpd release];
    mRenderPassDescriptor = nullptr;
  }
  

  if (mRenderPipeline)
  {
    id<MTLRenderPipelineState> renderPipeline = (id<MTLRenderPipelineState>) mRenderPipeline;
    [renderPipeline release];
    mRenderPipeline = nullptr;
  }
}

void TestCustomShaderControl::Draw(IGraphics& g)
{
  g.DrawDottedRect(COLOR_BLACK, mRECT);
  g.FillRect(mMouseIsOver ? COLOR_TRANSLUCENT : COLOR_TRANSPARENT, mRECT);

  auto* pCtx = static_cast<NVGcontext*>(g.GetDrawContext());
  
  auto w = mRECT.W() * g.GetTotalScale();
  auto h = mRECT.H() * g.GetTotalScale();
    
  if (invalidateFBO)
  {
    CleanUp();
    
    invalidateFBO = false;

    NSError *error;

    mFBO = nvgCreateFramebuffer(pCtx, w, h, 0);
    auto dev = static_cast<id<MTLDevice>>(mnvgDevice(pCtx));
    auto dstTex = static_cast<id<MTLTexture>>(mnvgImageHandle(pCtx, mFBO->image));
    
    auto rpd = (MTLRenderPassDescriptor*) mRenderPassDescriptor;

    // Set up a render pass descriptor for the render pass to render into
    rpd = [MTLRenderPassDescriptor new];

    rpd.colorAttachments[0].texture = dstTex;

    rpd.colorAttachments[0].loadAction = MTLLoadActionClear;
    rpd.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 0);

    rpd.colorAttachments[0].storeAction = MTLStoreActionStore;

    id<MTLLibrary> defaultLibrary = [dev newDefaultLibrary];

    MTLRenderPipelineDescriptor* psd = [[MTLRenderPipelineDescriptor alloc] init];
    psd.label = @"Offscreen Render Pipeline";
    psd.sampleCount = 1;
    psd.vertexFunction =  [defaultLibrary newFunctionWithName:@"simpleVertexShader"];
    psd.fragmentFunction =  [defaultLibrary newFunctionWithName:@"simpleFragmentShader"];
    psd.colorAttachments[0].pixelFormat = dstTex.pixelFormat;
    mRenderPipeline = [dev newRenderPipelineStateWithDescriptor:psd error:&error];
    [psd release];
    [defaultLibrary release];
    mRenderPassDescriptor = (void*) rpd;
  }

  @autoreleasepool {

  auto commandQueue = static_cast<id<MTLCommandQueue>>(mnvgCommandQueue(pCtx));

  id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];

  commandBuffer.label = @"Command Buffer";

  {
    static const SimpleVertex triVertices[] =
    {
      // Positions     ,  Colors
      { {  0.5,  -0.5 },  { 1.0, 0.0, 0.0, 1.0 } },
      { { -0.5,  -0.5 },  { 0.0, 1.0, 0.0, 1.0 } },
      { {  0.0,   0.5 },  { 0.0, 0.0, 1.0, 1.0 } },
    };

    id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:(MTLRenderPassDescriptor*) mRenderPassDescriptor];
  
    renderEncoder.label = @"Offscreen Render Pass";
    [renderEncoder setRenderPipelineState:(id<MTLRenderPipelineState>) mRenderPipeline];

    [renderEncoder setVertexBytes:&triVertices
                           length:sizeof(triVertices)
                          atIndex:VertexInputIndexVertices];

    [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangle
                      vertexStart:0
                      vertexCount:3];

    // End encoding commands for this render pass.
    [renderEncoder endEncoding];
  }

  [commandBuffer commit];
  [commandBuffer waitUntilCompleted];

  
  APIBitmap apibmp {mFBO->image, int(w), int(h), 1, 1.};
  IBitmap bmp {&apibmp, 1, false};
  
  g.DrawFittedBitmap(bmp, mRECT);

  } // @autoreleasepool
}

#endif // IGRAPHICS_METAL
