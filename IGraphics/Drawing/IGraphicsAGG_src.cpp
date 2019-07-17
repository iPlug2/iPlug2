/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#ifdef IGRAPHICS_AGG
#include "agg_arc.cpp"
#include "agg_arrowhead.cpp"
#include "agg_bezier_arc.cpp"
#include "agg_bspline.cpp"
#include "agg_curves.cpp"
#include "agg_font_freetype.cpp"
#include "agg_image_filters.cpp"
#include "agg_line_aa_basics.cpp"
#include "agg_line_profile_aa.cpp"
#include "agg_rounded_rect.cpp"
#include "agg_sqrt_tables.cpp"
#include "agg_trans_affine.cpp"
#include "agg_trans_double_path.cpp"
#include "agg_trans_single_path.cpp"
#include "agg_trans_warp_magnifier.cpp"
#include "agg_vcgen_bspline.cpp"
#include "agg_vcgen_contour.cpp"
#include "agg_vcgen_dash.cpp"
#include "agg_vcgen_markers_term.cpp"
#include "agg_vcgen_smooth_poly1.cpp"
#include "agg_vcgen_stroke.cpp"
#include "agg_vpgen_clip_polygon.cpp"
#include "agg_vpgen_clip_polyline.cpp"
#include "agg_vpgen_segmentator.cpp"
#ifdef OS_WIN
 #include "agg_win32_pmap.cpp"
#endif
#endif //IGRAPHICS_AGG
