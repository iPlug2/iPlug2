.. default-domain:: c
.. highlight:: c

########
Overview
########

The Pugl API revolves around two main objects: the `world` and the `view`.
An application creates a world to manage top-level state,
then creates one or more views to display.

The core API (excluding backend-specific components) is declared in ``pugl.h``:

.. code-block:: c

   #include <pugl/pugl.h>

.. toctree::

   world
   view
   events
   event-loop
   clipboards
   shutting-down

.. _pkg-config: https://www.freedesktop.org/wiki/Software/pkg-config/
