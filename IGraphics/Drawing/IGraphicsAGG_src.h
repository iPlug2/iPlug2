/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/


BEGIN_INCLUDE_DEPENDENCIES
#include "agg_basics.h"
#include "agg_renderer_base.h"
#include "agg_renderer_primitives.h"
#include "agg_rendering_buffer.h"
#include "agg_pixfmt_rgb.h"
#include "agg_pixfmt_rgba.h"
#include "agg_pixfmt_amask_adaptor.h"
#include "agg_renderer_scanline.h"
#include "agg_rasterizer_scanline_aa.h"
#include "agg_renderer_outline_aa.h"
#include "agg_rasterizer_outline_aa.h"
#include "agg_conv_stroke.h"
#include "agg_conv_dash.h"
#include "agg_conv_curve.h"
#include "agg_conv_contour.h"
#include "agg_conv_smooth_poly1.h"
#include "agg_conv_clip_polygon.h"
#include "agg_conv_marker.h"
#include "agg_arrowhead.h"
#include "agg_vcgen_markers_term.h"
#include "agg_scanline_p.h"
#include "agg_renderer_scanline.h"
#include "agg_pixfmt_rgb.h"
#include "agg_pixfmt_gray.h"
#include "agg_alpha_mask_u8.h"
#include "agg_path_storage.h"
#include "agg_bounding_rect.h"
#include "agg_ellipse.h"
#include "agg_font_freetype.h"
#include "agg_pmap.h"
#include "agg_image_accessors.h"
#include "agg_span_allocator.h"
#include "agg_span_interpolator_linear.h"
#include "agg_span_gradient.h"
#include "agg_renderer_outline_image.h"
#include "agg_pattern_filters_rgba.h"
#include "agg_span_image_filter_rgba.h"
#include "agg_span_image_filter_rgb.h"
#include "agg_span_image_filter_gray.h"
#include "agg_rounded_rect.h"
#include "agg_span_converter.h"
#include "agg_conv_segmentator.h"
#include "agg_trans_single_path.h"
#include "agg_gradient_lut.h"
#if PLATFORM_MAC
	#include "agg_mac_pmap.h"
#elif PLATFORM_WINDOWS
	//#pragma comment(lib, "libpng.lib")
	//#pragma comment(lib, "zlib.lib")
	//#pragma comment(lib, "freetype.lib")
	#include "agg_win32_pmap.h"
#endif
END_INCLUDE_DEPENDENCIES
