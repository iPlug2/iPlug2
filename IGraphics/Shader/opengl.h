/*
    nanogui/opengl.h -- Pulls in OpenGL, GLAD (if needed), GLFW, and
    NanoVG header files

    NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
    The widget drawing code is based on the NanoVG demo application
    by Mikko Mononen.

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/
/** \file */

#pragma once

#include "vector.h"

#if !defined(DOXYGEN_SHOULD_SKIP_THIS)
#  if defined(NANOGUI_USE_OPENGL)
#    if defined(NANOGUI_GLAD)
#      if defined(NANOGUI_SHARED) && !defined(GLAD_GLAPI_EXPORT)
#        define GLAD_GLAPI_EXPORT
#      endif
#      include <glad/glad.h>
#    else
#      if defined(__APPLE__)
#        define GLFW_INCLUDE_GLCOREARB
#      else
#        define GL_GLEXT_PROTOTYPES
#      endif
#    endif
#  elif defined(NANOGUI_USE_GLES) && NANOGUI_GLES_VERSION == 2
#    define GLFW_INCLUDE_ES2
#  elif defined(NANOGUI_USE_GLES) && NANOGUI_GLES_VERSION == 3
#    define GLFW_INCLUDE_ES3
#  elif defined(NANOGUI_USE_METAL)
#  else
#    error You must select a backend (OpenGL/GLES2/GLES3/Metal)
#  endif
#endif // DOXYGEN_SHOULD_SKIP_THIS

//#include <GLFW/glfw3.h>

#if defined(NANOGUI_USE_GLES)
#  if NANOGUI_GLES_VERSION == 2
#    include <GLES2/gl2ext.h>
#  elif NANOGUI_GLES_VERSION == 3
#    include <GLES3/gl2ext.h>
#  endif
#endif

#include <nanovg.h>

// Special treatment of linux Nvidia opengl headers
#if !defined(_WIN32) && !defined(__APPLE__) && defined(NANOGUI_USE_OPENGL)
  #if !defined(GL_UNIFORM_BUFFER)
    #warning NanoGUI suspects you have the NVIDIA OpenGL headers installed.  \
             Compilation will likely fail. If it does, you have two choices: \
             (1) Re-install the mesa-lib_gl header files.                    \
             (2) Compile with NANOGUI_USE_GLAD.
  #endif
#endif

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/// Allows for conversion between nanogui::Color and the NanoVG NVGcolor class.
inline Color::operator const NVGcolor &() const {
    return reinterpret_cast<const NVGcolor &>(*(this->v));
}

/**
 * \brief Determine whether an icon ID is a texture loaded via ``nvg_image_icon``.
 *
 * \rst
 * The implementation defines all ``value < 1024`` as image icons, and
 * everything ``>= 1024`` as an Entypo icon (see :ref:`file_nanogui_entypo.h`).
 * The value ``1024`` exists to provide a generous buffer on how many images
 * may have been loaded by NanoVG.
 * \endrst
 *
 * \param value
 *     The integral value of the icon.
 *
 * \return
 *     Whether or not this is an image icon.
 */
inline bool nvg_is_image_icon(int value) { return value < 1024; }

/**
 * \brief Determine whether an icon ID is a font-based icon (e.g. from ``entypo.ttf``).
 *
 * \rst
 * See :func:`nanogui::nvg_is_image_icon` for details.
 * \endrst
 *
 * \param value
 *     The integral value of the icon.
 *
 * \return
 *     Whether or not this is a font icon (from ``entypo.ttf``).
 */
inline bool nvg_is_font_icon(int value) { return value >= 1024; }


/// Check for OpenGL errors and warn if one is found (returns 'true' in that case')
extern bool nanogui_check_glerror(const char *cmd);

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
