#include "MetalRenderer.h"

#include "IPlugPlatform.h"

#define NS_PRIVATE_IMPLEMENTATION
#define UI_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define MTK_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION

#if defined OS_MAC
#include <AppKit/AppKit.hpp>
#define PLATFORM_VIEW NS::View
#elif defined OS_IOS
#include <UIKit/UIKit.hpp>
#define PLATFORM_VIEW UI::View
#else
#error "macOS or iOS only!"
#endif

#include <Metal/Metal.hpp>
#include <MetalKit/MetalKit.hpp>
#include <simd/simd.h>

class CustomMTKViewDelegate : public MTK::ViewDelegate
{
public:
  CustomMTKViewDelegate(MTL::Device* pDevice, const char* bundleID);
  virtual ~CustomMTKViewDelegate() override;
  virtual void drawInMTKView(MTK::View* pView) override;

private:
  Renderer* mpRenderer;
};

MetalUI::~MetalUI()
{
  mpMtkView->removeFromSuperview();
  mpMtkView->release();
  mpDevice->release();
  
  delete mpViewDelegate;

//  mpAutoreleasePool->release();
}

void* MetalUI::OpenWindow(void* pParent, const char* bundleID)
{
  mpAutoreleasePool = NS::AutoreleasePool::alloc()->init();
  mpDevice = MTL::CreateSystemDefaultDevice();
  PLATFORM_VIEW* pParentView = (PLATFORM_VIEW*) pParent;

  mpMtkView = MTK::View::alloc()->init(pParentView->frame(), mpDevice);
  mpMtkView->setColorPixelFormat(MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB);
  mpMtkView->setClearColor(MTL::ClearColor::Make(1.0, 0.0, 0.0, 1.0));
  
  mpViewDelegate = new CustomMTKViewDelegate(mpDevice, bundleID);
  mpMtkView->setDelegate(mpViewDelegate);

  pParentView->addSubview(mpMtkView);
  
  return mpMtkView;
}

void MetalUI::setSize(int width, int height)
{
  mpMtkView->setFrame({0.0, 0.0, CGFloat(width), CGFloat(height)});
}


#pragma mark - ViewDelegate
#pragma region ViewDelegate {

CustomMTKViewDelegate::CustomMTKViewDelegate(MTL::Device* pDevice, const char* bundleID)
: MTK::ViewDelegate()
, mpRenderer(new Renderer(pDevice, bundleID))
{
}

CustomMTKViewDelegate::~CustomMTKViewDelegate()
{
  delete mpRenderer;
}

void CustomMTKViewDelegate::drawInMTKView(MTK::View* pView)
{
  mpRenderer->draw(pView);
}

#pragma endregion ViewDelegate }


#pragma mark - Renderer
#pragma region Renderer {

