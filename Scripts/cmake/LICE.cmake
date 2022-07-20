cmake_minimum_required(VERSION 3.11)

set(LICE_SRC "${WDL_DIR}/lice/")
set(_src
  curverasterbuffer.h
  lice_arc.cpp
  lice_bezier.h
  lice_bmp.cpp
  lice_colorspace.cpp
  lice_combine.h
  lice.cpp
  lice_extended.h
  lice.h
  lice_ico.cpp
  lice_image.cpp
  lice_import.h
  lice_line.cpp
  lice_lvg.cpp
  lice_palette.cpp
  lice_pcx.cpp  
  lice_texgen.cpp
  lice_text.cpp
  lice_text.h
  lice_textnew.cpp
)
list(TRANSFORM _src PREPEND "${LICE_SRC}")
add_library(LICE_Core INTERFACE)
iplug_target_add(LICE_Core INTERFACE
  SOURCE ${_src}
  INCLUDE
    "${WDL_DIR}/swell"
    "${WDL_DIR}/lice"
)


set(_src
  lice_glbitmap.cpp
  lice_glbitmap.h
  lice_gl_ctx.cpp
  lice_gl_ctx.h
)
list(TRANSFORM _src PREPEND "${LICE_SRC}")
add_library(LICE_GL INTERFACE)
iplug_target_add(LICE_GL INTERFACE SOURCE ${_src})


set(_src
  lice_gif.cpp
  lice_gif_write.cpp
)
list(TRANSFORM _src PREPEND "${LICE_SRC}")
set(_src2
  dgif_lib.c
  egif_lib.c
  gif_hash.c
  gifalloc.c
)
list(TRANSFORM _src2 PREPEND "${WDL_DIR}/giflib/")
add_library(LICE_GIF INTERFACE)
iplug_target_add(LICE_GIF INTERFACE SOURCE ${_src} ${_src2})


set(_src
  lice_png.cpp
  lice_png_write.cpp
)
list(TRANSFORM _src PREPEND "${LICE_SRC}")
set(_src2
  png.c
  pngconf.h
  pngdebug.h
  pngerror.c
  pngget.c
  png.h
  pnginfo.h
  pnglibconf.h
  pnglibconf.h.prebuilt
  pngmem.c
  pngpread.c
  pngpriv.h
  pngread.c
  pngrio.c
  pngrtran.c
  pngrutil.c
  pngset.c
  pngstruct.h
  pngtrans.c
  pngwio.c
  pngwrite.c
  pngwtran.c
  pngwutil.c
)
list(TRANSFORM _src2 PREPEND "${WDL_DIR}/libpng/")
add_library(LICE_PNG INTERFACE)
iplug_target_add(LICE_PNG INTERFACE
  DEFINE "PNG_WRITE_SUPPORTED"
  SOURCE ${_src} ${_src2}
)


set(_src
  libxml_tinyxml.cpp
  libxml_tinyxml.h
  svgtiny_colors.c
  tinystr.cpp
  tinystr.h
  tinyxml.cpp
  tinyxmlerror.cpp
  tinyxml.h
  tinyxmlparser.cpp
)
list(TRANSFORM _src PREPEND "${WDL_DIR}/tinyxml/")
add_library(LICE_TinyXML INTERFACE)
iplug_target_add(LICE_TinyXML INTERFACE SOURCE "${_src}")


add_library(LICE_SVG INTERFACE)
iplug_target_add(LICE_SVG INTERFACE
  SOURCE
    "${LICE_SRC}/lice_svg.cpp"
  LINK
    LICE_TinyXML
)


set(_src
  lice_jpg.cpp
  lice_jpg_write.cpp
)
list(TRANSFORM _src PREPEND "${LICE_SRC}")
set(_src2
  jcapimin.c
  jcapistd.c
  jccoefct.c
  jccolor.c
  jcdctmgr.c
  jchuff.c
  jchuff.h
  jcinit.c
  jcmainct.c
  jcmarker.c
  jcmaster.c
  jcomapi.c
  jconfig.h
  jcparam.c
  jcphuff.c
  jcprepct.c
  jcsample.c
  jctrans.c
  jdapimin.c
  jdapistd.c
  jdatadst.c
  jdatasrc.c
  jdcoefct.c
  jdcolor.c
  jdct.h
  jddctmgr.c
  jdhuff.c
  jdhuff.h
  jdinput.c
  jdmainct.c
  jdmarker.c
  jdmaster.c
  jdmerge.c
  jdphuff.c
  jdpostct.c
  jdsample.c
  jdtrans.c
  jerror.c
  jerror.h
  jfdctflt.c
  jfdctfst.c
  jfdctint.c
  jidctflt.c
  jidctfst.c
  jidctint.c
  jidctred.c
  jinclude.h
  jmemmgr.c
  jmemnobs.c
  jmemsys.h
  jmorecfg.h
  jpegint.h
  jpeglib.h
  jquant1.c
  jquant2.c
  jutils.c
  jversion.h
)
list(TRANSFORM _src2 PREPEND "${WDL_DIR}/jpeglib/")
add_library(LICE_JPEG INTERFACE)
iplug_target_add(LICE_JPEG INTERFACE SOURCE ${_src} ${_src2})

set(_src
  adler32.c
  compress.c
  crc32.c
  crc32.h
  deflate.c
  deflate.h
  gzclose.c
  gzguts.h
  gzlib.c
  gzread.c
  gzwrite.c
  infback.c
  inffast.c
  inffast.h
  inffixed.h
  inflate.c
  inflate.h
  inftrees.c
  inftrees.h
  ioapi.c
  ioapi.h
  trees.c
  trees.h
  uncompr.c
  unzip.c
  unzip.h
  zconf.h
  zip.c
  zip.h
  zlib.h
  zlib_import.h
  zutil.c
  zutil.h
)
list(TRANSFORM _src PREPEND "${WDL_DIR}/zlib/")
add_library(LICE_ZLIB INTERFACE)
iplug_target_add(LICE_ZLIB INTERFACE SOURCE ${_src})
