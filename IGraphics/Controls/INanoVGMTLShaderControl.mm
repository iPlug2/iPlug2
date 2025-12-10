/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#if defined(IGRAPHICS_NANOVG) && defined(IGRAPHICS_METAL)

#include "INanoVGMTLShaderControl.h"
#include "nanovg_mtl.h"

#import <Metal/Metal.h>

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

namespace {

/** Uniform structure matching the expected layout in Metal shaders */
struct ShaderUniforms
{
  float time;
  float padding1;  // Align to float2
  float resolution[2];
  float mouse[2];
  float mouseButtons[2];
};

} // anonymous namespace

INanoVGMTLShaderControl::INanoVGMTLShaderControl(const IRECT& bounds,
                                                   const char* vertexFuncName,
                                                   const char* fragmentFuncName,
                                                   bool animate)
: IShaderControlBase(bounds, animate)
, mVertexFuncName(vertexFuncName)
, mFragmentFuncName(fragmentFuncName)
, mAnimate(animate)
, mUseSourceString(false)
{
}

INanoVGMTLShaderControl::INanoVGMTLShaderControl(const IRECT& bounds,
                                                   const char* shaderSource,
                                                   const char* vertexFuncName,
                                                   const char* fragmentFuncName,
                                                   bool animate)
: IShaderControlBase(bounds, animate)
, mVertexFuncName(vertexFuncName)
, mFragmentFuncName(fragmentFuncName)
, mAnimate(animate)
, mUseSourceString(true)
{
  mShaderSource.Set(shaderSource);
}

INanoVGMTLShaderControl::~INanoVGMTLShaderControl()
{
  CleanupMTL();
}

void INanoVGMTLShaderControl::CleanupMTL()
{
  if (mFBO)
  {
    nvgDeleteFramebuffer(mFBO);
    mFBO = nullptr;
  }

  if (mRenderPassDesc)
  {
    MTLRenderPassDescriptor* rpd = (__bridge_transfer MTLRenderPassDescriptor*)mRenderPassDesc;
    mRenderPassDesc = nullptr;
    (void)rpd; // Release via ARC
  }

  if (mRenderPipeline)
  {
    id<MTLRenderPipelineState> pipeline = (__bridge_transfer id<MTLRenderPipelineState>)mRenderPipeline;
    mRenderPipeline = nullptr;
    (void)pipeline;
  }

  if (mUniformBuffer)
  {
    id<MTLBuffer> buffer = (__bridge_transfer id<MTLBuffer>)mUniformBuffer;
    mUniformBuffer = nullptr;
    (void)buffer;
  }

  // Don't release device/commandQueue - they belong to NanoVG context
  mDevice = nullptr;
  mCommandQueue = nullptr;
}

