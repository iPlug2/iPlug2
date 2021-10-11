#include <cmath>
#include <map>

#include "IGraphicsSkia.h"

#pragma warning( push )
#pragma warning( disable : 4244 )
#include "SkDashPathEffect.h"
#include "SkGradientShader.h"
#include "SkMaskFilter.h"
#include "SkFont.h"
#include "SkFontMetrics.h"
#include "SkTypeface.h"
#include "SkVertices.h"
#include "SkSwizzle.h"
#pragma warning( pop )

#if defined OS_MAC || defined OS_IOS
  #include "SkCGUtils.h"
  #if defined IGRAPHICS_GL2
    #include <OpenGL/gl.h>
  #elif defined IGRAPHICS_GL3
    #include <OpenGL/gl3.h>
  #endif

  #if defined IGRAPHICS_METAL
    //even though this is a .cpp we are in an objc(pp) compilation unit
    #import <Metal/Metal.h>
    #import <QuartzCore/CAMetalLayer.h>
    #include "include/gpu/mtl/GrMtlBackendContext.h"
  #endif

#elif defined OS_WIN
  #pragma comment(lib, "libpng.lib")
  #pragma comment(lib, "zlib.lib")
  #pragma comment(lib, "skia.lib")
  #pragma comment(lib, "svg.lib")
  #pragma comment(lib, "skshaper.lib")
  #pragma comment(lib, "skunicode.lib")

  // if the skia static lib is built with SK_GL and SK_DIRECT3D defined,
  // we need to link these .libs, even though we might not define IGRAPHICS_GL or IGRAPHICS_D3D
  #pragma comment(lib, "opengl32.lib")
  #pragma comment(lib, "d3d12.lib")
  #pragma comment(lib, "d3dcompiler.lib")

#if defined IGRAPHICS_D3D

  #pragma comment(lib, "dxgi.lib")
  #include "include/gpu/d3d/GrD3DBackendContext.h"

  #include <d3d12sdklayers.h>
  #include <d3d12.h>
  #include <dxgi1_4.h>
  #include <wrl/client.h>

  #define GR_D3D_CALL_ERRCHECK(X)                                       \
      do {                                                              \
         HRESULT result = X;                                            \
         SkASSERT(SUCCEEDED(result));                                   \
         if (!SUCCEEDED(result)) {                                      \
             SkDebugf("Failed Direct3D call. Error: 0x%08x\n", result); \
         }                                                              \
      } while(false)

  using namespace Microsoft::WRL;

#endif

#endif

#if defined IGRAPHICS_GL
  #include "gl/GrGLInterface.h"
#endif

using namespace iplug;
using namespace igraphics;

extern std::map<std::string, MTLTexturePtr> gTextureMap;

#pragma mark - Private Classes and Structs

class IGraphicsSkia::Bitmap : public APIBitmap
{
public:
  Bitmap(sk_sp<SkSurface> surface, int width, int height, float scale, float drawScale);
  Bitmap(const char* path, double sourceScale);
  Bitmap(const void* pData, int size, double sourceScale);
  Bitmap(sk_sp<SkImage>, double sourceScale);

private:
  SkiaDrawable mDrawable;
};
  
IGraphicsSkia::Bitmap::Bitmap(sk_sp<SkSurface> surface, int width, int height, float scale, float drawScale)
{
  mDrawable.mSurface = surface;
  mDrawable.mIsSurface = true;
  
  SetBitmap(&mDrawable, width, height, scale, drawScale);
}

IGraphicsSkia::Bitmap::Bitmap(const char* path, double sourceScale)
{
  sk_sp<SkData> data = SkData::MakeFromFileName(path);
  
  assert(data && "Unable to load file at path");
  
  auto image = SkImage::MakeFromEncoded(data);
  
#ifdef IGRAPHICS_CPU
  image = image->makeRasterImage();
#endif
  
  mDrawable.mImage = image;
  
  mDrawable.mIsSurface = false;
  SetBitmap(&mDrawable, mDrawable.mImage->width(), mDrawable.mImage->height(), sourceScale, 1.f);
}

IGraphicsSkia::Bitmap::Bitmap(const void* pData, int size, double sourceScale)
{
  auto data = SkData::MakeWithoutCopy(pData, size);
  auto image = SkImage::MakeFromEncoded(data);
  
#ifdef IGRAPHICS_CPU
  image = image->makeRasterImage();
#endif
  
  mDrawable.mImage = image;

  mDrawable.mIsSurface = false;
  SetBitmap(&mDrawable, mDrawable.mImage->width(), mDrawable.mImage->height(), sourceScale, 1.f);
}

IGraphicsSkia::Bitmap::Bitmap(sk_sp<SkImage> image, double sourceScale)
{
#ifdef IGRAPHICS_CPU
  mDrawable.mImage = image->makeRasterImage();
#else
  mDrawable.mImage = image;
#endif

  SetBitmap(&mDrawable, mDrawable.mImage->width(), mDrawable.mImage->height(), sourceScale, 1.f);
}

struct IGraphicsSkia::Font
{
  Font(IFontDataPtr&& data, sk_sp<SkTypeface> typeFace)
    : mData(std::move(data)), mTypeface(typeFace) {}
    
  IFontDataPtr mData;
  sk_sp<SkTypeface> mTypeface;
};

// Fonts
StaticStorage<IGraphicsSkia::Font> IGraphicsSkia::sFontCache;

#pragma mark - Utility conversions

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

SkColor SkiaColor(const IColor& color, const IBlend* pBlend)
{
  if (pBlend)
    return SkColorSetARGB(Clip(static_cast<int>(pBlend->mWeight * color.A), 0, 255), color.R, color.G, color.B);
  else
    return SkColorSetARGB(color.A, color.R, color.G, color.B);
}

SkRect SkiaRect(const IRECT& r)
{
  return SkRect::MakeLTRB(r.L, r.T, r.R, r.B);
}

