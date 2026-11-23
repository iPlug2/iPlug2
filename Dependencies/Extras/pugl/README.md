Pugl
====

Pugl (PlUgin Graphics Library) is a minimal portability layer for GUIs which is
suitable for use in plugins and applications.  It works on X11, MacOS, and
Windows, and includes optional support for drawing with Vulkan, OpenGL, and
Cairo.

Pugl is vaguely similar to libraries like GLUT and GLFW, but has different
goals and priorities:

 * Minimal in scope, providing only a thin interface to isolate
   platform-specific details from applications.

 * Zero dependencies, aside from standard system libraries.

 * Support for embedding in native windows, for example as a plugin or
   component within a larger application that is not based on Pugl.

 * Explicit context and no static data, so that several instances can be used
   within a single program at once.

 * Consistent event-based API that makes dispatching in application or toolkit
   code easy with minimal boilerplate.

 * Suitable for both continuously rendering applications like games, and
   event-driven applications that only draw when necessary.

 * Well-integrated with windowing systems, with support for tracking and
   manipulating special window types, states, and styles.

 * Small, liberally licensed implementation that is suitable for vendoring
   and/or static linking.  Pugl can be installed as a library, or used by
   simply copying the implementation into a project.

Stability
---------

Pugl is currently being developed towards a long-term stable API.  For the time
being, however, the API may break occasionally.  Please report any relevant
feedback, or file feature requests, so that we can ensure that the released API
is stable for as long as possible.

When the API changes, backwards compatibility is maintained where possible.
These compatibility shims will be removed before release, so users are
encouraged to build with `PUGL_DISABLE_DEPRECATED` defined.

Documentation
-------------

Pugl is a C library that includes C++ bindings.
The reference documentation refers to the C API:

 * [C Documentation (single page)](https://lv2.gitlab.io/pugl/c/singlehtml/)
 * [C Documentation (paginated)](https://lv2.gitlab.io/pugl/c/html/)

The documentation will also be built from the source if the `docs`
configuration option is enabled, and both Doxygen and Sphinx are available.

The C++ documentation is currently a work in progress, for now you will have to
refer to the examples or headers for guidance on using the C++ bindings.

Testing
-------

Some unit tests are included, but unfortunately manual testing is still
required.  The tests and example programs are built by default.  You can run
all the tests at once via ninja:

    meson setup build
    cd build
    ninja test

The [examples](examples) directory contains several demonstration programs that
can be used for manual testing.

 -- David Robillard <d@drobilla.net>
