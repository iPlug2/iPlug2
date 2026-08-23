.. default-domain:: c
.. highlight:: c

###############
Creating a View
###############

A view is a drawable region that receives events.
You may think of it as a window,
though it may be embedded and not represent a top-level system window. [#f1]_

Creating a visible view is a multi-step process.
When a new view is created with :func:`puglNewView`,
it does not yet represent a "real" system view:

.. code-block:: c

   PuglView* view = puglNewView(world);

*********************
Configuring the Frame
*********************

Before display,
the necessary :doc:`frame <api/pugl_frame>` and :doc:`window <api/pugl_window>` attributes should be set.
These allow the window system (or plugin host) to arrange the view properly.
For example:

.. code-block:: c

   const PuglSpan defaultWidth  = 1920;
   const PuglSpan defaultHeight = 1080;

   puglSetViewString(view, PUGL_WINDOW_TITLE, "My Window");
   puglSetSizeHint(view, PUGL_DEFAULT_SIZE, 1920, 1080);
   puglSetSizeHint(view, PUGL_MIN_SIZE, 640, 480);
   puglSetSizeHint(view, PUGL_MIN_ASPECT, 1, 1);
   puglSetSizeHint(view, PUGL_MAX_ASPECT, 16, 9);

There are also several :enum:`hints <PuglViewHint>` for basic attributes that can be set:

.. code-block:: c

   puglSetViewHint(view, PUGL_RESIZABLE, PUGL_TRUE);
   puglSetViewHint(view, PUGL_IGNORE_KEY_REPEAT, PUGL_TRUE);

*********
Embedding
*********

To embed the view in another window,
you will need to somehow get the :type:`native view handle <PuglNativeView>` for the parent,
then set it with :func:`puglSetParentWindow`.
If the parent is a Pugl view,
the native handle can be accessed with :func:`puglGetNativeView`.
For example:

.. code-block:: c

   puglSetParentWindow(view, puglGetNativeView(parent));

************************
Setting an Event Handler
************************

In order to actually do anything, a view must process events from the system.
Pugl dispatches all events to a single :type:`event handling function <PuglEventFunc>`,
which is set with :func:`puglSetEventFunc`:

.. code-block:: c

   puglSetEventFunc(view, onEvent);

See :doc:`events` for details on writing the event handler itself.

*****************
Setting View Data
*****************

Since the event handler is called with only a view pointer and an event,
there needs to be some way to access application data associated with the view.
Similar to :ref:`setting application data <setting-application-data>`,
this is done by setting an opaque handle on the view with :func:`puglSetHandle`,
for example:

.. code-block:: c

   puglSetHandle(view, myViewData);

The handle can be later retrieved,
likely in the event handler,
with :func:`puglGetHandle`:

.. code-block:: c

   MyViewData* data = (MyViewData*)puglGetHandle(view);

All non-constant data should be accessed via this handle,
to avoid problems associated with static mutable data.

If data is also associated with the world,
it can be retrieved via the view using :func:`puglGetWorld`:

.. code-block:: c

   PuglWorld* world = puglGetWorld(view);
   MyApp*     app   = (MyApp*)puglGetWorldHandle(world);

*****************
Setting a Backend
*****************

Before being realized, the view must have a backend set with :func:`puglSetBackend`.

The backend manages the graphics API that will be used for drawing.
Pugl includes backends and supporting API for
:doc:`Cairo <api/pugl_cairo>`, :doc:`OpenGL <api/pugl_gl>`, and :doc:`Vulkan <api/pugl_vulkan>`.

Using Cairo
===========

Cairo-specific API is declared in the ``cairo.h`` header:

.. code-block:: c

   #include <pugl/cairo.h>

The Cairo backend is provided by :func:`puglCairoBackend()`:

.. code-block:: c

   puglSetBackend(view, puglCairoBackend());

No additional configuration is required for Cairo.
To draw when handling an expose event,
the `Cairo context <https://www.cairographics.org/manual/cairo-cairo-t.html>`_ can be accessed with :func:`puglGetContext`:

.. code-block:: c

   cairo_t* cr = (cairo_t*)puglGetContext(view);

Using OpenGL
============

OpenGL-specific API is declared in the ``gl.h`` header:

.. code-block:: c

   #include <pugl/gl.h>

The OpenGL backend is provided by :func:`puglGlBackend()`:

.. code-block:: c

   puglSetBackend(view, puglGlBackend());

Some hints must also be set so that the context can be set up correctly.
For example, to use OpenGL 3.3 Core Profile:

.. code-block:: c

   puglSetViewHint(view, PUGL_CONTEXT_VERSION_MAJOR, 3);
   puglSetViewHint(view, PUGL_CONTEXT_VERSION_MINOR, 3);
   puglSetViewHint(view,
                   PUGL_CONTEXT_PROFILE,
                   PUGL_OPENGL_COMPATIBILITY_PROFILE);

If you need to perform some setup using the OpenGL API,
there are two ways to do so.

The OpenGL context is active when
:enumerator:`PUGL_REALIZE <PuglEventType.PUGL_REALIZE>` and
:enumerator:`PUGL_UNREALIZE <PuglEventType.PUGL_UNREALIZE>`
events are dispatched,
so things like creating and destroying shaders and textures can be done then.

Alternatively, if it is cumbersome to set up and tear down OpenGL in the event handler,
:func:`puglEnterContext` and :func:`puglLeaveContext` can be used to manually activate the OpenGL context during application setup.
Note, however, that unlike many other APIs, these functions must not be used for drawing.
It is only valid to use the OpenGL API for configuration in a manually entered context,
rendering will not work.
For example:

.. code-block:: c

   puglEnterContext(view);
   setupOpenGL(myApp);
   puglLeaveContext(view);

   while (!myApp->quit) {
     puglUpdate(world, 0.0);
   }

   puglEnterContext(view);
   teardownOpenGL(myApp);
   puglLeaveContext(view);

Using Vulkan
============

Vulkan-specific API is declared in the ``vulkan.h`` header.
This header includes Vulkan headers,
so if you are dynamically loading Vulkan at runtime,
you should define ``VK_NO_PROTOTYPES`` before including it.

.. code-block:: c

   #define VK_NO_PROTOTYPES

   #include <pugl/vulkan.h>

The Vulkan backend is provided by :func:`puglVulkanBackend()`:

.. code-block:: c

   puglSetBackend(view, puglVulkanBackend());

Unlike OpenGL, almost all Vulkan configuration is done using the Vulkan API directly.
Pugl only provides a portable mechanism to load the Vulkan library and get the functions used to load the rest of the Vulkan API.

Loading Vulkan
--------------

For maximum compatibility,
it is best to not link to Vulkan at compile-time,
but instead load the Vulkan API at run-time.
To do so, first create a :struct:`PuglVulkanLoader`:

.. code-block:: c

   PuglVulkanLoader* loader = puglNewVulkanLoader(world, NULL);

The loader manages the dynamically loaded Vulkan library,
so it must be kept alive for as long as the application is using Vulkan.
You can get the function used to load Vulkan functions with :func:`puglGetInstanceProcAddrFunc`:

.. code-block:: c

   PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr =
     puglGetInstanceProcAddrFunc(loader);

This vkGetInstanceProcAddr_ function can be used to load the rest of the Vulkan API.
For example, you can use it to get the vkCreateInstance_ function,
then use that to create your Vulkan instance.
In practice, you will want to use some loader or wrapper API since there are many Vulkan functions.

For advanced situations,
there is also :func:`puglGetDeviceProcAddrFunc` which retrieves the vkGetDeviceProcAddr_ function instead.

The Vulkan loader is provided for convenience,
so that applications to not need to write platform-specific code to load Vulkan.
Its use it not mandatory and Pugl can be used with Vulkan loaded by some other method.

Linking with Vulkan
-------------------

If you do want to link to the Vulkan library at compile time,
note that the Pugl Vulkan backend does not depend on it,
so you will have to do so explicitly.

Creating a Surface
------------------

The details of using Vulkan are far beyond the scope of this documentation,
but Pugl provides a portable function, :func:`puglCreateSurface`,
to get the Vulkan surface for a view.
Assuming you have somehow created your ``VkInstance``,
you can get the surface for a view using :func:`puglCreateSurface`:

.. code-block:: c

   VkSurfaceKHR* surface = NULL;
   puglCreateSurface(puglGetDeviceProcAddrFunc(loader),
                     view,
                     vulkanInstance,
                     NULL,
                     &surface);

****************
Showing the View
****************

Once the view is configured, it can be "realized" with :func:`puglRealize`.
This creates a "real" system view, for example:

.. code-block:: c

   PuglStatus status = puglRealize(view);
   if (status) {
     fprintf(stderr, "Error realizing view (%s)\n", puglStrerror(status));
   }

Note that realizing a view can fail for many reasons,
so the return code should always be checked.
This is generally the case for any function that interacts with the window system.
Most functions also return a :enum:`PuglStatus`,
but these checks are omitted for brevity in the rest of this documentation.

A realized view is not initially visible,
but can be shown with :func:`puglShow`:

.. code-block:: c

   puglShow(view);

To create an initially visible view,
it is also possible to simply call :func:`puglShow` right away.
The view will be automatically realized if necessary.

.. rubric:: Footnotes

.. [#f1] MacOS has a strong distinction between
   `views <https://developer.apple.com/documentation/appkit/nsview>`_,
   which may be nested, and
   `windows <https://developer.apple.com/documentation/appkit/nswindow>`_,
   which may not.
   On Windows and X11, everything is a nestable window,
   but top-level windows are configured differently.

.. _vkCreateInstance: https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/vkCreateInstance.html

.. _vkGetDeviceProcAddr: https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/vkGetDeviceProcAddr.html

.. _vkGetInstanceProcAddr: https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/vkGetInstanceProcAddr.html