SkBlendMode SkiaBlendMode(const IBlend* pBlend)
{
  if (!pBlend)
    return SkBlendMode::kSrcOver;
    
  switch (pBlend->mMethod)
  {
    case EBlend::SrcOver:      return SkBlendMode::kSrcOver;
    case EBlend::SrcIn:        return SkBlendMode::kSrcIn;
    case EBlend::SrcOut:       return SkBlendMode::kSrcOut;
    case EBlend::SrcAtop:      return SkBlendMode::kSrcATop;
    case EBlend::DstOver:      return SkBlendMode::kDstOver;
    case EBlend::DstIn:        return SkBlendMode::kDstIn;
    case EBlend::DstOut:       return SkBlendMode::kDstOut;
    case EBlend::DstAtop:      return SkBlendMode::kDstATop;
    case EBlend::Add:          return SkBlendMode::kPlus;
    case EBlend::XOR:          return SkBlendMode::kXor;
  }
  
  return SkBlendMode::kClear;
}

SkTileMode SkiaTileMode(const IPattern& pattern)
{
  switch (pattern.mExtend)
  {
    case EPatternExtend::None:      return SkTileMode::kDecal;
    case EPatternExtend::Reflect:   return SkTileMode::kMirror;
    case EPatternExtend::Repeat:    return SkTileMode::kRepeat;
    case EPatternExtend::Pad:       return SkTileMode::kClamp;
  }

  return SkTileMode::kClamp;
}

SkPaint SkiaPaint(const IPattern& pattern, const IBlend* pBlend)
{
  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setBlendMode(SkiaBlendMode(pBlend));
  int numStops = pattern.NStops();
    
  if (pattern.mType == EPatternType::Solid || numStops <  2)
  {
    paint.setColor(SkiaColor(pattern.GetStop(0).mColor, pBlend));
  }
  else
  {
    double x1 = 0.0;
    double y1 = 0.0;
    double x2 = 0.0;
    double y2 = 1.0;
      
    IMatrix m = pattern.mTransform;
    m.Invert();
    m.TransformPoint(x1, y1);
    m.TransformPoint(x2, y2);
      
    SkPoint points[2] =
    {
      SkPoint::Make(x1, y1),
      SkPoint::Make(x2, y2)
    };
    
    SkColor colors[8];
    SkScalar positions[8];
      
    assert(numStops <= 8);
    
    for(int i = 0; i < numStops; i++)
    {
      const IColorStop& stop = pattern.GetStop(i);
      colors[i] = SkiaColor(stop.mColor, pBlend);
      positions[i] = stop.mOffset;
    }

    switch (pattern.mType)
    {
    case EPatternType::Linear:
      paint.setShader(SkGradientShader::MakeLinear(points, colors, positions, numStops, SkiaTileMode(pattern), 0, nullptr));
      break;

    case EPatternType::Radial:
    {
      float xd = points[0].x() - points[1].x();
      float yd = points[0].y() - points[1].y();
      float radius = std::sqrt(xd * xd + yd * yd);
      paint.setShader(SkGradientShader::MakeRadial(points[0], radius, colors, positions, numStops, SkiaTileMode(pattern), 0, nullptr));
      break;
    }

    case EPatternType::Sweep:
    {
      SkMatrix matrix = SkMatrix::MakeAll(m.mXX, m.mYX, 0, m.mXY, m.mYY, 0, 0, 0, 1);
      
      paint.setShader(SkGradientShader::MakeSweep(x1, y1, colors, nullptr, numStops, SkTileMode::kDecal,
        0, 360 * positions[numStops - 1], 0, &matrix));

      break;
    }

    default:
      break;
    }
  }
    
  return paint;
}

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE

#pragma mark -

IGraphicsSkia::IGraphicsSkia(IGEditorDelegate& dlg, int w, int h, int fps, float scale)
: IGraphics(dlg, w, h, fps, scale)
{
  mMainPath.setIsVolatile(true);
  
  DBGMSG("%s @ %i FPS\n", GetDrawingAPIStr(), fps);
  StaticStorage<Font>::Accessor storage(sFontCache);
  storage.Retain();
}

IGraphicsSkia::~IGraphicsSkia()
{
  StaticStorage<Font>::Accessor storage(sFontCache);
  storage.Release();
}

bool IGraphicsSkia::BitmapExtSupported(const char* ext)
{
  char extLower[32];
  ToLower(extLower, ext);
  return (strstr(extLower, "png") != nullptr) || (strstr(extLower, "jpg") != nullptr) || (strstr(extLower, "jpeg") != nullptr);
}

APIBitmap* IGraphicsSkia::LoadAPIBitmap(const char* fileNameOrResID, int scale, EResourceLocation location, const char* ext)
{
//#ifdef OS_IOS
//  if (location == EResourceLocation::kPreloadedTexture)
//  {
//    assert(0 && "SKIA does not yet load KTX textures");
//    GrMtlTextureInfo textureInfo;
//    textureInfo.fTexture.retain((void*)(gTextureMap[fileNameOrResID]));
//    id<MTLTexture> texture = (id<MTLTexture>) textureInfo.fTexture.get();
//
//    MTLPixelFormat pixelFormat = texture.pixelFormat;
//
//    auto grBackendTexture = GrBackendTexture(texture.width, texture.height, GrMipMapped::kNo, textureInfo);
//
//    sk_sp<SkImage> image = SkImage::MakeFromTexture(mGrContext.get(), grBackendTexture, kTopLeft_GrSurfaceOrigin, kBGRA_8888_SkColorType, kOpaque_SkAlphaType, nullptr);
//    return new Bitmap(image, scale);
//  }
//  else
//#endif
#ifdef OS_WIN
  if (location == EResourceLocation::kWinBinary)
  {
    int size = 0;
    const void* pData = LoadWinResource(fileNameOrResID, ext, size, GetWinModuleHandle());
    return new Bitmap(pData, size, scale);
  }
  else
#endif
  return new Bitmap(fileNameOrResID, scale);
}

APIBitmap* IGraphicsSkia::LoadAPIBitmap(const char* name, const void* pData, int dataSize, int scale)
{
  return new Bitmap(pData, dataSize, scale);
}

