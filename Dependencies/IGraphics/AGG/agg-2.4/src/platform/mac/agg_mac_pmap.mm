//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
// Contact: mcseemagg@yahoo.com
//          baer@karto.baug.ethz.ch
//----------------------------------------------------------------------------
//
// class pixel_map
//
//----------------------------------------------------------------------------

#include <string.h>
#include "agg_mac_pmap.h"
#include "agg_basics.h"
#include "agg_color_conv_rgb8.h"
#include "png.h"

namespace agg
{
  
  //------------------------------------------------------------------------
  pixel_map_mac::~pixel_map_mac()
  {
    destroy();
  }
  
  
  //------------------------------------------------------------------------
  pixel_map_mac::pixel_map_mac() :
  m_ctx(0),
  m_buf(0),
  m_bpp(0),
  m_img_size(0)
  {
  }
  
  
  //------------------------------------------------------------------------
  void pixel_map_mac::destroy()
  {
    delete[] m_buf;
    m_buf = NULL;
    if (m_ctx != nil)
    {
      CGContextRelease(m_ctx);
      m_ctx = nil;
    }
  }
  
  
  //------------------------------------------------------------------------
  void pixel_map_mac::create(unsigned width,
                             unsigned height,
                             unsigned clear_val)
  {
    destroy();
    
    if (width == 0)  width = 1;
    if (height == 0) height = 1;
    
    m_bpp = 32;
    
    int row_bytes = calc_row_len (width, m_bpp);
    
    m_img_size = row_bytes * height;
    
    m_buf = (unsigned char *)calloc(row_bytes * height, 1);
    if (!m_buf) return;
    
    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    m_ctx = CGBitmapContextCreate(m_buf, width, height, 8, row_bytes, cs, kCGImageAlphaNoneSkipFirst);
    CGColorSpaceRelease(cs);
    
    if (!m_ctx)
    {
      assert(false);
      free(m_buf);
      return;
    }

    if (clear_val <= 255)
    {
      memset(m_buf, clear_val, m_img_size);
    }
  }
  
  
  
  //------------------------------------------------------------------------
  void pixel_map_mac::clear(unsigned clear_val)
  {
    if (m_buf)
      memset(m_buf, clear_val, m_img_size);
  }
  
  //static
  //This function is just copied from the Win32 plattform support.
  //Is also seems to be appropriate for MacOS as well, but it is not
  //thouroughly tested so far.
  //------------------------------------------------------------------------
  
  unsigned pixel_map_mac::calc_row_len(unsigned width, unsigned bits_per_pixel)
  {
    unsigned n = width;
    unsigned k;
    
    switch (bits_per_pixel)
    {
      case  1: k = n;
        n = n >> 3;
        if(k & 7) n++;
        break;
        
      case  4: k = n;
        n = n >> 1;
        if(k & 3) n++;
        break;
        
      case  8:
        break;
        
      case 16: n = n << 1;
        break;
        
      case 24: n = (n << 1) + n;
        break;
        
      case 32: n = n << 2;
        break;
        
      default: n = 0;
        break;
    }
    return ((n + 3) >> 2) << 2;
  }
  
  void pixel_map_mac::draw(CGContextRef ctx, double scale)
  {
    CGRect rect = CGRectMake(0.0, 0.0, width() / scale, height() / scale);
    
    CGImageRef img = CGBitmapContextCreateImage(m_ctx);

    CGContextDrawImage(ctx, rect, img);
    
    CGImageRelease(img);
  }
  
  CGContextRef CreateARGBBitmapContext(CGImageRef image)
  {
    // Get image width, height. We'll use the entire image.
    int pixelsWide = (int)CGImageGetWidth(image);
    int pixelsHigh = (int)CGImageGetHeight(image);
    int bitmapBytesPerRow = (int)CGImageGetBytesPerRow(image);
    int bitmapByteCount = bitmapBytesPerRow * pixelsHigh;
    
    // Use the generic RGB color space.
    CGColorSpaceRef colorSpace = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
    if (colorSpace == 0)
    {
      NSLog(@"Error allocating color space");
      return NULL;
    }
    
    // Allocate memory for image data. This is the destination in memory
    // where any drawing to the bitmap context will be rendered.
    void * bitmapData = malloc( bitmapByteCount );
    if (bitmapData == 0)
    {
      NSLog(@"Memory not allocated!");
      CGColorSpaceRelease( colorSpace );
      return 0;
    }
    
    // Create the bitmap context. We want pre-multiplied ARGB, 8-bits
    // per component. Regardless of what the source image format is
    // (CMYK, Grayscale, and so on) it will be converted over to the format
    // specified here by CGBitmapContextCreate.
    CGContextRef context = CGBitmapContextCreate (bitmapData,
                                                  pixelsWide,
                                                  pixelsHigh,
                                                  8, //bits per component
                                                  bitmapBytesPerRow,
                                                  colorSpace,
                                                  kCGImageAlphaPremultipliedFirst);
    if (context == 0)
    {
      free (bitmapData);
      NSLog(@"Context not created!");
    }
    
    // Make sure and release colorspace before returning
    CGColorSpaceRelease( colorSpace );
    
    return context;
  }
  
