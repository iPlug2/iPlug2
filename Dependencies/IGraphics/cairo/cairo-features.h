/* Cairo Solution Files for Visual Studio 2017
*
* Copyright © 2014 Sólyom Zoltán
*
* This project is free software; you can redistribute it and/or
* modify it either under the terms of the GNU Lesser General Public
* License version 2.1 as published by the Free Software Foundation
* (the "LGPL") or, at your option, under the terms of the Mozilla
* Public License Version 1.1 (the "MPL"). If you do not alter this
* notice, a recipient may use your version of this file under either
* the MPL or the LGPL.
*
* You should have received a copy of the LGPL along with this library
* in the file COPYING-LGPL-2.1; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA 02110-1335, USA
* You should have received a copy of the MPL along with this library
* in the file COPYING-MPL-1.1
*
* The contents of this file are subject to the Mozilla Public License
* Version 1.1 (the "License"); you may not use this file except in
* compliance with the License. You may obtain a copy of the License at
* http://www.mozilla.org/MPL/
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY
* OF ANY KIND, either express or implied. See the LGPL or the MPL for
* the specific language governing rights and limitations.
*
*/

// Edit this file and recompile if you want to include some features in cairo.
// Some of the commented features might require libpng and zlib.


#ifndef CAIRO_FEATURES_H
#define CAIRO_FEATURES_H

#include <GL/glew.h>
#define CAIRO_HAS_GL_SURFACE 1

#define CAIRO_HAS_IMAGE_SURFACE 1
#define CAIRO_HAS_USER_FONT 1

#define CAIRO_HAS_PNG_FUNCTIONS 1

#define CAIRO_HAS_SVG_SURFACE 0
#define CAIRO_HAS_PDF_SURFACE 0
#define CAIRO_HAS_PS_SURFACE 0

#define CAIRO_HAS_WIN32_SURFACE 1
#define CAIRO_HAS_WIN32_FONT 1

#define CAIRO_HAS_FT_FONT 0

#define CAIRO_HAS_WGL_FUNCTIONS 1
#endif