#if defined IGRAPHICS_D3D
void get_hardware_adapter(IDXGIFactory4* pFactory, IDXGIAdapter1** ppAdapter) {
  *ppAdapter = nullptr;
  for (UINT adapterIndex = 0; ; ++adapterIndex) {
    IDXGIAdapter1* pAdapter = nullptr;
    if (DXGI_ERROR_NOT_FOUND == pFactory->EnumAdapters1(adapterIndex, &pAdapter)) {
      // No more adapters to enumerate.
      break;
    }

    // Check to see if the adapter supports Direct3D 12, but don't create the
    // actual device yet.
    if (SUCCEEDED(D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device),
      nullptr))) {
      *ppAdapter = pAdapter;
      return;
    }
    pAdapter->Release();
  }
}

bool CreateD3DBackendContext(GrD3DBackendContext* ctx, bool isProtected = false)
{
#if defined(SK_ENABLE_D3D_DEBUG_LAYER)
  // Enable the D3D12 debug layer.
  {
    gr_cp<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
      debugController->EnableDebugLayer();
    }
  }
#endif
  // Create the device
  gr_cp<IDXGIFactory4> factory;
  if (!SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
    return false;
  }

  gr_cp<IDXGIAdapter1> hardwareAdapter;
  get_hardware_adapter(factory.get(), &hardwareAdapter);

  gr_cp<ID3D12Device> device;
  if (!SUCCEEDED(D3D12CreateDevice(hardwareAdapter.get(),
    D3D_FEATURE_LEVEL_11_0,
    IID_PPV_ARGS(&device)))) {
    return false;
  }

  // Create the command queue
  gr_cp<ID3D12CommandQueue> queue;
  D3D12_COMMAND_QUEUE_DESC queueDesc = {};
  queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
  queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

  if (!SUCCEEDED(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&queue)))) {
    return false;
  }

  ctx->fAdapter = hardwareAdapter;
  ctx->fDevice = device;
  ctx->fQueue = queue;
  // TODO: set up protected memory
  ctx->fProtectedContext = /*isProtected ? GrProtected::kYes :*/ GrProtected::kNo;

  return true;
}
#endif //IGRAPHICS_D3D

void IGraphicsSkia::OnViewInitialized(void* pContext)
{
#if defined IGRAPHICS_GL
  if (GetBackendMode() == EBackendMode::OpenGL)
  {
    auto glInterface = GrGLMakeNativeInterface();
    mGrContext = GrDirectContext::MakeGL(glInterface);
  }
#endif
  
#if defined IGRAPHICS_METAL
  if (GetBackendMode() == EBackendMode::Metal)
  {
    CAMetalLayer* pMTLLayer = (CAMetalLayer*) pContext;
    id<MTLDevice> device = pMTLLayer.device;
    id<MTLCommandQueue> commandQueue = [device newCommandQueue];
    
    GrMtlBackendContext backendContext = {};
    backendContext.fDevice.retain((__bridge GrMTLHandle) device);
    backendContext.fQueue.retain((__bridge GrMTLHandle) commandQueue);
    mGrContext = GrDirectContext::MakeMetal(backendContext);
    
    mMTLDevice = (void*) device;
    mMTLCommandQueue = (void*) commandQueue;
    mMTLLayer = pContext;
  }
#endif

#if defined IGRAPHICS_D3D
  if (GetBackendMode() == EBackendMode::Direct3D)
  {
    GrD3DBackendContext backendContext;
    CreateD3DBackendContext(&backendContext);
    mDevice = backendContext.fDevice;
    mQueue = backendContext.fQueue;

    mGrContext = GrDirectContext::MakeDirect3D(backendContext);

    // Make the swapchain
    UINT dxgiFactoryFlags = 0;
    SkDEBUGCODE(dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG);

    gr_cp<IDXGIFactory4> factory;
    GR_D3D_CALL_ERRCHECK(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

    auto w = static_cast<int>(std::ceil(static_cast<float>(WindowWidth()) * GetScreenScale()));
    auto h = static_cast<int>(std::ceil(static_cast<float>(WindowHeight()) * GetScreenScale()));
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = kNumBuffers;
    swapChainDesc.Width = w;
    swapChainDesc.Height = h;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    HWND hWnd = (HWND) GetWindow();
    gr_cp<IDXGISwapChain1> swapChain;
    GR_D3D_CALL_ERRCHECK(factory->CreateSwapChainForHwnd(mQueue.get(), hWnd, &swapChainDesc, nullptr, nullptr, &swapChain));

    // We don't support fullscreen transitions.
    GR_D3D_CALL_ERRCHECK(factory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));

    GR_D3D_CALL_ERRCHECK(swapChain->QueryInterface(IID_PPV_ARGS(&mSwapChain)));

    mBufferIndex = mSwapChain->GetCurrentBackBufferIndex();

    for (int i = 0; i < kNumBuffers; ++i)
    {
      mFenceValues[i] = 10000;   // use a high value to make it easier to track these in PIX
    }

    GR_D3D_CALL_ERRCHECK(mDevice->CreateFence(mFenceValues[mBufferIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));

    mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    SkASSERT(mFenceEvent);
  }
#endif

  DrawResize();
}

void IGraphicsSkia::OnViewDestroyed()
{
  RemoveAllControls();

#if defined IGRAPHICS_GL || defined IGRAPHICS_METAL || defined IGRAPHICS_D3D
  mSurface = nullptr;
  mScreenSurface = nullptr;
  mGrContext = nullptr;
#endif

#if defined IGRAPHICS_METAL
  if (GetBackendMode() == EBackendMode::Metal)
  {
    [(id<MTLCommandQueue>) mMTLCommandQueue release];
    mMTLCommandQueue = nullptr;
    mMTLLayer = nullptr;
    mMTLDevice = nullptr;
  }
#endif

#if defined IGRAPHICS_D3D
  if (GetBackendMode() == EBackendMode::Direct3D)
  {
    CloseHandle(mFenceEvent);
    mFence.reset(nullptr);

    for (int i = 0; i < kNumBuffers; ++i)
    {
      mSurfaces[i].reset(nullptr);
      mBuffers[i].reset(nullptr);
    }

    mSwapChain.reset(nullptr);
    mQueue.reset(nullptr);
    mDevice.reset(nullptr);
  }
#endif
}

void IGraphicsSkia::DrawResize()
{
  auto w = static_cast<int>(std::ceil(static_cast<float>(WindowWidth()) * GetScreenScale()));
  auto h = static_cast<int>(std::ceil(static_cast<float>(WindowHeight()) * GetScreenScale()));

#if defined IGRAPHICS_GL || defined IGRAPHICS_METAL
  if (mGrContext.get() && GetBackendMode() > EBackendMode::Software)
  {
    SkImageInfo info = SkImageInfo::MakeN32Premul(w, h);
    mSurface = SkSurface::MakeRenderTarget(mGrContext.get(), SkBudgeted::kYes, info);
  }
#endif

#if defined IGRAPHICS_D3D
  if (mGrContext.get() && GetBackendMode() == EBackendMode::Direct3D)
  {
    // Clean up any outstanding resources in command lists
    mGrContext->flush({});
    mGrContext->submit(true);

    // release the previous surface and backbuffer resources
    for (int i = 0; i < kNumBuffers; ++i)
    {
      // Let present complete
      if (mFence->GetCompletedValue() < mFenceValues[i])
      {
        GR_D3D_CALL_ERRCHECK(mFence->SetEventOnCompletion(mFenceValues[i], mFenceEvent));
        WaitForSingleObjectEx(mFenceEvent, INFINITE, FALSE);
      }
      mSurfaces[i].reset(nullptr);
      mBuffers[i].reset(nullptr);
    }

    GR_D3D_CALL_ERRCHECK(mSwapChain->ResizeBuffers(0, w, h, DXGI_FORMAT_R8G8B8A8_UNORM, 0));

    // set up base resource info
    GrD3DTextureResourceInfo info(nullptr, nullptr, D3D12_RESOURCE_STATE_PRESENT, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, 0);

    for (int i = 0; i < kNumBuffers; ++i)
    {
      GR_D3D_CALL_ERRCHECK(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mBuffers[i])));

      SkASSERT(mBuffers[i]->GetDesc().Width == (UINT64)w && mBuffers[i]->GetDesc().Height == (UINT64)h);

      SkSurfaceProps props{ 0, kRGB_H_SkPixelGeometry };

      info.fResource = mBuffers[i];

      GrBackendRenderTarget backendRT(w, h, info);
      mSurfaces[i] = SkSurface::MakeFromBackendRenderTarget(mGrContext.get(), backendRT, kTopLeft_GrSurfaceOrigin, kRGBA_8888_SkColorType, nullptr, &props);
    }
  }
