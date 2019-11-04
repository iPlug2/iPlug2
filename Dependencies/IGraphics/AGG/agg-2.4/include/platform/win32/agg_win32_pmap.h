//----------------------------------------------------------------------------
// Anti-Grain Geometry - Version 2.4
// Copyright (C) 2002-2005 Maxim Shemanarev (http://www.antigrain.com)
//
// Permission to copy, use, modify, sell and distribute this software 
// is granted provided this copyright notice appears in all copies. 
// This software is provided "as is" without express or implied
// warranty, and with no claim as to its suitability for any purpose.
//
//----------------------------------------------------------------------------
// Contact: mcseem@antigrain.com
//          mcseemagg@yahoo.com
//          http://www.antigrain.com
//----------------------------------------------------------------------------
//
// class pixel_map
//
//----------------------------------------------------------------------------
#ifndef AGG_WIN32_PMAP_INCLUDED
#define AGG_WIN32_PMAP_INCLUDED


#include <windows.h>
#include <stdio.h>
#include "agg_pmap.h"

#include "png.h"

namespace agg
{
class pixel_map_win32 : public pixel_map
{
public:

    pixel_map_win32();
    ~pixel_map_win32();

    virtual void destroy();
    virtual void create(unsigned int width, unsigned int height, unsigned clear_val=256);
    virtual void clear(unsigned clear_val=256);
    virtual unsigned char* buf();
    virtual unsigned width() const;
    virtual unsigned height() const;
    virtual int row_bytes() const;
    virtual unsigned bpp() const { return m_bpp; }

    bool load_img_file(const char * filename);
    bool load_img(HINSTANCE hInst, const char * name, format_e format);
    void draw(HDC h_dc, int x, int y, double scale=1.0) const;

    static unsigned calc_row_len(unsigned width, unsigned bits_per_pixel);
        
private:

    bool load_png_from_filename(const char * filename);
    bool load_png_from_resource(HINSTANCE hInst, const char * name);
    bool load_jpg_from_resource(HINSTANCE hInst, const char * name);

    pixel_map_win32(const pixel_map_win32&);
      const pixel_map_win32& operator = (const pixel_map_win32&);


private:

    HDC             m_dc;
    HBITMAP         m_bitmap;
    HGDIOBJ         m_oldbitmap;

    unsigned char*  m_buf;
    unsigned        m_bpp;

    unsigned        m_img_size;
    int             m_row_bytes;
};

}


#endif
