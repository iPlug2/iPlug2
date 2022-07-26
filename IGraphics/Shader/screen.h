///*
//    nanogui/screen.h -- Top-level widget and interface between NanoGUI and GLFW
//
//    A significant redesign of this code was contributed by Christian Schueller.
//
//    NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
//    The widget drawing code is based on the NanoVG demo application
//    by Mikko Mononen.
//
//    All rights reserved. Use of this source code is governed by a
//    BSD-style license that can be found in the LICENSE.txt file.
//*/
///** \file */
//
//#pragma once
//
//#include <nanogui/widget.h>
//#include <nanogui/texture.h>
//
//BEGIN_IPLUG_NAMESPACE
//BEGIN_IGRAPHICS_NAMESPACE
//
//class Texture;
//
///**
// * \class Screen screen.h nanogui/screen.h
// *
// * \brief Represents a display surface (i.e. a full-screen or windowed GLFW window)
// * and forms the root element of a hierarchy of nanogui widgets.
// */
//class NANOGUI_EXPORT Screen : public Widget {
//    friend class Widget;
//    friend class Window;
//public:
//    /**
//     * Create a new Screen instance
//     *
//     * \param size
//     *     Size in pixels at 96 dpi (on high-DPI screens, the actual resolution
//     *     in terms of hardware pixels may be larger by an integer factor)
//     *
//     * \param caption
//     *     Window title (in UTF-8 encoding)
//     *
//     * \param resizable
//     *     If creating a window, should it be resizable?
//     *
//     * \param fullscreen
//     *     Specifies whether to create a windowed or full-screen view
//     *
//     * \param stencil_buffer
//     *     Should an 8-bit stencil buffer be allocated? NanoVG requires this to
//     *     rasterize non-convex polygons. (NanoGUI does not render such
//     *     polygons, but your application might.)
//     *
//     * \param float_buffer
//     *     Should NanoGUI try to allocate a floating point framebuffer? This
//     *     is useful for HDR and wide-gamut displays.
//     *
//     * \param gl_major
//     *     The requested OpenGL Major version number.  The default is 3, if
//     *     changed the value must correspond to a forward compatible core
//     *     profile (for portability reasons).  For example, set this to 4 and
//     *     \ref gl_minor to 1 for a forward compatible core OpenGL 4.1 profile.
//     *     Requesting an invalid profile will result in no context (and
//     *     therefore no GUI) being created. This attribute is ignored when
//     *     targeting OpenGL ES 2 or Metal.
//     *
//     * \param gl_minor
//     *     The requested OpenGL Minor version number.  The default is 2, if
//     *     changed the value must correspond to a forward compatible core
//     *     profile (for portability reasons).  For example, set this to 1 and
//     *     \ref gl_major to 4 for a forward compatible core OpenGL 4.1 profile.
//     *     Requesting an invalid profile will result in no context (and
//     *     therefore no GUI) being created. This attribute is ignored when
//     *     targeting OpenGL ES 2 or Metal.
//     */
//    Screen(
//        const Vector2i &size,
//        const std::string &caption = "Unnamed",
//        bool resizable = true,
//        bool fullscreen = false,
//        bool depth_buffer = true,
//        bool stencil_buffer = true,
//        bool float_buffer = false,
//        unsigned int gl_major = 3,
//        unsigned int gl_minor = 2
//    );
//
//    /// Release all resources
//    virtual ~Screen();
//
//    /// Get the window title bar caption
//    const std::string &caption() const { return m_caption; }
//
//    /// Set the window title bar caption
//    void set_caption(const std::string &caption);
//
//    /// Return the screen's background color
//    const Color &background() const { return m_background; }
//
//    /// Set the screen's background color
//    void set_background(const Color &background) { m_background = background; }
//
//    /// Set the top-level window visibility (no effect on full-screen windows)
//    void set_visible(bool visible);
//
//    /// Set window size
//    void set_size(const Vector2i& size);
//
//    /// Return the framebuffer size (potentially larger than size() on high-DPI screens)
//    const Vector2i &framebuffer_size() const { return m_fbsize; }
//
//    /// Send an event that will cause the screen to be redrawn at the next event loop iteration
//    void redraw();
//
//    /**
//     * \brief Redraw the screen if the redraw flag is set
//     *
//     * This function does everything -- it calls \ref draw_setup(), \ref
//     * draw_contents() (which also clears the screen by default), \ref draw(),
//     * and finally \ref draw_teardown().
//     *
//     * \sa redraw
//     */
//    virtual void draw_all();
//
//    /**
//     * \brief Clear the screen with the background color (glClearColor, glClear, etc.)
//     *
//     * You typically won't need to call this function yourself, as it is called by
//     * the default implementation of \ref draw_contents() (which is called by \ref draw_all())
//     */
//    virtual void clear();
//
//    /**
//     * \brief Prepare the graphics pipeline for the next frame
//     *
//     * This involves steps such as obtaining a drawable, querying the drawable
//     * resolution, setting the viewport used for drawing, etc..
//     *
//     * You typically won't need to call this function yourself, as it is called
//     * by \ref draw_all(), which is executed by the run loop.
//     */
//    virtual void draw_setup();
//
//    /// Calls clear() and draws the window contents --- put your rendering code here.
//    virtual void draw_contents();
//
//    /**
//     * \brief Wrap up drawing of the current frame
//     *
//     * This involves steps such as releasing the current drawable, swapping
//     * the framebuffer, etc.
//     *
//     * You typically won't need to call this function yourself, as it is called
//     * by \ref draw_all(), which is executed by the run loop.
//     */
//    virtual void draw_teardown();
//
//    /// Return the ratio between pixel and device coordinates (e.g. >= 2 on Mac Retina displays)
//    float pixel_ratio() const { return m_pixel_ratio; }
//
//    /// Handle a file drop event
//    virtual bool drop_event(const std::vector<std::string> & /* filenames */) {
//        return false; /* To be overridden */
//    }
//
//    /// Default keyboard event handler
//    virtual bool keyboard_event(int key, int scancode, int action, int modifiers);
//
//    /// Text input event handler: codepoint is native endian UTF-32 format
//    virtual bool keyboard_character_event(unsigned int codepoint);
//
//    /// Window resize event handler
//    virtual bool resize_event(const Vector2i& size);
//
//    /// Set the resize callback
//    std::function<void(Vector2i)> resize_callback() const { return m_resize_callback; }
//    void set_resize_callback(const std::function<void(Vector2i)> &callback) {
//        m_resize_callback = callback;
//    }
//
//    /// Return the last observed mouse position value
//    Vector2i mouse_pos() const { return m_mouse_pos; }
//
//    /// Return a pointer to the underlying GLFW window data structure
//    GLFWwindow *glfw_window() const { return m_glfw_window; }
//
//    /// Return a pointer to the underlying NanoVG draw context
//    NVGcontext *nvg_context() const { return m_nvg_context; }
//
//    /// Return the component format underlying the screen
//    Texture::ComponentFormat component_format() const;
//
//    /// Return the pixel format underlying the screen
//    Texture::PixelFormat pixel_format() const;
//
//    /// Does the framebuffer have a depth buffer
//    bool has_depth_buffer() const { return m_depth_buffer; }
//
//    /// Does the framebuffer have a stencil buffer
//    bool has_stencil_buffer() const { return m_stencil_buffer; }
//
//    /// Does the framebuffer use a floating point representation
//    bool has_float_buffer() const { return m_float_buffer; }
//
//#if defined(NANOGUI_USE_METAL)
//    /// Return the associated CAMetalLayer object
//    void *metal_layer() const;
//
//    /// Return the texure of the currently active Metal drawable (or NULL)
//    void *metal_texture() const { return m_metal_texture; }
//
//    /// Return the associated depth/stencil texture
//    Texture *depth_stencil_texture() { return m_depth_stencil_texture; }
//#endif
//
//    /// Flush all queued up NanoVG rendering commands
//    void nvg_flush();
//
//    /// Shut down GLFW when the window is closed?
//    void set_shutdown_glfw(bool v) { m_shutdown_glfw = v; }
//    bool shutdown_glfw() { return m_shutdown_glfw; }
//
//    /// Is a tooltip currently fading in?
//    bool tooltip_fade_in_progress() const;
//
//    using Widget::perform_layout;
//
//    /// Compute the layout of all widgets
//    void perform_layout() {
//        this->perform_layout(m_nvg_context);
//    }
//
//public:
//    /********* API for applications which manage GLFW themselves *********/
//
//    /**
//     * \brief Default constructor
//     *
//     * Performs no initialization at all. Use this if the application is
//     * responsible for setting up GLFW, OpenGL, etc.
//     *
//     * In this case, override \ref Screen and call \ref initalize() with a
//     * pointer to an existing \c GLFWwindow instance
//     *
//     * You will also be responsible in this case to deliver GLFW callbacks
//     * to the appropriate callback event handlers below
//     */
//    Screen();
//
//    /// Initialize the \ref Screen
//    void initialize(GLFWwindow *window, bool shutdown_glfw);
//
//    /* Event handlers */
//    void cursor_pos_callback_event(double x, double y);
//    void mouse_button_callback_event(int button, int action, int modifiers);
//    void key_callback_event(int key, int scancode, int action, int mods);
//    void char_callback_event(unsigned int codepoint);
//    void drop_callback_event(int count, const char **filenames);
//    void scroll_callback_event(double x, double y);
//    void resize_callback_event(int width, int height);
//
//    /* Internal helper functions */
//    void update_focus(Widget *widget);
//    void dispose_window(Window *window);
//    void center_window(Window *window);
//    void move_window_to_front(Window *window);
//    void draw_widgets();
//
//protected:
//    GLFWwindow *m_glfw_window = nullptr;
//    NVGcontext *m_nvg_context = nullptr;
//    GLFWcursor *m_cursors[(size_t) Cursor::CursorCount];
//    Cursor m_cursor;
//    std::vector<Widget *> m_focus_path;
//    Vector2i m_fbsize;
//    float m_pixel_ratio;
//    int m_mouse_state, m_modifiers;
//    Vector2i m_mouse_pos;
//    bool m_drag_active;
//    Widget *m_drag_widget = nullptr;
//    double m_last_interaction;
//    bool m_process_events = true;
//    Color m_background;
//    std::string m_caption;
//    bool m_shutdown_glfw;
//    bool m_fullscreen;
//    bool m_depth_buffer;
//    bool m_stencil_buffer;
//    bool m_float_buffer;
//    bool m_redraw;
//    std::function<void(Vector2i)> m_resize_callback;
//#if defined(NANOGUI_USE_METAL)
//    void *m_metal_texture = nullptr;
//    void *m_metal_drawable = nullptr;
//    ref<Texture> m_depth_stencil_texture;
//#endif
//};
//
//END_IGRAPHICS_NAMESPACE
//END_IPLUG_NAMESPACE
