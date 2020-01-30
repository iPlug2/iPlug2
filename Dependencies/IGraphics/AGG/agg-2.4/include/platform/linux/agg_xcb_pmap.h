//----------------------------------------------------------------------------
// Anti-Grain Geometry - Version 2.4 +
//
// Modified for iPlug2/Linux project by Alexey Zhelezov, 2020
//
//-------------------- Original file from ------------------------------------
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
#ifndef AGG_XCB_PMAP_INCLUDED
#define AGG_XCB_PMAP_INCLUDED

#include <xcb/xcb.h>
#include <stdio.h>
#include "agg_pmap.h"

#include "png.h"

namespace agg
{
class pixel_map_xcb : public pixel_map
{
public:

    pixel_map_xcb();
    ~pixel_map_xcb();

    virtual void destroy();
    virtual void create(unsigned int width, unsigned int height, unsigned clear_val=256);
    virtual void clear(unsigned clear_val=256);
    virtual unsigned char* buf();
    virtual unsigned width() const;
    virtual unsigned height() const;
    virtual int row_bytes() const;
    virtual unsigned bpp() const { return m_bpp; }

    bool load_img_file(const char * filename);
    void draw(xcb_connection_t *conn, xcb_drawable_t wnd) const;

    static unsigned calc_row_len(unsigned width, unsigned bits_per_pixel);
        
private:

    bool load_png_from_filename(const char * filename);

    pixel_map_xcb(const pixel_map_xcb&);
    const pixel_map_xcb& operator = (const pixel_map_xcb&);


private:
    unsigned char*  m_buf;
    unsigned        m_bpp;

    unsigned        m_width;
    unsigned        m_height;
    unsigned        m_img_size;
    int             m_row_bytes;
};

}


#endif
