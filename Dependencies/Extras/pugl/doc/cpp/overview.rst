.. default-domain:: cpp
.. highlight:: cpp
.. namespace:: pugl

########
Overview
########

Pugl is a C library,
but the bindings documented here provide a more idiomatic and type-safe API for C++.
If you would rather use C,
refer instead to the `C API documentation <../../c/singlehtml/index.html>`_.

The C++ bindings are very lightweight and do not require virtual functions,
RTTI,
exceptions,
or linking to the C++ standard library.
They are provided by the package ``puglpp-0`` which must be used in addition to the desired platform+backend package above.

The core API (excluding backend-specific components) is declared in ``pugl.hpp``:

.. code-block:: cpp

   #include <pugl/pugl.hpp>

The API revolves around two main objects: the `world` and the `view`.
An application creates a world to manage top-level state,
then creates one or more views to display.

.. toctree::

   world
   view
   events
   event-loop