  bool pixel_map_mac::load_img(const char * filename, format_e format)
  {
    if (format != format_png)
    {
      assert("jpg not supported");
    }
    
    FILE *fp = NULL;
    
    if (!fp) fp = fopen(filename,"rb");
    if(!fp) return 0;
    
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(!png_ptr)
    {
      fclose(fp);
      return 0;
    }
    
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if(!info_ptr)
    {
      png_destroy_read_struct(&png_ptr, NULL, NULL);
      fclose(fp);
      return 0;
    }
    
    if (setjmp(png_jmpbuf(png_ptr)))
    {
      png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
      fclose(fp);
      return 0;
    }
    
    png_init_io(png_ptr, fp);
    
    png_read_info(png_ptr, info_ptr);
    
    unsigned int width, height;
    int bit_depth, color_type, interlace_type, compression_type, filter_method;
    png_get_IHDR(png_ptr, info_ptr, &width, &height,
                 &bit_depth, &color_type, &interlace_type,
                 &compression_type, &filter_method);
    
    //convert whatever it is to RGBA
    if (color_type == PNG_COLOR_TYPE_PALETTE)
      png_set_palette_to_rgb(png_ptr);
    
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
      png_set_expand_gray_1_2_4_to_8(png_ptr);
    
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
      png_set_tRNS_to_alpha(png_ptr);
    
    if (bit_depth == 16)
      png_set_strip_16(png_ptr);
    
    if (bit_depth < 8)
      png_set_packing(png_ptr);
    
    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
      png_set_gray_to_rgb(png_ptr);
    
    if (color_type & PNG_COLOR_MASK_ALPHA)
      png_set_swap_alpha(png_ptr);
    else
      png_set_filler(png_ptr, 0xff, PNG_FILLER_BEFORE);
    
    if (m_buf)
    {
      free(m_buf);
      m_buf = 0;
    }
    
    assert(m_buf == 0);
    
    m_bpp = 32;
    
    m_row_bytes = calc_row_len (width, m_bpp);
    m_img_size = m_row_bytes * height;
    
    create(width, height, 0);
    
    if (!m_buf) return false;
    
    unsigned char **row_pointers = (unsigned char **)malloc(height * sizeof(unsigned char *));
    unsigned char *bmpptr = m_buf;
    
    for (int i=0; i<height; i++)
    {
      row_pointers[i] = (unsigned char *)bmpptr;
      bmpptr += m_row_bytes;
    }
    
    png_read_image(png_ptr, row_pointers);
    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
    fclose(fp);
    
    free(row_pointers);
    return true;
  }
  
  bool pixel_map_mac::save_as_file(const char *filename) const
  {
    NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDesktopDirectory, NSUserDomainMask, YES);
    NSString * path = [[paths objectAtIndex:0] stringByAppendingPathComponent:[NSString stringWithUTF8String:filename]];
    
    CFURLRef url = (CFURLRef)[NSURL fileURLWithPath:path];
    CGImageDestinationRef destination = CGImageDestinationCreateWithURL(url, kUTTypePNG, 1, NULL);
    
    if (!destination)
    {
      NSLog(@"Failed to create CGImageDestination for %@", path);
      return false;
    }
    
    CFDataRef data = CFDataCreate(NULL, m_buf, m_img_size);
    CGDataProviderRef provider = CGDataProviderCreateWithCFData(data);
    
    int row_bytes = calc_row_len (width(), m_bpp);
    
    CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
    CGImageRef img = CGImageCreate(width(),                     //width
                                   height(),                    //height
                                   8,                           //bitsPerComponent
                                   m_bpp,                       //bitsPerPixel
                                   row_bytes,                   //bytesPerRow
                                   space,                       //colorspace
                                   kCGImageAlphaPremultipliedFirst,//bitmapInfo
                                   provider,                    //CGDataProvider
                                   NULL,                        //decode array
                                   false,                       //shouldInterpolate
                                   kCGRenderingIntentDefault);  //intent
    
    CGImageDestinationAddImage(destination, img, nil);
    
    bool result = CGImageDestinationFinalize(destination);
    
    if (!result)
    {
      NSLog(@"Failed to write image to %@", path);
    }
    
    CFRelease(img);
    CFRelease(data);
    CFRelease(destination);
    
    return result;
  }
  
  
  //------------------------------------------------------------------------
  unsigned char* pixel_map_mac::buf()
  {
    return m_buf;
  }
  
  //------------------------------------------------------------------------
  unsigned pixel_map_mac::width() const
  {
    return (unsigned)CGBitmapContextGetWidth(m_ctx);
  }
  
  //------------------------------------------------------------------------
  unsigned pixel_map_mac::height() const
  {
    return (unsigned)CGBitmapContextGetHeight(m_ctx);
  }
  
  //------------------------------------------------------------------------
  int pixel_map_mac::row_bytes() const
  {
    return (int)CGBitmapContextGetBytesPerRow(m_ctx);
  }
}



