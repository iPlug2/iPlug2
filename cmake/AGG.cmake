cmake_minimum_required(VERSION 3.11)

set(sdk "${IPLUG2_DIR}/Dependencies/IGraphics/AGG/agg-2.4")
set(_src
  "${sdk}/src/agg_arc.cpp"
  "${sdk}/src/agg_arrowhead.cpp"
  "${sdk}/src/agg_bezier_arc.cpp"
  "${sdk}/src/agg_bspline.cpp"
  "${sdk}/src/agg_color_rgba.cpp"
  "${sdk}/src/agg_curves.cpp"
  "${sdk}/src/agg_image_filters.cpp"
  "${sdk}/src/agg_line_aa_basics.cpp"
  "${sdk}/src/agg_line_profile_aa.cpp"
  "${sdk}/src/agg_rounded_rect.cpp"
  "${sdk}/src/agg_sqrt_tables.cpp"
  "${sdk}/src/agg_trans_affine.cpp"
  "${sdk}/src/agg_trans_double_path.cpp"
  "${sdk}/src/agg_trans_single_path.cpp"
  "${sdk}/src/agg_trans_warp_magnifier.cpp"
  "${sdk}/src/agg_vcgen_bspline.cpp"
  "${sdk}/src/agg_vcgen_contour.cpp"
  "${sdk}/src/agg_vcgen_dash.cpp"
  "${sdk}/src/agg_vcgen_markers_term.cpp"
  "${sdk}/src/agg_vcgen_smooth_poly1.cpp"
  "${sdk}/src/agg_vcgen_stroke.cpp"
  "${sdk}/src/agg_vpgen_clip_polygon.cpp"
  "${sdk}/src/agg_vpgen_clip_polyline.cpp"
  "${sdk}/src/agg_vpgen_segmentator.cpp"
)
add_library(AGG STATIC ${_src})
iplug2_target_add(AGG PUBLIC
  INCLUDE
    "${sdk}/include"
    "${sdk}/font_freetype"
    "${sdk}/include/util"
    "${sdk}/src"
    "${sdk}/include/platform/win32"
    "${sdk}/src/platform/win32"
)

add_library(iPlug2_AGG INTERFACE)
iplug2_target_add(iPlug2_AGG INTERFACE
  LINK AGG
  DEFINE "IGRAPHICS_AGG"
)
