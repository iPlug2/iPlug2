#include <assert.h>
#include <algorithm>
#include "agg_xcb_pmap.h"
#include "agg_basics.h"

#define PIXEL_RGBA(r,g,b,a) (((b)&0xff)|(((g)&0xff)<<8)|(((r)&0xff)<<16)|(((a)&0xff)<<24))

typedef struct 
{
    unsigned char *data;
    int len;
} pngReadStruct;

namespace agg
{

//------------------------------------------------------------------------
pixel_map_xcb::pixel_map_xcb()
: m_buf(0)
, m_bpp(0)
, m_width(0)
, m_height(0)
, m_img_size(0)
{
  // TODO: add xcb image properties parameter (bpp, lign_align, byte order) and create corresponding pixel_map
}

//------------------------------------------------------------------------
pixel_map_xcb::~pixel_map_xcb()
{
    destroy();
}

//------------------------------------------------------------------------
void pixel_map_xcb::destroy()
{
  if (m_buf)
  {
    delete [] m_buf;
    m_buf = nullptr;
  }
}


//------------------------------------------------------------------------
void pixel_map_xcb::create(unsigned width, unsigned height, unsigned clear_val)
{
    destroy();

    m_bpp = 32;

    if (width == 0)  width = 1;
    if (height == 0) height = 1;    

    m_row_bytes = calc_row_len (width, m_bpp);

    m_img_size = m_row_bytes * height;
    
    m_buf = new unsigned char [m_img_size];

    m_width = width;
    
    m_height = height;
    
    if (clear_val <= 255)
    {
        memset(m_buf, clear_val, m_img_size);
    }
}

//------------------------------------------------------------------------
void pixel_map_xcb::clear(unsigned clear_val)
{
    if (m_buf) memset(m_buf, clear_val, m_img_size);
}

//static
//------------------------------------------------------------------------
unsigned pixel_map_xcb::calc_row_len(unsigned width, unsigned bits_per_pixel)
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

        case 16: n *= 2;
                    break;

        case 24: n *= 3; 
                    break;

        case 32: n *= 4;
                    break;

        case 48: n *= 6; 
                    break;

        case 64: n *= 8; 
                    break;

        default: n = 0;
                    break;
    }
    return (n+3)&(~3); // assume 32bit (4 bytes) line_align
}

static void staticPngReadFunc(png_structp png_ptr, png_bytep data, png_size_t length)
{
    pngReadStruct *readStruct = (pngReadStruct *)png_get_io_ptr(png_ptr);
    memset(data, 0, length);

    int l = std::min((int)length, readStruct->len);
    memcpy(data, readStruct->data, l);
    readStruct->data += l;
    readStruct->len -= l;
}

bool pixel_map_xcb::load_png_from_filename(const char * filename)
{
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

    for(int i=0;i<height;i++)
    {
        unsigned char *bmpptr = row_pointers[i];
        int j=width;
        while (j-->0)
        {
            unsigned char a = bmpptr[0];
            unsigned char r = bmpptr[1];
            unsigned char g = bmpptr[2];
            unsigned char b = bmpptr[3];
            ((unsigned int*)bmpptr)[0] = PIXEL_RGBA(r,g,b,a);
            bmpptr+=4;
        }
    }

    free(row_pointers);
    return true;  
}

bool pixel_map_xcb::load_img_file(const char * filename)
{
    return load_png_from_filename(filename);
}
  

//------------------------------------------------------------------------
unsigned char* pixel_map_xcb::buf()
{
    return m_buf;
}

//------------------------------------------------------------------------
unsigned pixel_map_xcb::width() const
{
    return m_width;
}

//------------------------------------------------------------------------
unsigned pixel_map_xcb::height() const
{
    return m_height;
}

//------------------------------------------------------------------------
int pixel_map_xcb::row_bytes() const
{
    return calc_row_len(m_width, m_bpp);
}
}



