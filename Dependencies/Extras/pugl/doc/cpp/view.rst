.. default-domain:: cpp
.. highlight:: cpp
.. namespace:: pugl

###############
Creating a View
###############

A `view` is a drawable region that receives events.
You may think of it as a window,
though it may be embedded and not represent a top-level system window. [#f1]_

Pugl communicates with views by dispatching events.
For flexibility, the event handler can be a different object than the view.
This allows using :class:`View` along with a separate event handler class.
Alternatively, a view class can inherit from :class:`View` and set itself as its event handler,
for a more object-oriented style.

This documentation will use the latter approach,
so we will define a class for our view that contains everything needed:

.. code-block:: cpp

   class MyView : public pugl::View
   {
   public:
     explicit MyView(pugl::World& world)
         : pugl::View{world}
     {
       setEventHandler(*this);
     }

     pugl::Status onEvent(const pugl::ConfigureEvent& event) noexcept;
     pugl::Status onEvent(const pugl::ExposeEvent& event) noexcept;

     // With other handlers here as needed...

     // Fallback handler for all other events
     template<PuglEventType t, class Base>
     pugl::Status onEvent(const pugl::Event<t, Base>&) noexcept
     {
          return pugl::Status::success;
     }

   private:
     // Some data...
   };

Pugl will call an ``onEvent`` method of the event handler (the view in this case) for every event.

Note that Pugl uses a static dispatching mechanism rather than virtual functions to minimize overhead.
It is therefore necessary for the final class to define a handler for every event type.
A terse way to do this without writing every implementation is to define a fallback handler as a template,
as in the example above.
Alternatively, you can define an explicit handler for each event that simply returns :enumerator:`Status::success`.
This way, it will be a compile error if any event is not explicitly handled.

*********************
Configuring the Frame
*********************

Before display,
the necessary :doc:`frame <api/puglpp_frame>` and :doc:`window <api/puglpp_window>` attributes should be set.
These allow the window system (or plugin host) to arrange the view properly.

Derived classes can configure themselves during construction,
but we assume here that configuration is being done outside the view.
For example:

.. code-block:: cpp

   const double defaultWidth = 1920.0;
   const double defaultHeight = 1080.0;

   view.setWindowTitle("My Window");
   view.setDefaultSize(defaultWidth, defaultHeight);
   view.setMinSize(defaultWidth / 4.0, defaultHeight / 4.0);
   view.setAspectRatio(1, 1, 16, 9);

There are also several :type:`hints <PuglViewHint>` for basic attributes that can be set:

.. code-block:: cpp

   view.setHint(pugl::ViewHint::resizable, true);
   view.setHint(pugl::ViewHint::ignoreKeyRepeat, true);

*********
Embedding
*********

To embed the view in another window,
you will need to somehow get the :type:`native view handle <pugl::NativeView>` for the parent,
then set it with :func:`View::setParentWindow`.
If the parent is a Pugl view,
the native handle can be accessed with :func:`View::nativeView`.
For example:

.. code-block:: cpp

   view.setParentWindow(view, parent.getNativeView());

*****************
Setting a Backend
*****************

Before being realized, the view must have a backend set with :func:`View::setBackend`.

The backend manages the graphics API that will be used for drawing.
Pugl includes backends and supporting API for
:doc:`Cairo <api/cairo>`, :doc:`OpenGL <api/gl>`, and :doc:`Vulkan <api/vulkan>`.

Using Cairo
===========

Cairo-specific API is declared in the ``cairo.hpp`` header:

.. code-block:: cpp

   #include <pugl/cairo.hpp>

The Cairo backend is provided by :func:`cairoBackend()`:

.. code-block:: cpp

   view.setBackend(pugl::cairoBackend());

No additional configuration is required for Cairo.
To draw when handling an expose event,
the `Cairo context <https://www.cairographics.org/manual/cairo-cairo-t.html>`_ can be accessed with :func:`View::context`:

.. code-block:: cpp

   cairo_t* cr = static_cast<cairo_t*>(view.context());

Using OpenGL
============

OpenGL-specific API is declared in the ``gl.hpp`` header:

.. code-block:: cpp

   #include <pugl/gl.hpp>

The OpenGL backend is provided by :func:`glBackend()`:

.. code-block:: cpp

   view.setBackend(pugl::glBackend());

Some hints must also be set so that the context can be set up correctly.
For example, to use OpenGL 3.3 Core Profile:

.. code-block:: cpp

   view.setHint(pugl::ViewHint::useCompatProfile, false);
   view.setHint(pugl::ViewHint::contextVersionMajor, 3);
   view.setHint(pugl::ViewHint::contextVersionMinor, 3);

If you need to perform some setup using the OpenGL API,
there are two ways to do so.

The OpenGL context is active when
:type:`RealizeEvent` and
:type:`UnrealizeEvent`
events are dispatched,
so things like creating and destroying shaders and textures can be done then.

Alternatively, if it is cumbersome to set up and tear down OpenGL in the event handler,
:func:`enterContext` and :func:`leaveContext` can be used to manually activate the OpenGL context during application setup.
Note, however, that unlike many other APIs, these functions must not be used for drawing.
It is only valid to use the OpenGL API for configuration in a manually entered context,
rendering will not work.
For example:

.. code-block:: cpp

   pugl::enterContext(view);
   myApp.setupOpenGL();
   pugl::leaveContext(view);

   while (!myApp.quit()) {
     world.update(0.0);
   }

   pugl::enterContext(view);
   myApp.teardownOpenGL();
   pugl::leaveContext(view);

Using Vulkan
============

Vulkan-specific API is declared in the ``vulkan.hpp`` header.
This header includes Vulkan headers,
so if you are dynamically loading Vulkan at runtime,
you should define ``VK_NO_PROTOTYPES`` before including it.

.. code-block:: cpp

   #define VK_NO_PROTOTYPES

   #include <pugl/vulkan.hpp>

The Vulkan backend is provided by :func:`vulkanBackend()`:

.. code-block:: cpp

   view.setBackend(pugl::vulkanBackend());

Unlike OpenGL, almost all Vulkan configuration is done using the Vulkan API directly.
Pugl only provides a portable mechanism to load the Vulkan library and get the functions used to load the rest of the Vulkan API.

Loading Vulkan
--------------

For maximum compatibility,
it is best to not link to Vulkan at compile-time,
but instead load the Vulkan API at run-time.
To do so, first create a :class:`VulkanLoader`:

.. code-block:: cpp

   pugl::VulkanLoader loader{world};

The loader manages the dynamically loaded Vulkan library,
so it must be kept alive for as long as the application is using Vulkan.
You can get the function used to load Vulkan functions with :func:`VulkanLoader::getInstanceProcAddrFunc`:

.. code-block:: cpp

   auto vkGetInstanceProcAddr = loader.getInstanceProcAddrFunc();

It is best to use this function to load everything at run time,
rather than link to the Vulkan library at run time.
You can, for example, pass this to get the ``vkCreateInstance`` function using this,
then use that to create your Vulkan instance.
In practice, you will want to use some loader or wrapper API since there are many Vulkan functions.

It is not necessary to use :class:`VulkanLoader`,
you can, for example, use the ``DynamicLoader`` from ``vulkan.hpp`` in the Vulkan SDK instead.

The details of using Vulkan are far beyond the scope of this documentation,
but Pugl provides a portable function, :func:`createSurface`,
to get the Vulkan surface for a view.
Assuming you have somehow created your ``VkInstance``,
you can get the surface for a view using :func:`createSurface`:

.. code-block:: cpp

   VkSurfaceKHR* surface = nullptr;
   puglCreateSurface(loader.getDeviceProcAddrFunc(),
                     view,
                     vulkanInstance,
                     nullptr,
                     &surface);

Pugl does not provide API that uses ``vulkan.hpp`` to avoid the onerous dependency,
but if you are using it with exceptions and unique handles,
it is straightforward to wrap the surface handle yourself.

****************
Showing the View
****************

Once the view is configured, it can be "realized" with :func:`View::realize`.
This creates a "real" system view, for example:

.. code-block:: cpp

   pugl::Status status = view.realize();
   if (status != pugl::Status::success) {
     std::cerr << "Error realizing view: " << pugl::strerror(status) << "\n";
   }

Note that realizing a view can fail for many reasons,
so the return code should always be checked.
This is generally the case for any function that interacts with the window system.
Most functions also return a :enum:`Status`,
but these checks are omitted for brevity in the rest of this documentation.

A realized view is not initially visible,
but can be shown with :func:`View::show`:

.. code-block:: cpp

   view.show();

To create an initially visible view,
it is also possible to simply call :func:`View::show()` right away.
The view will be automatically realized if necessary.

.. rubric:: Footnotes

.. [#f1] MacOS has a strong distinction between
   `views <https://developer.apple.com/documentation/appkit/nsview>`_,
   which may be nested, and
   `windows <https://developer.apple.com/documentation/appkit/nswindow>`_,
   which may not.
   On Windows and X11, everything is a nestable window,
   but top-level windows are configured differently.