#endif // IGRAPHICS_D3D

#if defined IGRAPHICS_CPU
  if (GetBackendMode() == EBackendMode::Software)
  {
#if defined OS_WIN
    mSurface.reset();

    const size_t bmpSize = sizeof(BITMAPINFOHEADER) + (w * h * sizeof(uint32_t));
    mSurfaceMemory.Resize(bmpSize);
    BITMAPINFO* bmpInfo = reinterpret_cast<BITMAPINFO*>(mSurfaceMemory.Get());
    ZeroMemory(bmpInfo, sizeof(BITMAPINFO));
    bmpInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmpInfo->bmiHeader.biWidth = w;
    bmpInfo->bmiHeader.biHeight = -h; // negative means top-down bitmap. Skia draws top-down.
    bmpInfo->bmiHeader.biPlanes = 1;
    bmpInfo->bmiHeader.biBitCount = 32;
    bmpInfo->bmiHeader.biCompression = BI_RGB;
    void* pixels = bmpInfo->bmiColors;

    SkImageInfo info = SkImageInfo::Make(w, h, kN32_SkColorType, kPremul_SkAlphaType, nullptr);
    mSurface = SkSurface::MakeRasterDirect(info, pixels, sizeof(uint32_t) * w);
#else
    mSurface = SkSurface::MakeRasterN32Premul(w, h);
#endif
  }
#endif // IGRAPHICS_CPU
  
  if (mSurface)
  {
    mCanvas = mSurface->getCanvas();
    mCanvas->save();
  }
}

void IGraphicsSkia::BeginFrame()
{
#if defined IGRAPHICS_GL
  if (GetBackendMode() == EBackendMode::OpenGL)
  {
    if (mGrContext.get())
    {
      int width = WindowWidth() * GetScreenScale();
      int height = WindowHeight() * GetScreenScale();

      // Bind to the current main framebuffer
      int fbo = 0, samples = 0, stencilBits = 0;
      glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);
      glGetIntegerv(GL_SAMPLES, &samples);
#ifdef IGRAPHICS_GL3
      glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_STENCIL, GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE, &stencilBits);
#else
      glGetIntegerv(GL_STENCIL_BITS, &stencilBits);
#endif

      GrGLFramebufferInfo fbinfo;
      fbinfo.fFBOID = fbo;
      fbinfo.fFormat = 0x8058;

      GrBackendRenderTarget backendRT(width, height, samples, stencilBits, fbinfo);

      mScreenSurface = SkSurface::MakeFromBackendRenderTarget(mGrContext.get(), backendRT, kBottomLeft_GrSurfaceOrigin, kRGBA_8888_SkColorType, nullptr, nullptr);
      assert(mScreenSurface);
    }
  }
#endif //IGRAPHICS_GL

#if defined IGRAPHICS_METAL
  if (GetBackendMode() == EBackendMode::Metal)
  {
    if (mGrContext.get())
    {
      int width = WindowWidth() * GetScreenScale();
      int height = WindowHeight() * GetScreenScale();

      id<CAMetalDrawable> drawable = [(CAMetalLayer*)mMTLLayer nextDrawable];

      GrMtlTextureInfo fbInfo;
      fbInfo.fTexture.retain((const void*)(drawable.texture));
      GrBackendRenderTarget backendRT(width, height, 1 /* sample count/MSAA */, fbInfo);

      mScreenSurface = SkSurface::MakeFromBackendRenderTarget(mGrContext.get(), backendRT, kTopLeft_GrSurfaceOrigin, kBGRA_8888_SkColorType, nullptr, nullptr);

      mMTLDrawable = (void*)drawable;
      assert(mScreenSurface);
    }
  }
