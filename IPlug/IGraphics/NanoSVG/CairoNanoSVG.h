
#pragma once

#include "nanosvg.h"
#include "cairo/cairo.h"

namespace CairoNanoSVGRender
{
    void RenderNanoSVG(cairo_t* cr, NSVGimage *image);
}

