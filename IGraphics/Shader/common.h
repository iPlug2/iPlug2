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

#include <stdint.h>
#include <functional>
#include <vector>
#include <string>
#include <stdexcept>

#include "IGraphicsNanoVG.h"

#define NANOGUI_VERSION_MAJOR 0
#define NANOGUI_VERSION_MINOR 1
#define NANOGUI_VERSION_PATCH 4

#define NANOGUI_STRINGIFY(x) #x
#define NANOGUI_TOSTRING(x)  NANOGUI_STRINGIFY(x)
#define NANOGUI_VERSION                                                          \
    (NANOGUI_TOSTRING(NANOGUI_VERSION_MAJOR) "."                                 \
     NANOGUI_TOSTRING(NANOGUI_VERSION_MINOR) "."                                 \
     NANOGUI_TOSTRING(NANOGUI_VERSION_PATCH))

/* Set to 1 to draw boxes around widgets */
//#define NANOGUI_SHOW_WIDGET_BOUNDS 1

#if !defined(NAMESPACE_BEGIN) || defined(DOXYGEN_DOCUMENTATION_BUILD)
    /**
     * \brief Convenience macro for namespace declarations
     *
     * The macro ``NAMESPACE_BEGIN(nanogui)`` will expand to ``namespace
     * nanogui {``. This is done to hide the namespace scope from editors and
     * C++ code formatting tools that may otherwise indent the entire file.
     * The corresponding ``NAMESPACE_END`` macro also lists the namespace
     * name for improved readability.
     *
     * \param name
     *     The name of the namespace scope to open
     */
    #define NAMESPACE_BEGIN(name) namespace name {
#endif
#if !defined(NAMESPACE_END) || defined(DOXYGEN_DOCUMENTATION_BUILD)
    /**
     * \brief Convenience macro for namespace declarations
     *
     * Closes a namespace (counterpart to ``NAMESPACE_BEGIN``)
     * ``NAMESPACE_END(nanogui)`` will expand to only ``}``.
     *
     * \param name
     *     The name of the namespace scope to close
     */
    #define NAMESPACE_END(name) }
#endif

#if defined(NANOGUI_SHARED)
#  if defined(_WIN32)
#    if defined(NANOGUI_BUILD)
#      define NANOGUI_EXPORT __declspec(dllexport)
#    else
#      define NANOGUI_EXPORT __declspec(dllimport)
#    endif
#  elif defined(NANOGUI_BUILD)
#    define NANOGUI_EXPORT __attribute__ ((visibility("default")))
#  else
#    define NANOGUI_EXPORT
#  endif
#else
     /**
      * If the build flag ``NANOGUI_SHARED`` is defined, this directive will expand
      * to be the platform specific shared library import / export command depending
      * on the compilation stage.  If undefined, it expands to nothing. **Do not**
      * define this directive on your own.
      */
#    define NANOGUI_EXPORT
#endif

/* Force usage of discrete GPU on laptops (macro must be invoked in main application) */
#if defined(_WIN32) && !defined(DOXYGEN_DOCUMENTATION_BUILD)
#define NANOGUI_FORCE_DISCRETE_GPU() \
    extern "C" { \
        __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1; \
        __declspec(dllexport) int NvOptimusEnablement = 1; \
    }
#else
/**
 * On Windows, exports ``AmdPowerXpressRequestHighPerformance`` and
 * ``NvOptimusEnablement`` as ``1``.
 */
#define NANOGUI_FORCE_DISCRETE_GPU()
#endif

#if defined(_MSC_VER)
#  if defined(NANOGUI_BUILD)
/* Quench a few warnings on when compiling NanoGUI on Windows with MSVC */
#    pragma warning(disable : 4127) // warning C4127: conditional expression is constant
#    pragma warning(disable : 4244) // warning C4244: conversion from X to Y, possible loss of data
#  endif
#  pragma warning(disable : 4251) // warning C4251: class X needs to have dll-interface to be used by clients of class Y
#  pragma warning(disable : 4714) // warning C4714: function X marked as __forceinline not inlined
#endif

// These will produce broken links in the docs build
#if !defined(DOXYGEN_SHOULD_SKIP_THIS)