bool INanoVGMTLShaderControl::SetupPipeline(WDL_String& error)
{
  if (!mDevice)
  {
    error.Set("Metal device not available");
    return false;
  }

  id<MTLDevice> device = (__bridge id<MTLDevice>)mDevice;
  NSError* nsError = nil;
  id<MTLLibrary> library = nil;

  if (mUseSourceString && mShaderSource.GetLength() > 0)
  {
    // Compile shader from source string at runtime
    NSString* source = [NSString stringWithUTF8String:mShaderSource.Get()];
    MTLCompileOptions* options = [[MTLCompileOptions alloc] init];
    library = [device newLibraryWithSource:source options:options error:&nsError];
    [options release];

    if (!library)
    {
      error.SetFormatted(1024, "Metal shader compilation failed: %s",
                         [[nsError localizedDescription] UTF8String]);
      return false;
    }
  }
  else
  {
    // Get default library (compiled from .metal files in bundle)
    library = [device newDefaultLibrary];

    if (!library)
    {
      error.Set("Failed to load default Metal library");
      return false;
    }
  }

  // Get shader functions
  NSString* vertexName = [NSString stringWithUTF8String:mVertexFuncName];
  NSString* fragmentName = [NSString stringWithUTF8String:mFragmentFuncName];

  id<MTLFunction> vertexFunc = [library newFunctionWithName:vertexName];
  if (!vertexFunc)
  {
    error.SetFormatted(256, "Vertex function '%s' not found", mVertexFuncName);
    [library release];
    return false;
  }

  id<MTLFunction> fragmentFunc = [library newFunctionWithName:fragmentName];
  if (!fragmentFunc)
  {
    error.SetFormatted(256, "Fragment function '%s' not found", mFragmentFuncName);
    [vertexFunc release];
    [library release];
    return false;
  }

  // Create render pipeline descriptor
  MTLRenderPipelineDescriptor* pipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];
  pipelineDesc.label = @"IShaderControl Pipeline";
  pipelineDesc.vertexFunction = vertexFunc;
  pipelineDesc.fragmentFunction = fragmentFunc;
  pipelineDesc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
  pipelineDesc.colorAttachments[0].blendingEnabled = YES;
  pipelineDesc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
  pipelineDesc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;

  // Create pipeline state
  id<MTLRenderPipelineState> pipeline = [device newRenderPipelineStateWithDescriptor:pipelineDesc
                                                                               error:&nsError];
  [pipelineDesc release];
  [vertexFunc release];
  [fragmentFunc release];
  [library release];

  if (!pipeline)
  {
    error.SetFormatted(512, "Pipeline creation failed: %s",
                       [[nsError localizedDescription] UTF8String]);
    return false;
  }

  // Store pipeline (transfer ownership)
  if (mRenderPipeline)
  {
    id<MTLRenderPipelineState> old = (__bridge_transfer id<MTLRenderPipelineState>)mRenderPipeline;
    (void)old;
  }
  mRenderPipeline = (__bridge_retained void*)pipeline;

  // Create uniform buffer
  id<MTLBuffer> uniformBuffer = [device newBufferWithLength:sizeof(ShaderUniforms)
                                                    options:MTLResourceStorageModeShared];
  if (mUniformBuffer)
  {
    id<MTLBuffer> old = (__bridge_transfer id<MTLBuffer>)mUniformBuffer;
    (void)old;
  }
  mUniformBuffer = (__bridge_retained void*)uniformBuffer;

  return true;
}

void INanoVGMTLShaderControl::Draw(IGraphics& g)
{
  // No shader loaded - show placeholder
  if (!mVertexFuncName || !mFragmentFuncName)
  {
    g.FillRect(COLOR_DARK_GRAY, mRECT);
    g.DrawText(IText(14, COLOR_GRAY), "No shader", mRECT);
    return;
  }

  if (mAnimate || IsDirty())
    UpdateTime();

  NVGcontext* vg = static_cast<NVGcontext*>(g.GetDrawContext());
  int w = static_cast<int>(mRECT.W() * g.GetTotalScale());
  int h = static_cast<int>(mRECT.H() * g.GetTotalScale());

  // Get Metal device from NanoVG context
  if (!mDevice)
  {
    mDevice = mnvgDevice(vg);
    mCommandQueue = mnvgCommandQueue(vg);
  }

  if (!mDevice || !mCommandQueue)
    return;

  // Setup pipeline on first draw
  if (mNeedsSetup)
  {
    WDL_String err;
    if (!SetupPipeline(err))
    {
      DBGMSG("INanoVGMTLShaderControl: %s\n", err.Get());
      mNeedsSetup = false;
      mSetupFailed = true;
      return;
    }
    mNeedsSetup = false;
    mSetupFailed = false;
  }

  // Shader failed to load
  if (mSetupFailed)
  {
    g.FillRect(COLOR_DARK_GRAY, mRECT);
    g.DrawText(IText(14, COLOR_RED), "Shader error", mRECT);
    return;
  }

  // Recreate FBO if needed
  if (mInvalidateFBO || !mFBO)
  {
    if (mFBO)
      nvgDeleteFramebuffer(mFBO);

    mFBO = nvgCreateFramebuffer(vg, w, h, 0);

    if (!mFBO)
      return;

    // Setup render pass descriptor
    id<MTLTexture> texture = (__bridge id<MTLTexture>)mnvgImageHandle(vg, mFBO->image);

    if (mRenderPassDesc)
    {
      MTLRenderPassDescriptor* old = (__bridge_transfer MTLRenderPassDescriptor*)mRenderPassDesc;
      (void)old;
    }

    MTLRenderPassDescriptor* rpd = [MTLRenderPassDescriptor new];
    rpd.colorAttachments[0].texture = texture;
    rpd.colorAttachments[0].loadAction = MTLLoadActionClear;
    rpd.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 0);
    rpd.colorAttachments[0].storeAction = MTLStoreActionStore;
    mRenderPassDesc = (__bridge_retained void*)rpd;

    mInvalidateFBO = false;
  }

  if (!mRenderPipeline || !mRenderPassDesc || !mUniformBuffer)
    return;

  @autoreleasepool {
    id<MTLCommandQueue> commandQueue = (__bridge id<MTLCommandQueue>)mCommandQueue;
    id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
    commandBuffer.label = @"IShaderControl Command Buffer";

    // Update uniforms
    id<MTLBuffer> uniformBuffer = (__bridge id<MTLBuffer>)mUniformBuffer;
    ShaderUniforms* uniforms = (ShaderUniforms*)[uniformBuffer contents];
    uniforms->time = mUniforms[static_cast<int>(EShaderUniform::Time)];
    uniforms->resolution[0] = static_cast<float>(w);
    uniforms->resolution[1] = static_cast<float>(h);
    uniforms->mouse[0] = mUniforms[static_cast<int>(EShaderUniform::MouseX)];
    uniforms->mouse[1] = mUniforms[static_cast<int>(EShaderUniform::MouseY)];
    uniforms->mouseButtons[0] = mUniforms[static_cast<int>(EShaderUniform::MouseL)];
    uniforms->mouseButtons[1] = mUniforms[static_cast<int>(EShaderUniform::MouseR)];

    // Create render encoder
    MTLRenderPassDescriptor* rpd = (__bridge MTLRenderPassDescriptor*)mRenderPassDesc;
    id<MTLRenderCommandEncoder> encoder = [commandBuffer renderCommandEncoderWithDescriptor:rpd];
    encoder.label = @"IShaderControl Render";

    id<MTLRenderPipelineState> pipeline = (__bridge id<MTLRenderPipelineState>)mRenderPipeline;
    [encoder setRenderPipelineState:pipeline];

    // Fullscreen quad vertices
    static const float quadVertices[] = {
      -1.0f, -1.0f,
       1.0f, -1.0f,
      -1.0f,  1.0f,
       1.0f,  1.0f,
    };

    [encoder setVertexBytes:quadVertices length:sizeof(quadVertices) atIndex:0];
    [encoder setFragmentBuffer:uniformBuffer offset:0 atIndex:0];
    [encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
    [encoder endEncoding];

    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];
  }

  // Draw FBO as bitmap
  APIBitmap apibmp{mFBO->image, w, h, 1, 1.};
  IBitmap bmp{&apibmp, 1, false};
  g.DrawFittedBitmap(bmp, mRECT);
}

