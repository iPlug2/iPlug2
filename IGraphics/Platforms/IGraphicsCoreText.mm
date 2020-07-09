/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include "IGraphicsCoreText.h"
#include "IPlugPaths.h"

using namespace iplug;
using namespace igraphics;

IFontDataPtr CoreTextFont::GetFontData()
{
  CFLocal<CFDataRef> rawData(CGDataProviderCopyData(mProvider));
  const UInt8* bytes = CFDataGetBytePtr(rawData.Get());
  int faceIdx = 0;
    
  if (mStyleString.GetLength())
      faceIdx = GetFaceIdx(bytes, static_cast<int>(CFDataGetLength(rawData.Get())), mStyleString.Get());
    
  IFontDataPtr fontData(new IFontData(bytes, static_cast<int>(CFDataGetLength(rawData.Get())), faceIdx));
  
  return fontData;
}

CoreTextFont::~CoreTextFont()
{
  CGDataProviderRelease(mProvider);
  if (mDescriptor)
    CFRelease(mDescriptor);
};

PlatformFontPtr CoreTextHelpers::LoadPlatformFont(const char* fontID, const char* fileNameOrResID, const char* bundleID, const char* sharedResourceSubPath)
{
  WDL_String fullPath;
  const EResourceLocation fontLocation = LocateResource(fileNameOrResID, "ttf", fullPath, bundleID, nullptr, sharedResourceSubPath);
  
  if (fontLocation == kNotFound)
    return nullptr;
  
  CFLocal<CFStringRef> path(CFStringCreateWithCString(NULL, fullPath.Get(), kCFStringEncodingUTF8));
  CFLocal<CFURLRef> url(CFURLCreateWithFileSystemPath(NULL, path.Get(), kCFURLPOSIXPathStyle, false));
  CFLocal<CGDataProviderRef> provider(url.Get() ? CGDataProviderCreateWithURL(url.Get()) : nullptr); // CGDataProviderCreateWithURL will fail in macOS sandbox!
  
  if (!provider.Get())
    return nullptr;
  
  CFLocal<CGFontRef> cgFont(CGFontCreateWithDataProvider(provider.Get()));
  CFLocal<CTFontRef> ctFont(CTFontCreateWithGraphicsFont(cgFont.Get(), 0.f, NULL, NULL));
  CFLocal<CTFontDescriptorRef> descriptor(CTFontCopyFontDescriptor(ctFont.Get()));
  
  if (!descriptor.Get())
    return nullptr;
  
  return PlatformFontPtr(new CoreTextFont(descriptor.Release(), provider.Release(), "", false));
}

PlatformFontPtr CoreTextHelpers::LoadPlatformFont(const char* fontID, const char* fontName, ETextStyle style)
{
  CFLocal<CFStringRef> fontStr(CFStringCreateWithCString(NULL, fontName, kCFStringEncodingUTF8));
  CFLocal<CFStringRef> styleStr(CFStringCreateWithCString(NULL, TextStyleString(style), kCFStringEncodingUTF8));
  
  CFStringRef keys[] = { kCTFontFamilyNameAttribute, kCTFontStyleNameAttribute };
  CFTypeRef values[] = { fontStr.Get(), styleStr.Get() };
  
  CFLocal<CFDictionaryRef> dictionary(CFDictionaryCreate(NULL, (const void**)&keys, (const void**)&values, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));
  CFLocal<CTFontDescriptorRef> descriptor(CTFontDescriptorCreateWithAttributes(dictionary.Get()));
  CFLocal<CFURLRef> url((CFURLRef) CTFontDescriptorCopyAttribute(descriptor.Get(), kCTFontURLAttribute));
  CFLocal<CGDataProviderRef> provider(url.Get() ? CGDataProviderCreateWithURL(url.Get()) : nullptr);
  
  if (!provider.Get())
    return nullptr;
  
  return PlatformFontPtr(new CoreTextFont(descriptor.Release(), provider.Release(), TextStyleString(style), true));
}

void releaseFontData(void* info, const void* data, size_t size)
{
  uint8_t* pData = (uint8_t*)data;
  delete[] pData;
}

PlatformFontPtr CoreTextHelpers::LoadPlatformFont(const char* fontID, void* pData, int dataSize)
{
  uint8_t* dataCopy = new uint8_t[dataSize];
  memcpy((void*)dataCopy, pData, dataSize);

  CFLocal<CGDataProviderRef> provider(CGDataProviderCreateWithData(nullptr, dataCopy, (size_t)dataSize, &releaseFontData));
  
  if (!provider.Get())
    return nullptr;
  
  CFLocal<CGFontRef> cgFont(CGFontCreateWithDataProvider(provider.Get()));
  CFLocal<CTFontRef> ctFont(CTFontCreateWithGraphicsFont(cgFont.Get(), 0.f, NULL, NULL));
  CFLocal<CTFontDescriptorRef> descriptor(CTFontCopyFontDescriptor(ctFont.Get()));
  
  if (!descriptor.Get())
    return nullptr;
  
  return PlatformFontPtr(new CoreTextFont(descriptor.Release(), provider.Release(), "", false));
}

void CoreTextHelpers::CachePlatformFont(const char* fontID, const PlatformFontPtr& font, StaticStorage<CoreTextFontDescriptor>& cache)
{
  StaticStorage<CoreTextFontDescriptor>::Accessor storage(cache);
  
  CTFontDescriptorRef descriptor = font->GetDescriptor();
  IFontDataPtr data = font->GetFontData();
    
  if (!storage.Find(fontID))
    storage.Add(new CoreTextFontDescriptor(descriptor, data->GetHeightEMRatio()), fontID);
}

CoreTextFontDescriptor* CoreTextHelpers::GetCTFontDescriptor(const IText& text, StaticStorage<CoreTextFontDescriptor>& cache)
{
  StaticStorage<CoreTextFontDescriptor>::Accessor storage(cache);
  
  CoreTextFontDescriptor* cachedFont = storage.Find(text.mFont);
  
  assert(cachedFont && "font not found - did you forget to load it?");
  
  return cachedFont;
}