#endif

  IGraphics::BeginFrame();
}

void IGraphicsSkia::EndFrame()
{
#if defined IGRAPHICS_CPU
  if (GetBackendMode() == EBackendMode::Software)
  {
  #if defined OS_MAC || defined OS_IOS
    SkPixmap pixmap;
    mSurface->peekPixels(&pixmap);
    SkBitmap bmp;
    bmp.installPixels(pixmap);  
    CGContext* pCGContext = (CGContextRef) GetPlatformContext();
    CGContextSaveGState(pCGContext);
    CGContextScaleCTM(pCGContext, 1.0 / GetScreenScale(), 1.0 / GetScreenScale());
    SkCGDrawBitmap(pCGContext, bmp, 0, 0);
    CGContextRestoreGState(pCGContext);
  #elif defined OS_WIN
    auto w = WindowWidth() * GetScreenScale();
    auto h = WindowHeight() * GetScreenScale();
    BITMAPINFO* bmpInfo = reinterpret_cast<BITMAPINFO*>(mSurfaceMemory.Get());
    HWND hWnd = (HWND) GetWindow();
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);
    StretchDIBits(hdc, 0, 0, w, h, 0, 0, w, h, bmpInfo->bmiColors, bmpInfo, DIB_RGB_COLORS, SRCCOPY);
    ReleaseDC(hWnd, hdc);
    EndPaint(hWnd, &ps);
  #else
    #error NOT IMPLEMENTED
  #endif
  }
#endif

#if defined IGRAPHICS_GL || defined IGRAPHICS_METAL
  if ((GetBackendMode() == EBackendMode::OpenGL || GetBackendMode() == EBackendMode::Metal) && mScreenSurface)
  {
    mSurface->draw(mScreenSurface->getCanvas(), 0.0, 0.0, nullptr);

    #if defined IGRAPHICS_IMGUI
    if (mImGuiRenderer)
    {
      mImGuiRenderer->NewFrame();
      DrawImGui(mScreenSurface.get());
    }
    #endif

    mScreenSurface->getCanvas()->flush();

    #if defined IGRAPHICS_METAL
    if (GetBackendMode() == EBackendMode::Metal)
    {
      id<MTLCommandBuffer> commandBuffer = [(id<MTLCommandQueue>) mMTLCommandQueue commandBuffer];
      commandBuffer.label = @"Present";

      [commandBuffer presentDrawable : (id<CAMetalDrawable>) mMTLDrawable];
      [commandBuffer commit];
    }
    #endif
  }
#endif
#if defined IGRAPHICS_D3D
  if (GetBackendMode() == EBackendMode::Direct3D)
  {
    auto getBackbufferSurface = [&]() {
      // Update the frame index.
      const UINT64 currentFenceValue = mFenceValues[mBufferIndex];
      mBufferIndex = mSwapChain->GetCurrentBackBufferIndex();

      // If the last frame for this buffer index is not done, wait until it is ready.
      if (mFence->GetCompletedValue() < mFenceValues[mBufferIndex])
      {
        GR_D3D_CALL_ERRCHECK(mFence->SetEventOnCompletion(mFenceValues[mBufferIndex], mFenceEvent));
        WaitForSingleObjectEx(mFenceEvent, INFINITE, FALSE);
      }

      // Set the fence value for the next frame.
      mFenceValues[mBufferIndex] = currentFenceValue + 1;

      return mSurfaces[mBufferIndex];
    };

    sk_sp<SkSurface> backbuffer = getBackbufferSurface();
    mSurface->draw(backbuffer->getCanvas(), 0.0, 0.0, nullptr);
    backbuffer->flushAndSubmit();
    // swap buffers
    SkSurface* surface = mSurfaces[mBufferIndex].get();

    GrFlushInfo info;
    surface->flush(SkSurface::BackendSurfaceAccess::kPresent, info);
    mGrContext->submit();

    GR_D3D_CALL_ERRCHECK(mSwapChain->Present(1, 0));

    // Schedule a Signal command in the queue.
    GR_D3D_CALL_ERRCHECK(mQueue->Signal(mFence.get(), mFenceValues[mBufferIndex]));
  }
#endif
}

void IGraphicsSkia::DrawBitmap(const IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend)
{
  SkPaint p;
  
  p.setAntiAlias(true);
  p.setBlendMode(SkiaBlendMode(pBlend));

  if (pBlend)
    p.setAlpha(Clip(static_cast<int>(pBlend->mWeight * 255), 0, 255));
 
  SkiaDrawable* image = bitmap.GetAPIBitmap()->GetBitmap();

  double scale1 = 1.0 / (bitmap.GetScale() * bitmap.GetDrawScale());
  double scale2 = bitmap.GetScale() * bitmap.GetDrawScale();
  
  mCanvas->save();
  mCanvas->clipRect(SkiaRect(dest));
  mCanvas->translate(dest.L, dest.T);
  mCanvas->scale(scale1, scale1);
  mCanvas->translate(-srcX * scale2, -srcY * scale2);
  
#ifdef IGRAPHICS_CPU
  auto samplingOptions = SkSamplingOptions(SkFilterMode::kLinear, SkMipmapMode::kNone);
#else
  auto samplingOptions = SkSamplingOptions(SkCubicResampler::Mitchell());
#endif
    
  if (image->mIsSurface)
    image->mSurface->draw(mCanvas, 0.0, 0.0, samplingOptions, &p);
  else
    mCanvas->drawImage(image->mImage, 0.0, 0.0, samplingOptions, &p);
    
  mCanvas->restore();
}

