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

#ifndef AGG_PMAP_INCLUDED
#define AGG_PMAP_INCLUDED

#include <stdio.h>

namespace agg
{
  class pixel_map
  {
  public:
    
    enum format_e
    {
      format_png = 0,
      format_jpg
    };
    
    pixel_map() {}
    virtual ~pixel_map() {}
    
    virtual void destroy() = 0;
    virtual void create(unsigned width, unsigned height, unsigned clear_val=255) = 0;
    
    virtual void clear(unsigned clear_val=255) = 0;

    virtual unsigned char* buf() = 0;
    
    virtual unsigned width() const = 0;
    virtual unsigned height() const = 0;
    
    virtual int row_bytes() const = 0;
    virtual unsigned bpp() const = 0;
    
  private:
    
    pixel_map(const pixel_map&);
    const pixel_map& operator = (const pixel_map&);
  };
}

#endif
