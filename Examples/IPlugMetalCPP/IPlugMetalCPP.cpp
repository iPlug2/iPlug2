#include "IPlugMetalCPP.h"
#include "IPlug_include_in_plug_src.h"

#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define MTK_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#include <Metal/Metal.hpp>
#include <MetalKit/MetalKit.hpp>
#include <AppKit/AppKit.hpp>
#include <simd/simd.h>

class Renderer
{
public:
  Renderer( MTL::Device* pDevice );
  ~Renderer();
  void buildShaders();
  void buildBuffers();
  void draw( MTK::View* pView );

private:
  MTL::Device* mpDevice;
  MTL::CommandQueue* mpCommandQueue;
  MTL::RenderPipelineState* mpPSO;
  MTL::Buffer* mpVertexPositionsBuffer;
  MTL::Buffer* mpVertexColorsBuffer;
};

class MyMTKViewDelegate : public MTK::ViewDelegate
{
public:
  MyMTKViewDelegate( MTL::Device* pDevice );
  virtual ~MyMTKViewDelegate() override;
  virtual void drawInMTKView( MTK::View* pView ) override;

private:
  Renderer* mpRenderer;
};


IPlugMetalCPP::IPlugMetalCPP(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kGain)->InitDouble("Gain", 0., 0., 100.0, 0.01, "%");
}

void* IPlugMetalCPP::OpenWindow(void* pParent)
{
  mpAutoreleasePool = NS::AutoreleasePool::alloc()->init();
  mpDevice = MTL::CreateSystemDefaultDevice();
  NS::View* pParentNsView = (NS::View*) pParent;

  mpMtkView = MTK::View::alloc()->init( pParentNsView->frame(), mpDevice );
  mpMtkView->setColorPixelFormat( MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB );
  mpMtkView->setClearColor( MTL::ClearColor::Make( 1.0, 0.0, 0.0, 1.0 ) );

  mpViewDelegate = new MyMTKViewDelegate( mpDevice );
  mpMtkView->setDelegate( mpViewDelegate );

  pParentNsView->addSubview(mpMtkView);
  
  return mpMtkView;
}

void IPlugMetalCPP::CloseWindow()
{
  mpMtkView->removeFromSuperview();
  mpMtkView->release();
  mpDevice->release();
  
  delete mpViewDelegate;

//  mpAutoreleasePool->release();
}

void IPlugMetalCPP::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kGain)->Value() / 100.;
  const int nChans = NOutChansConnected();
  
  for (int s = 0; s < nFrames; s++) {
  for (int c = 0; c < nChans; c++) {
    outputs[c][s] = inputs[c][s] * gain;
  }
  }
}

#pragma mark - ViewDelegate
#pragma region ViewDelegate {

MyMTKViewDelegate::MyMTKViewDelegate( MTL::Device* pDevice )
: MTK::ViewDelegate()
, mpRenderer( new Renderer( pDevice ) )
{
}

MyMTKViewDelegate::~MyMTKViewDelegate()
{
  delete mpRenderer;
}

void MyMTKViewDelegate::drawInMTKView( MTK::View* pView )
{
  mpRenderer->draw( pView );
}

#pragma endregion ViewDelegate }


#pragma mark - Renderer
#pragma region Renderer {

Renderer::Renderer( MTL::Device* pDevice )
: mpDevice( pDevice->retain() )
{
  mpCommandQueue = mpDevice->newCommandQueue();
  buildShaders();
  buildBuffers();
}

Renderer::~Renderer()
{
  mpVertexPositionsBuffer->release();
  mpVertexColorsBuffer->release();
  mpPSO->release();
  mpCommandQueue->release();
  mpDevice->release();
}

