//----------------------------------------------------------------------------
// Anti-Grain Geometry - Version 2.4
// Copyright (C) 2002-2005 Maxim Shemanarev (McSeem)
// Copyright (C) 2002 Hansruedi Baer (MacOS support)
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
//			baer@karto.baug.eth.ch
//----------------------------------------------------------------------------
//
// class pixel_map
//
//----------------------------------------------------------------------------
#ifndef AGG_MAC_PMAP_INCLUDED
#define AGG_MAC_PMAP_INCLUDED

#include <stdio.h>
#include <ApplicationServices/ApplicationServices.h>
#include "agg_pmap.h"

namespace agg
{
  class pixel_map_mac : public pixel_map
  {
  public:
    
    pixel_map_mac();
    virtual ~pixel_map_mac();
    
    virtual void destroy();
    virtual void create(unsigned width, unsigned height, unsigned clear_val=255);
    
    virtual void clear(unsigned clear_val=255);
    
    virtual unsigned char* buf();
    
    virtual unsigned width() const;
    virtual unsigned height() const;
    
    virtual int row_bytes() const;
    virtual unsigned bpp() const { return m_bpp; }
    
    void draw(CGContextRef ctx, double scale);
    bool load_img(const char* filename, format_e format);
    
    //auxiliary static functions
    static unsigned calc_row_len(unsigned width, unsigned bits_per_pixel);
    
  private:
    
    bool save_as_file(const char *filename) const;
    
    pixel_map_mac(const pixel_map_mac&);
    const pixel_map_mac& operator = (const pixel_map_mac&);
    
    CGContextRef m_ctx;
    CGImageRef m_img;
    unsigned char* m_buf;
    unsigned m_bpp;
    unsigned m_img_size;
    unsigned m_row_bytes;
  };
}


#endif
