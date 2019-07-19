#include <assert.h>
#include <algorithm>
#include "agg_win32_pmap.h"
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
pixel_map_win32::pixel_map_win32()
: m_buf(0)
, m_bpp(0)
, m_img_size(0)
, m_dc(0)
, m_bitmap(0)
, m_oldbitmap(0)
{
    m_dc = CreateCompatibleDC(NULL);
}

//------------------------------------------------------------------------
pixel_map_win32::~pixel_map_win32()
{
    destroy();
}

//------------------------------------------------------------------------
void pixel_map_win32::destroy()
{
    if (m_oldbitmap && m_dc) SelectObject(m_dc, m_oldbitmap);
    if (m_bitmap) DeleteObject(m_bitmap);
    if (m_dc) DeleteDC(m_dc);
}


//------------------------------------------------------------------------
void pixel_map_win32::create(unsigned width, unsigned height, unsigned clear_val)
{
    if (!m_dc) { width = height = 0; m_buf = 0; return; }

    m_bpp = 32;

    if (m_oldbitmap) 
    {
        SelectObject(m_dc, m_oldbitmap);
        m_oldbitmap = 0;
    }

    if (m_bitmap) DeleteObject(m_bitmap);
    
    m_bitmap = 0;
    m_buf = 0;

    if (width == 0)  width = 1;
    if (height == 0) height = 1;    
    
    BITMAPINFO pbmInfo = {0};
    pbmInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pbmInfo.bmiHeader.biWidth = width;
    pbmInfo.bmiHeader.biHeight = -height;
    pbmInfo.bmiHeader.biPlanes = 1;
    pbmInfo.bmiHeader.biBitCount = m_bpp;
    pbmInfo.bmiHeader.biCompression = BI_RGB;

    m_bitmap = CreateDIBSection(NULL, &pbmInfo, DIB_RGB_COLORS, (void **)&m_buf, NULL, 0);

    m_row_bytes = calc_row_len (width, m_bpp);
    m_img_size = m_row_bytes * height;
    
    if (m_bitmap)
    {
        m_oldbitmap = SelectObject(m_dc, m_bitmap);
    }

    if (clear_val <= 255)
    {
        memset(m_buf, clear_val, m_img_size);
    }
}

//------------------------------------------------------------------------
void pixel_map_win32::clear(unsigned clear_val)
{
    if (m_buf) memset(m_buf, clear_val, m_img_size);
}

//static
//------------------------------------------------------------------------
unsigned pixel_map_win32::calc_row_len(unsigned width, unsigned bits_per_pixel)
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
    return ((n + 3) >> 2) << 2;
}


//------------------------------------------------------------------------
void pixel_map_win32::draw(HDC h_dc, int x, int y, double scale) const
{
    if (m_dc == 0 || m_buf == 0) return;

    BitBlt(h_dc, x, y, width(), height(), m_dc, x, y, SRCCOPY);
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

bool pixel_map_win32::load_png_from_filename(const char * filename)
{
    FILE *fp = NULL;
#ifdef _WIN32
  if (GetVersion()<0x80000000)
  {
    WCHAR wf[2048];
    if (MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,filename,-1,wf,2048))
      fp = _wfopen(wf,L"rb");
  }
#endif

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

    assert (m_bitmap != 0);

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

bool pixel_map_win32::load_png_from_resource(HINSTANCE hInst, const char * name)
{
    HRSRC hResource = FindResource(hInst, name, "PNG");
    if (!hResource) return false;

    DWORD imageSize = SizeofResource(hInst, hResource);
    if (imageSize < 8) return false;

    HGLOBAL res = LoadResource(hInst, hResource);
    const void* resourceData = LockResource(res);
    if (!resourceData) return false;

    if (imageSize < 8) return NULL;
    unsigned char *data = (unsigned char *)(void*)resourceData;
    if (png_sig_cmp(data, 0, 8)) return false;

    pngReadStruct readStruct = {data, imageSize};

    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL); 
    
    if (!png_ptr) 
    {
        return false;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr); 
    if (!info_ptr)
    {
        png_destroy_read_struct(&png_ptr, NULL, NULL); 
        return false;
    }
  
    if (setjmp(png_jmpbuf(png_ptr)))
    { 
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL); 
        return false;
    }

    png_set_read_fn(png_ptr, &readStruct, staticPngReadFunc);

    png_read_info(png_ptr, info_ptr);

    unsigned int width = 0;
    unsigned int height = 0;

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

    assert (m_bitmap != 0);

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

bool pixel_map_win32::load_jpg_from_resource(HINSTANCE hInst, const char * name)
{
    assert(false);
    return false;
}

bool pixel_map_win32::load_img_file(const char * filename)
{
    return load_png_from_filename(filename);
}
  
bool pixel_map_win32::load_img(HINSTANCE hInst, const char * name, format_e format)
{
    switch (format)
    {
    case format_png:
        return load_png_from_resource(hInst, name);
    case format_jpg:
        return load_jpg_from_resource(hInst, name);
    }
    return false;
}

//------------------------------------------------------------------------
unsigned char* pixel_map_win32::buf()
{
    return m_buf;
}

//------------------------------------------------------------------------
unsigned pixel_map_win32::width() const
{
    BITMAP bm; 
    GetObject(m_bitmap, sizeof(BITMAP), &bm);
    return bm.bmWidth;
}

//------------------------------------------------------------------------
unsigned pixel_map_win32::height() const
{
    BITMAP bm; 
    GetObject(m_bitmap, sizeof(BITMAP), &bm);
    return bm.bmHeight;
}

//------------------------------------------------------------------------
int pixel_map_win32::row_bytes() const
{
    BITMAP bm; 
    GetObject(m_bitmap, sizeof(BITMAP), &bm);
    return calc_row_len(bm.bmWidth, bm.bmBitsPixel);
}
}