void IGraphicsSkia::PathArc(float cx, float cy, float r, float a1, float a2, EWinding winding)
{
  SkPath arc;
  arc.setIsVolatile(true);
  
  float sweep = (a2 - a1);

  if (sweep >= 360.f || sweep <= -360.f)
  {
    arc.addCircle(cx, cy, r);
    mMainPath.addPath(arc, mMatrix, SkPath::kAppend_AddPathMode);
  }
  else
  {
    if (winding == EWinding::CW)
    {
      while (sweep < 0)
        sweep += 360.f;
    }
    else
    {
      while (sweep > 0)
        sweep -= 360.f;
    }
      
    arc.arcTo(SkRect::MakeLTRB(cx - r, cy - r, cx + r, cy + r), a1 - 90.f, sweep, false);
    mMainPath.addPath(arc, mMatrix, SkPath::kExtend_AddPathMode);
  }
}

IColor IGraphicsSkia::GetPoint(int x, int y)
{
  SkBitmap bitmap;
  bitmap.allocPixels(SkImageInfo::MakeN32Premul(1, 1));
  mCanvas->readPixels(bitmap, x, y);
  auto color = bitmap.getColor(0,0);
  return IColor(SkColorGetA(color), SkColorGetR(color), SkColorGetG(color), SkColorGetB(color));
}

bool IGraphicsSkia::LoadAPIFont(const char* fontID, const PlatformFontPtr& font)
{
  StaticStorage<Font>::Accessor storage(sFontCache);
  Font* cached = storage.Find(fontID);
  
  if (cached)
    return true;
  
  IFontDataPtr data = font->GetFontData();
  
  if (data->IsValid())
  {
    auto wrappedData = SkData::MakeWithoutCopy(data->Get(), data->GetSize());
    int index = data->GetFaceIdx();
    auto typeface = SkTypeface::MakeFromData(wrappedData, index);
    
    if (typeface)
    {
      storage.Add(new Font(std::move(data), typeface), fontID);
      return true;
    }
  }
  
  return false;
}

void IGraphicsSkia::PrepareAndMeasureText(const IText& text, const char* str, IRECT& r, double& x, double & y, SkFont& font) const
{
  SkFontMetrics metrics;
  SkPaint paint;
  //SkRect bounds;
  
  StaticStorage<Font>::Accessor storage(sFontCache);
  Font* pFont = storage.Find(text.mFont);
  
  assert(pFont && "No font found - did you forget to load it?");

  font.setTypeface(pFont->mTypeface);
  font.setHinting(SkFontHinting::kSlight);
  font.setForceAutoHinting(false);
  font.setSubpixel(true);
  font.setSize(text.mSize * pFont->mData->GetHeightEMRatio());
  
  // Draw / measure
  const double textWidth = font.measureText(str, strlen(str), SkTextEncoding::kUTF8, nullptr/* &bounds*/);
  font.getMetrics(&metrics);
  
  const double textHeight = text.mSize;
  const double ascender = metrics.fAscent;
  const double descender = metrics.fDescent;
  
  switch (text.mAlign)
  {
    case EAlign::Near:     x = r.L;                          break;
    case EAlign::Center:   x = r.MW() - (textWidth / 2.0);   break;
    case EAlign::Far:      x = r.R - textWidth;              break;
  }
  
  switch (text.mVAlign)
  {
    case EVAlign::Top:      y = r.T - ascender;                            break;
    case EVAlign::Middle:   y = r.MH() - descender + (textHeight / 2.0);   break;
    case EVAlign::Bottom:   y = r.B - descender;                           break;
  }
  
  r = IRECT((float) x, (float) y + ascender, (float) (x + textWidth), (float) (y + ascender + textHeight));
}

float IGraphicsSkia::DoMeasureText(const IText& text, const char* str, IRECT& bounds) const
{
  SkFont font;
  font.setEdging(SkFont::Edging::kSubpixelAntiAlias);

  IRECT r = bounds;
  double x, y;
  PrepareAndMeasureText(text, str, bounds, x, y, font);
  DoMeasureTextRotation(text, r, bounds);
  return bounds.W();
}

void IGraphicsSkia::DoDrawText(const IText& text, const char* str, const IRECT& bounds, const IBlend* pBlend)
{
  IRECT measured = bounds;
  
  SkFont font;
  font.setEdging(SkFont::Edging::kSubpixelAntiAlias);

  double x, y;

  PrepareAndMeasureText(text, str, measured, x, y, font);
  PathTransformSave();
  DoTextRotation(text, bounds, measured);
  SkPaint paint;
  paint.setColor(SkiaColor(text.mFGColor, pBlend));
  mCanvas->drawSimpleText(str, strlen(str), SkTextEncoding::kUTF8, x, y, font, paint);
  PathTransformRestore();
}

void IGraphicsSkia::PathStroke(const IPattern& pattern, float thickness, const IStrokeOptions& options, const IBlend* pBlend)
{
  SkPaint paint = SkiaPaint(pattern, pBlend);
  paint.setStyle(SkPaint::kStroke_Style);

  switch (options.mCapOption)
  {
    case ELineCap::Butt:   paint.setStrokeCap(SkPaint::kButt_Cap);     break;
    case ELineCap::Round:  paint.setStrokeCap(SkPaint::kRound_Cap);    break;
    case ELineCap::Square: paint.setStrokeCap(SkPaint::kSquare_Cap);   break;
  }

  switch (options.mJoinOption)
  {
    case ELineJoin::Miter: paint.setStrokeJoin(SkPaint::kMiter_Join);   break;
    case ELineJoin::Round: paint.setStrokeJoin(SkPaint::kRound_Join);   break;
    case ELineJoin::Bevel: paint.setStrokeJoin(SkPaint::kBevel_Join);   break;
  }
  
  if (options.mDash.GetCount())
  {
    // N.B. support odd counts by reading the array twice
    int dashCount = options.mDash.GetCount();
    int dashMax = dashCount & 1 ? dashCount * 2 : dashCount;
    float dashArray[16];
      
    for (int i = 0; i < dashMax; i += 2)
    {
      dashArray[i + 0] = options.mDash.GetArray()[i % dashCount];
      dashArray[i + 1] = options.mDash.GetArray()[(i + 1) % dashCount];
    }
    
    paint.setPathEffect(SkDashPathEffect::Make(dashArray, dashMax, options.mDash.GetOffset()));
  }
  
  paint.setStrokeWidth(thickness);
  paint.setStrokeMiter(options.mMiterLimit);
    
  RenderPath(paint);
  
  if (!options.mPreserve)
    mMainPath.reset();
}