void Renderer::buildShaders()
{
  using NS::StringEncoding::UTF8StringEncoding;

  const char* shaderSrc = R"(
    #include <metal_stdlib>
    using namespace metal;

    struct v2f
    {
      float4 position [[position]];
      half3 color;
    };

    v2f vertex vertexMain( uint vertexId [[vertex_id]],
                 device const float3* positions [[buffer(0)]],
                 device const float3* colors [[buffer(1)]] )
    {
      v2f o;
      o.position = float4( positions[ vertexId ], 1.0 );
      o.color = half3 ( colors[ vertexId ] );
      return o;
    }

    half4 fragment fragmentMain( v2f in [[stage_in]] )
    {
      return half4( in.color, 1.0 );
    }
  )";

  NS::Error* pError = nullptr;
  MTL::Library* pLibrary = mpDevice->newLibrary( NS::String::string(shaderSrc, UTF8StringEncoding), nullptr, &pError );
  if ( !pLibrary )
  {
    __builtin_printf( "%s", pError->localizedDescription()->utf8String() );
    assert( false );
  }

  MTL::Function* pVertexFn = pLibrary->newFunction( NS::String::string("vertexMain", UTF8StringEncoding) );
  MTL::Function* pFragFn = pLibrary->newFunction( NS::String::string("fragmentMain", UTF8StringEncoding) );

  MTL::RenderPipelineDescriptor* pDesc = MTL::RenderPipelineDescriptor::alloc()->init();
  pDesc->setVertexFunction( pVertexFn );
  pDesc->setFragmentFunction( pFragFn );
  pDesc->colorAttachments()->object(0)->setPixelFormat( MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB );

  mpPSO = mpDevice->newRenderPipelineState( pDesc, &pError );
  if ( !mpPSO )
  {
    __builtin_printf( "%s", pError->localizedDescription()->utf8String() );
    assert( false );
  }

  pVertexFn->release();
  pFragFn->release();
  pDesc->release();
  pLibrary->release();
}

void Renderer::buildBuffers()
{
  const size_t NumVertices = 3;

  simd::float3 positions[NumVertices] =
  {
    { -0.8f,  0.8f, 0.0f },
    {  0.0f, -0.8f, 0.0f },
    { +0.8f,  0.8f, 0.0f }
  };

  simd::float3 colors[NumVertices] =
  {
    {  1.0, 0.3f, 0.2f },
    {  0.8f, 1.0, 0.0f },
    {  0.8f, 0.0f, 1.0 }
  };

  const size_t positionsDataSize = NumVertices * sizeof( simd::float3 );
  const size_t colorDataSize = NumVertices * sizeof( simd::float3 );

  MTL::Buffer* pVertexPositionsBuffer = mpDevice->newBuffer( positionsDataSize, MTL::ResourceStorageModeManaged );
  MTL::Buffer* pVertexColorsBuffer = mpDevice->newBuffer( colorDataSize, MTL::ResourceStorageModeManaged );

  mpVertexPositionsBuffer = pVertexPositionsBuffer;
  mpVertexColorsBuffer = pVertexColorsBuffer;

  memcpy( mpVertexPositionsBuffer->contents(), positions, positionsDataSize );
  memcpy( mpVertexColorsBuffer->contents(), colors, colorDataSize );

  mpVertexPositionsBuffer->didModifyRange( NS::Range::Make( 0, mpVertexPositionsBuffer->length() ) );
  mpVertexColorsBuffer->didModifyRange( NS::Range::Make( 0, mpVertexColorsBuffer->length() ) );
}

void Renderer::draw( MTK::View* pView )
{
  NS::AutoreleasePool* pPool = NS::AutoreleasePool::alloc()->init();

  MTL::CommandBuffer* pCmd = mpCommandQueue->commandBuffer();
  MTL::RenderPassDescriptor* pRpd = pView->currentRenderPassDescriptor();
  MTL::RenderCommandEncoder* pEnc = pCmd->renderCommandEncoder( pRpd );

  pEnc->setRenderPipelineState( mpPSO );
  pEnc->setVertexBuffer( mpVertexPositionsBuffer, 0, 0 );
  pEnc->setVertexBuffer( mpVertexColorsBuffer, 0, 1 );
  pEnc->drawPrimitives( MTL::PrimitiveType::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(3) );

  pEnc->endEncoding();
  pCmd->presentDrawable( pView->currentDrawable() );
  pCmd->commit();

  pPool->release();
}

#pragma endregion Renderer }
