/*
    NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
    The widget drawing code is based on the NanoVG demo application
    by Mikko Mononen.

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/
/**
 * \file nanogui/common.h
 *
 * \brief Common definitions used by NanoGUI.
 */

#pragma once

#include "vector.h"

#if defined(NANOGUI_USE_METAL)
BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/// Initialize the Metal backend
extern void metal_init();

/// Shut down the Metal backend
extern void metal_shutdown();

/// Return a pointer to the underlying Metal device (id<MTLDevice>)
extern void *metal_device();

/// Return a pointer to the underlying Metal command queue (id<MTLCommandQueue>)
extern void *metal_command_queue();

/// Return a pointer to the underlying Metal command queue (CAMetalLayer *)
extern void *metal_layer(void *nswin);

/// Associate a metal layer with a NSWindow created by GLEW
extern void metal_window_init(void *nswin, bool float_buffer);

/// Set size of the drawable underlying an NSWindow
extern void metal_window_set_size(void *nswin, const Vector2i &size);

/// Set content scale of the drawable underlying an NSWindow
extern void metal_window_set_content_scale(void *nswin, float scale);

/// Return the CAMetalLayer associated with a given NSWindow
extern void *metal_window_layer(void *nswin);

/// Acquire the next id<MTLDrawable> from the Metal layer
extern void* metal_window_next_drawable(void *nswin);

/// Return the id<MTLTexture> associated with an id<MTLDrawable>
extern void *metal_drawable_texture(void *drawable);

/// Release a drawable back to the pool
extern void metal_present_and_release_drawable(void *drawable);

/// Check whether any connected display supports 10-bit or EDR mode
extern std::pair<bool, bool> metal_10bit_edr_support();

// Create a new autorelease pool
extern void *autorelease_init();

// Drawin an autorelease pool
extern void autorelease_release(void *pool_);

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
#endif