Renderer::Renderer(MTL::Device* pDevice, const char* bundleID)
: mpDevice(pDevice->retain())
{
  mpCommandQueue = mpDevice->newCommandQueue();
  buildShaders(bundleID);
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

void Renderer::buildShaders(const char* bundleID)
{
  using NS::StringEncoding::UTF8StringEncoding;
  NS::Error* pError = nullptr;

  MTL::Library* pLibrary = mpDevice->newDefaultLibrary(NS::Bundle::bundleWithIdentifier(NS::String::string(bundleID, UTF8StringEncoding)), &pError);
  
  if (!pLibrary)
  {
    __builtin_printf("%s", pError->localizedDescription()->utf8String());
    assert(false);
  }

  MTL::Function* pVertexFn = pLibrary->newFunction(NS::String::string("vertexMain", UTF8StringEncoding));
  MTL::Function* pFragFn = pLibrary->newFunction(NS::String::string("fragmentMain", UTF8StringEncoding));

  MTL::RenderPipelineDescriptor* pDesc = MTL::RenderPipelineDescriptor::alloc()->init();
  pDesc->setVertexFunction(pVertexFn);
  pDesc->setFragmentFunction(pFragFn);
  pDesc->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB);

  // Set up vertex descriptor for the vertex attributes
  MTL::VertexDescriptor* pVertexDesc = MTL::VertexDescriptor::alloc()->init();
  
  // Position attribute
  pVertexDesc->attributes()->object(0)->setFormat(MTL::VertexFormatFloat3);
  pVertexDesc->attributes()->object(0)->setOffset(0);
  pVertexDesc->attributes()->object(0)->setBufferIndex(0);
  
  // Texcoord attribute
  pVertexDesc->attributes()->object(1)->setFormat(MTL::VertexFormatFloat2);
  pVertexDesc->attributes()->object(1)->setOffset(0);
  pVertexDesc->attributes()->object(1)->setBufferIndex(1);
  
  // Set up buffer layouts
  pVertexDesc->layouts()->object(0)->setStride(sizeof(simd::float3));
  pVertexDesc->layouts()->object(1)->setStride(sizeof(simd::float2));
  
  pDesc->setVertexDescriptor(pVertexDesc);

  mpPSO = mpDevice->newRenderPipelineState(pDesc, &pError);
  if (!mpPSO)
  {
    __builtin_printf("%s", pError->localizedDescription()->utf8String());
    assert(false);
  }

  pVertexDesc->release();
  pVertexFn->release();
  pFragFn->release();
  pDesc->release();
  pLibrary->release();
}

void Renderer::buildBuffers()
{
  const size_t NumVertices = 6;

  // Positions remain the same
  simd::float3 positions[NumVertices] = {
      { -1.0f,  1.0f, 0.0f }, // Triangle 1, vertex 1: Top left
      { -1.0f, -1.0f, 0.0f }, // Triangle 1, vertex 2: Bottom left
      {  1.0f, -1.0f, 0.0f }, // Triangle 1, vertex 3: Bottom right

      { -1.0f,  1.0f, 0.0f }, // Triangle 2, vertex 1: Top left
      {  1.0f, -1.0f, 0.0f }, // Triangle 2, vertex 2: Bottom right
      {  1.0f,  1.0f, 0.0f }  // Triangle 2, vertex 3: Top right
  };

  // Add texture coordinates for each vertex
  simd::float2 texcoords[NumVertices] = {
      { 0.0f, 0.0f }, // Top left
      { 0.0f, 1.0f }, // Bottom left
      { 1.0f, 1.0f }, // Bottom right

      { 0.0f, 0.0f }, // Top left
      { 1.0f, 1.0f }, // Bottom right
      { 1.0f, 0.0f }  // Top right
  };

  const size_t positionsDataSize = NumVertices * sizeof(simd::float3);
  const size_t texcoordsDataSize = NumVertices * sizeof(simd::float2);

  mpVertexPositionsBuffer = mpDevice->newBuffer(positionsDataSize, MTL::ResourceStorageModeShared);
  mpVertexColorsBuffer = mpDevice->newBuffer(texcoordsDataSize, MTL::ResourceStorageModeShared);

  memcpy(mpVertexPositionsBuffer->contents(), positions, positionsDataSize);
  memcpy(mpVertexColorsBuffer->contents(), texcoords, texcoordsDataSize);
}

void Renderer::draw(MTK::View* pView)
{
  NS::AutoreleasePool* pPool = NS::AutoreleasePool::alloc()->init();

  MTL::CommandBuffer* pCmd = mpCommandQueue->commandBuffer();
  MTL::RenderPassDescriptor* pRpd = pView->currentRenderPassDescriptor();
  MTL::RenderCommandEncoder* pEnc = pCmd->renderCommandEncoder(pRpd);

  pEnc->setRenderPipelineState(mpPSO);
  pEnc->setVertexBuffer(mpVertexPositionsBuffer, 0, 0);
  pEnc->setVertexBuffer(mpVertexColorsBuffer, 0, 1);
  pEnc->drawPrimitives(MTL::PrimitiveType::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(6));

  pEnc->endEncoding();
  pCmd->presentDrawable(pView->currentDrawable());
  pCmd->commit();

  pPool->release();
}

#pragma endregion Renderer 