extern "C" {
    /* Opaque handle types */
    typedef struct NVGcontext NVGcontext;
    typedef struct GLFWwindow GLFWwindow;
}

struct NVGcolor;
struct NVGglyphPosition;
struct GLFWcursor;

#endif // DOXYGEN_SHOULD_SKIP_THIS

// Define command key for windows/mac/linux
#if defined(__APPLE__) || defined(DOXYGEN_DOCUMENTATION_BUILD)
    /// If on OSX, maps to ``GLFW_MOD_SUPER``.  Otherwise, maps to ``GLFW_MOD_CONTROL``.
    #define SYSTEM_COMMAND_MOD GLFW_MOD_SUPER
#else
    #define SYSTEM_COMMAND_MOD GLFW_MOD_CONTROL
#endif

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

///// Cursor shapes available to use in GLFW.  Shape of actual cursor determined by Operating System.
//enum class Cursor {
//    Arrow = 0,  ///< The arrow cursor.
//    IBeam,      ///< The I-beam cursor.
//    Crosshair,  ///< The crosshair cursor.
//    Hand,       ///< The hand cursor.
//    HResize,    ///< The horizontal resize cursor.
//    VResize,    ///< The vertical resize cursor.
//    CursorCount ///< Not a cursor --- should always be last: enables a loop over the cursor types.
//};

// skip the forward declarations for the docs
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/* Forward declarations */
//template <typename T> class ref;
//class AdvancedGridLayout;
//class BoxLayout;
//class Button;
//class CheckBox;
//class Canvas;
//class ColorWheel;
//class ColorPicker;
//class ComboBox;
//class GLFramebuffer;
//class GLShader;
//class GridLayout;
//class GroupLayout;
//class ImagePanel;
//class ImageView;
//class Label;
//class Layout;
//class MessageDialog;
//class Object;
//class Popup;
//class PopupButton;
//class ProgressBar;
class RenderPass;
class Shader;
//class Screen;
//class Serializer;
//class Slider;
//class TabWidgetBase;
//class TabWidget;
//class TextBox;
//class TextArea;
class Texture;
//class Theme;
//class ToolButton;
//class VScrollPanel;
//class Widget;
//class Window;

#endif // DOXYGEN_SHOULD_SKIP_THIS

/**
 * Static initialization; should be called once before invoking **any** NanoGUI
 * functions **if** you are having NanoGUI manage OpenGL / GLFW.  This method
 * is effectively a wrapper call to ``glfwInit()``, so if you are managing
 * OpenGL / GLFW on your own *do not call this method*.
 *
 * \rst
 * Refer to :ref:`nanogui_example_3` for how you might go about managing OpenGL
 * and GLFW on your own, while still using NanoGUI's classes.
 * \endrst
 */
extern NANOGUI_EXPORT void init();

/// Static shutdown; should be called before the application terminates.
extern NANOGUI_EXPORT void shutdown();

/**
 * \brief Enter the application main loop
 *
 * \param refresh
 *     NanoGUI issues a redraw call whenever an keyboard/mouse/.. event is
 *     received. In the absence of any external events, it enforces a redraw
 *     once every ``refresh`` milliseconds. To disable the refresh timer,
 *     specify a negative value here.
 *
 * \param detach
 *     This parameter only exists in the Python bindings. When the active
 *     \c Screen instance is provided via the \c detach parameter, the
 *     ``mainloop()`` function becomes non-blocking and returns
 *     immediately (in this case, the main loop runs in parallel on a newly
 *     created thread). This feature is convenient for prototyping user
 *     interfaces on an interactive Python command prompt. When
 *     ``detach != None``, the function returns an opaque handle that
 *     will release any resources allocated by the created thread when the
 *     handle's ``join()`` method is invoked (or when it is garbage
 *     collected).
 *
 * \remark
 *     Unfortunately, Mac OS X strictly requires all event processing to take
 *     place on the application's main thread, which is fundamentally
 *     incompatible with this type of approach. Thus, NanoGUI relies on a
 *     rather crazy workaround on Mac OS (kudos to Dmitriy Morozov):
 *     ``mainloop()`` launches a new thread as before but then uses
 *     libcoro to swap the thread execution environment (stack, registers, ..)
 *     with the main thread. This means that the main application thread is
 *     hijacked and processes events in the main loop to satisfy the
 *     requirements on Mac OS, while the thread that actually returns from this
 *     function is the newly created one (paradoxical, as that may seem).
 *     Deleting or ``join()``ing the returned handle causes application to
 *     wait for the termination of the main loop and then swap the two thread
 *     environments back into their initial configuration.
 */