void IGraphicsSkia::PathFill(const IPattern& pattern, const IFillOptions& options, const IBlend* pBlend)
{
  SkPaint paint = SkiaPaint(pattern, pBlend);
  paint.setStyle(SkPaint::kFill_Style);
  
  if (options.mFillRule == EFillRule::Winding)
    mMainPath.setFillType(SkPathFillType::kWinding);
  else
    mMainPath.setFillType(SkPathFillType::kEvenOdd);
  
  RenderPath(paint);
  
  if (!options.mPreserve)
    mMainPath.reset();
}

#ifdef IGRAPHICS_DRAWFILL_DIRECT
void IGraphicsSkia::DrawRect(const IColor& color, const IRECT& bounds, const IBlend* pBlend, float thickness)
{
  auto paint = SkiaPaint(color, pBlend);
  paint.setStyle(SkPaint::Style::kStroke_Style);
  paint.setStrokeWidth(thickness);
  mCanvas->drawRect(SkiaRect(bounds), paint);
}

void IGraphicsSkia::DrawRoundRect(const IColor& color, const IRECT& bounds, float cornerRadius, const IBlend* pBlend, float thickness)
{
  auto paint = SkiaPaint(color, pBlend);
  paint.setStyle(SkPaint::Style::kStroke_Style);
  paint.setStrokeWidth(thickness);
  mCanvas->drawRoundRect(SkiaRect(bounds), cornerRadius, cornerRadius, paint);
}

void IGraphicsSkia::DrawArc(const IColor& color, float cx, float cy, float r, float a1, float a2, const IBlend* pBlend, float thickness)
{
  auto paint = SkiaPaint(color, pBlend);
  paint.setStyle(SkPaint::Style::kStroke_Style);
  paint.setStrokeWidth(thickness);
  mCanvas->drawArc(SkRect::MakeLTRB(cx - r, cy - r, cx + r, cy + r), a1 - 90.f, (a2 - a1), false, paint);
}

void IGraphicsSkia::DrawCircle(const IColor& color, float cx, float cy, float r, const IBlend* pBlend, float thickness)
{
  auto paint = SkiaPaint(color, pBlend);
  paint.setStyle(SkPaint::Style::kStroke_Style);
  paint.setStrokeWidth(thickness);
  mCanvas->drawCircle(cx, cy, r, paint);
}

void IGraphicsSkia::DrawEllipse(const IColor& color, const IRECT& bounds, const IBlend* pBlend, float thickness)
{
  auto paint = SkiaPaint(color, pBlend);
  paint.setStyle(SkPaint::Style::kStroke_Style);
  paint.setStrokeWidth(thickness);
  mCanvas->drawOval(SkiaRect(bounds), paint);
}

void IGraphicsSkia::FillRect(const IColor& color, const IRECT& bounds, const IBlend* pBlend)
{
  auto paint = SkiaPaint(color, pBlend);
  paint.setStyle(SkPaint::Style::kFill_Style);
  mCanvas->drawRect(SkiaRect(bounds), paint);
}

void IGraphicsSkia::FillRoundRect(const IColor& color, const IRECT& bounds, float cornerRadius, const IBlend* pBlend)
{
  auto paint = SkiaPaint(color, pBlend);
  paint.setStyle(SkPaint::Style::kFill_Style);
  mCanvas->drawRoundRect(SkiaRect(bounds), cornerRadius, cornerRadius, paint);
}

void IGraphicsSkia::FillArc(const IColor& color, float cx, float cy, float r, float a1, float a2, const IBlend* pBlend)
{
  auto paint = SkiaPaint(color, pBlend);
  paint.setStyle(SkPaint::Style::kFill_Style);
  mCanvas->drawArc(SkRect::MakeLTRB(cx - r, cy - r, cx + r, cy + r), a1 - 90.f, (a2 - a1), true, paint);
}

void IGraphicsSkia::FillCircle(const IColor& color, float cx, float cy, float r, const IBlend* pBlend)
{
  auto paint = SkiaPaint(color, pBlend);
  paint.setStyle(SkPaint::Style::kFill_Style);
  mCanvas->drawCircle(cx, cy, r, paint);
}

void IGraphicsSkia::FillEllipse(const IColor& color, const IRECT& bounds, const IBlend* pBlend)
{
  auto paint = SkiaPaint(color, pBlend);
  paint.setStyle(SkPaint::Style::kFill_Style);
  mCanvas->drawOval(SkiaRect(bounds), paint);
}
#endif

void IGraphicsSkia::RenderPath(SkPaint& paint)
{
  SkMatrix invMatrix;
    
  if (!mMatrix.isIdentity() && mMatrix.invert(&invMatrix))
  {
    SkPath path;
    path.setIsVolatile(true);
    mMainPath.transform(invMatrix, &path);
    mCanvas->drawPath(path, paint);
  }
  else
  {
    mCanvas->drawPath(mMainPath, paint);
  }
}

void IGraphicsSkia::PathTransformSetMatrix(const IMatrix& m)
{
  double xTranslate = 0.0;
  double yTranslate = 0.0;
    
  if (!mCanvas)
    return;
    
  if (!mLayers.empty())
  {
    IRECT bounds = mLayers.top()->Bounds();
    
    xTranslate = -bounds.L;
    yTranslate = -bounds.T;
  }

  mMatrix = SkMatrix::MakeAll(m.mXX, m.mXY, m.mTX, m.mYX, m.mYY, m.mTY, 0, 0, 1);
  auto scale = GetTotalScale();
  SkMatrix globalMatrix = SkMatrix::Scale(scale, scale);
  mClipMatrix = SkMatrix();
  mFinalMatrix = mMatrix;
  globalMatrix.preTranslate(xTranslate, yTranslate);
  mClipMatrix.postConcat(globalMatrix);
  mFinalMatrix.postConcat(globalMatrix);
  mCanvas->setMatrix(mFinalMatrix);
}