bool INanoVGMTLShaderControl::SetShaderStr(const char* shaderStr, WDL_String& error)
{
  if (!shaderStr || strlen(shaderStr) == 0)
  {
    error.Set("Empty shader source");
    return false;
  }

  mShaderSource.Set(shaderStr);
  mUseSourceString = true;

  // Clear existing pipeline to force recreation
  if (mRenderPipeline)
  {
    id<MTLRenderPipelineState> old = (__bridge_transfer id<MTLRenderPipelineState>)mRenderPipeline;
    mRenderPipeline = nullptr;
    (void)old;
  }

  mNeedsSetup = true;
  mSetupFailed = false;
  SetDirty(true);
  return true;
}

bool INanoVGMTLShaderControl::SetShaderFunctions(const char* vertexFuncName,
                                                   const char* fragmentFuncName,
                                                   WDL_String& error)
{
  mVertexFuncName = vertexFuncName;
  mFragmentFuncName = fragmentFuncName;

  // Clear existing pipeline to force recreation
  if (mRenderPipeline)
  {
    id<MTLRenderPipelineState> old = (__bridge_transfer id<MTLRenderPipelineState>)mRenderPipeline;
    mRenderPipeline = nullptr;
    (void)old;
  }

  mNeedsSetup = true;
  mSetupFailed = false;
  SetDirty(true);
  return true;
}

bool INanoVGMTLShaderControl::IsShaderValid() const
{
  return mRenderPipeline != nullptr;
}

void INanoVGMTLShaderControl::OnResize()
{
  IShaderControlBase::OnResize();
  mInvalidateFBO = true;
}

void INanoVGMTLShaderControl::OnRescale()
{
  mInvalidateFBO = true;
}

void INanoVGMTLShaderControl::OnShaderResize(int w, int h)
{
  mInvalidateFBO = true;
}

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE

#endif // IGRAPHICS_NANOVG && IGRAPHICS_METAL