extern NANOGUI_EXPORT void mainloop(float refresh = -1.f);

/// Request the application main loop to terminate (e.g. if you detached mainloop).
extern NANOGUI_EXPORT void leave();

/// Return whether or not a main loop is currently active
extern NANOGUI_EXPORT bool active();

/**
 * \brief Enqueue a function to be executed executed before
 * the application is redrawn the next time.
 *
 * NanoGUI is not thread-safe, and async() provides a mechanism
 * for queuing up UI-related state changes from other threads.
 */
extern NANOGUI_EXPORT void async(const std::function<void()> &func);

/**
 * \brief Open a native file open/save dialog.
 *
 * \param filetypes
 *     Pairs of permissible formats with descriptions like
 *     ``("png", "Portable Network Graphics")``.
 *
 * \param save
 *     Set to ``true`` if you would like subsequent file dialogs to open
 *     at whatever folder they were in when they close this one.
 */
//extern NANOGUI_EXPORT std::string
//file_dialog(const std::vector<std::pair<std::string, std::string>> &filetypes,
//            bool save);


/**
 * \brief Check for the availability of displays with 10-bit color and/or
 * extended dynamic range (EDR), i.e. the ability to reproduce intensities
 * exceeding the standard dynamic range from 0.0-1.0.
 *
 * To leverage either of these features, you will need to create a \ref Screen
 * with <tt>float_buffer=True</tt>. Only the macOS Metal backend of NanoGUI
 * implements this function right now. All other platforms return <tt>(false,
 * false)</tt>.
 *
 * \return A <tt>std::pair</tt> with two boolean values. The first indicates
 * 10-bit color support, and the second indicates EDR support.
 */
extern NANOGUI_EXPORT std::pair<bool, bool> test_10bit_edr_support();

/**
 * \brief Open a native file open dialog, which allows multiple selection.
 *
 * \param filetypes
 *     Pairs of permissible formats with descriptions like
 *     ``("png", "Portable Network Graphics")``.
 *
 * \param save
 *     Set to ``true`` if you would like subsequent file dialogs to open
 *     at whatever folder they were in when they close this one.
 *
 * \param multiple
 *     Set to ``true`` if you would like to be able to select multiple
 *     files at once. May not be simultaneously true with \p save.
 */
//extern NANOGUI_EXPORT std::vector<std::string>
//file_dialog(const std::vector<std::pair<std::string, std::string>> &filetypes,
//            bool save, bool multiple);

#if defined(__APPLE__) || defined(DOXYGEN_DOCUMENTATION_BUILD)
/**
 * \brief Move to the application bundle's parent directory
 *
 * This is function is convenient when deploying .app bundles on OSX. It
 * adjusts the file path to the parent directory containing the bundle.
 */
extern NANOGUI_EXPORT void chdir_to_bundle_parent();
#endif

/**
 * \brief Convert a single UTF32 character code to UTF8.
 *
 * \rst
 * NanoGUI uses this to convert the icon character codes
 * defined in :ref:`file_nanogui_entypo.h`.
 * \endrst
 *
 * \param c
 *     The UTF32 character to be converted.
 */
extern NANOGUI_EXPORT std::string utf8(uint32_t c);

/// Load a directory of PNG images and upload them to the GPU (suitable for use with ImagePanel)
extern NANOGUI_EXPORT std::vector<std::pair<int, std::string>>
    load_image_directory(NVGcontext *ctx, const std::string &path);

/// Convenience function for instanting a PNG icon from the application's data segment (via bin2c)
#define nvgImageIcon(ctx, name) nanogui::__nanogui_get_image(ctx, #name, name##_png, name##_png_size)
/// Helper function used by nvg_image_icon
extern NANOGUI_EXPORT int __nanogui_get_image(NVGcontext *ctx, const std::string &name,
                                              uint8_t *data, uint32_t size);

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
