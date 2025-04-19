Pugl is a minimal portability layer for embeddable GUIs.
It provides a drawing context and event-based main loop API,
which can be used to create graphical applications or embedded views.

Pugl is particularly suitable for plugins,
since it has no implicit context or mutable static data,
and can be statically linked.
The "core" library implements platform support,
and depends only on standard system libraries.
MacOS, Windows, and X11 are currently supported.

Graphics backends are built as separate libraries,
so applications depend only on the APIs that they use.
Pugl includes graphics backends for Cairo_, OpenGL_, and Vulkan_.
Other graphics APIs can be used by implementing a custom backend.

.. _Cairo: https://www.cairographics.org/
.. _OpenGL: https://www.opengl.org/
.. _Vulkan: https://www.khronos.org/vulkan/
