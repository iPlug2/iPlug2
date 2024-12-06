#####
Usage
#####

**************************
Using an Installed Version
**************************

When Pugl is installed,
particularly on POSIX systems,
pkg-config_ packages are provided that link with the core platform library and desired backend:

.. code-block:: sh

   pkg-config --cflags --libs pugl-0
   pkg-config --cflags --libs pugl-cairo-0
   pkg-config --cflags --libs pugl-gl-0
   pkg-config --cflags --libs pugl-vulkan-0

Depending on one of these packages should be all that is necessary to use Pugl.
If pkg-config is not available,
details on the individual libraries that are installed are available in the README.

************************
Using a Meson Subproject
************************

Pugl uses the meson_ build system,
which allows it to be included as a subproject within other meson projects.

To use Pugl as a subproject,
copy the source tree (or use a git submodule or subtree) to your ``subprojects`` directory,
for example to ``subprojects/pugl``,
and use the ``subproject`` function in meson to include it:

.. code-block:: meson

   pugl_proj = subproject('pugl')

See the `Meson subproject documentation <https://mesonbuild.com/Subprojects.html>`_ for details.

******************
Vendoring Manually
******************

To "vendor" Pugl with projects that use a different build system,
the headers in the ``include`` subdirectory and sources in ``src`` are needed.
The ``include`` directory needs to be added to the search path for the compiler,
and the required source files need to be compiled.
It is only necessary to build the needed platform and backend implementations.
Due to the modular design,
no centralized configuration is needed to enable or disable platform/backend support.
However, a few preprocessor symbols can be used to control which X11 features are used,
see the top of ``src/x11.c`` for details.

.. _meson: https://mesonbuild.com/
.. _pkg-config: https://www.freedesktop.org/wiki/Software/pkg-config/