void IGraphicsSkia::SetClipRegion(const IRECT& r)
{
  mCanvas->restoreToCount(0);
  mCanvas->save();
  mCanvas->setMatrix(mClipMatrix);
  mCanvas->clipRect(SkiaRect(r));
  mCanvas->setMatrix(mFinalMatrix);
}

APIBitmap* IGraphicsSkia::CreateAPIBitmap(int width, int height, float scale, double drawScale, bool cacheable)
{
  sk_sp<SkSurface> surface;
  
#if defined IGRAPHICS_CPU
  if (GetBackendMode() == EBackendMode::Software)
  {
    surface = SkSurface::MakeRasterN32Premul(width, height);
  }
#endif
  
#if defined IGRAPHICS_GL || defined IGRAPHICS_METAL
  if (GetBackendMode() > EBackendMode::Software)
  {
    SkImageInfo info = SkImageInfo::MakeN32Premul(width, height);

    if (cacheable)
    {
      surface = SkSurface::MakeRasterN32Premul(width, height);
    }
    else
    {
      surface = SkSurface::MakeRenderTarget(mGrContext.get(), SkBudgeted::kYes, info);
    }
  }
#endif
  
  surface->getCanvas()->save();

  return new Bitmap(std::move(surface), width, height, scale, drawScale);
}

void IGraphicsSkia::UpdateLayer()
{
  mCanvas = mLayers.empty() ? mSurface->getCanvas() : mLayers.top()->GetAPIBitmap()->GetBitmap()->mSurface->getCanvas();
}

static size_t CalcRowBytes(int width)
{
  width = ((width + 7) & (-8));
  return width * sizeof(uint32_t);
}

void IGraphicsSkia::GetLayerBitmapData(const ILayerPtr& layer, RawBitmapData& data)
{
  SkiaDrawable* pDrawable = layer->GetAPIBitmap()->GetBitmap();
  size_t rowBytes = CalcRowBytes(pDrawable->mSurface->width());
  int size = pDrawable->mSurface->height() * static_cast<int>(rowBytes);
    
  data.Resize(size);
   
  if (data.GetSize() >= size)
  {
    SkImageInfo info = SkImageInfo::MakeN32Premul(pDrawable->mSurface->width(), pDrawable->mSurface->height());
    pDrawable->mSurface->readPixels(info, data.Get(), rowBytes, 0, 0);
  }
}

void IGraphicsSkia::ApplyShadowMask(ILayerPtr& layer, RawBitmapData& mask, const IShadow& shadow)
{
  SkiaDrawable* pDrawable = layer->GetAPIBitmap()->GetBitmap();
  int width = pDrawable->mSurface->width();
  int height = pDrawable->mSurface->height();
  size_t rowBytes = CalcRowBytes(width);
  double scale = layer->GetAPIBitmap()->GetDrawScale() * layer->GetAPIBitmap()->GetScale();
  
  SkCanvas* pCanvas = pDrawable->mSurface->getCanvas();
    
  SkMatrix m;
  m.reset();
    
  SkImageInfo info = SkImageInfo::MakeN32Premul(width, height);
  SkPixmap pixMap(info, mask.Get(), rowBytes);
  sk_sp<SkImage> image = SkImage::MakeFromRaster(pixMap, nullptr, nullptr);
  sk_sp<SkImage> foreground;
    
  // Copy the foreground if needed
    
  if (shadow.mDrawForeground)
    foreground = pDrawable->mSurface->makeImageSnapshot();
 
  pCanvas->clear(SK_ColorTRANSPARENT);
 
  IBlend blend(EBlend::Default, shadow.mOpacity);
  pCanvas->setMatrix(m);
  pCanvas->drawImage(image, shadow.mXOffset * scale, shadow.mYOffset * scale);
  m = SkMatrix::Scale(scale, scale);
  pCanvas->setMatrix(m);
  pCanvas->translate(-layer->Bounds().L, -layer->Bounds().T);
  SkPaint p = SkiaPaint(shadow.mPattern, &blend);
  p.setBlendMode(SkBlendMode::kSrcIn);
  pCanvas->drawPaint(p);

  if (shadow.mDrawForeground)
  {
    m.reset();
    pCanvas->setMatrix(m);
    pCanvas->drawImage(foreground.get(), 0.0, 0.0);
  }
}

void IGraphicsSkia::DrawFastDropShadow(const IRECT& innerBounds, const IRECT& outerBounds, float xyDrop, float roundness, float blur, IBlend* pBlend)
{
  SkRect r = SkiaRect(innerBounds.GetTranslated(xyDrop, xyDrop));
  
  SkPaint paint = SkiaPaint(COLOR_BLACK_DROP_SHADOW, pBlend);
  paint.setStyle(SkPaint::Style::kFill_Style);
  
  paint.setMaskFilter(SkMaskFilter::MakeBlur(kSolid_SkBlurStyle, blur * 0.5)); // 0.5 seems to match nanovg
  mCanvas->drawRoundRect(r, roundness, roundness, paint);
}

const char* IGraphicsSkia::GetDrawingAPIStr()
{
  if (GetBackendMode() == EBackendMode::Software)
  {
    return "SKIA | CPU";
  }
  else if (GetBackendMode() == EBackendMode::OpenGL)
  {
#if defined IGRAPHICS_GL2
    return "SKIA | GL2";
#elif defined IGRAPHICS_GL3
    return "SKIA | GL3";
#else
    return "GL not enabled";
#endif
  }
  else if (GetBackendMode() == EBackendMode::Metal)
  {
    return "SKIA | Metal";
  }
  else if (GetBackendMode() == EBackendMode::Direct3D)
  {
    return "SKIA | Direct3D";
  }
  
  return "";
}
