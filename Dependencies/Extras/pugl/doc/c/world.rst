################
Creating a World
################

.. default-domain:: c
.. highlight:: c

The world is the top-level object which represents an instance of Pugl.
It handles the connection to the window system,
and manages views and the event loop.

An application typically has a single world,
which is constructed once on startup and used to drive the main event loop.

************
Construction
************

A world must be created before any views, and it must outlive all of its views.
A world is created with :func:`puglNewWorld`, for example:

.. code-block:: c

   PuglWorld* world = puglNewWorld(PUGL_PROGRAM, 0);

For a plugin, specify :enumerator:`PUGL_MODULE <PuglWorldType.PUGL_MODULE>` instead.
In some cases, it is necessary to pass additional flags.
For example, Vulkan requires thread support:

.. code-block:: c

   PuglWorld* world = puglNewWorld(PUGL_MODULE, PUGL_WORLD_THREADS)

It is a good idea to set a class name for your project with :func:`puglSetWorldString`.
This allows the window system to distinguish different applications and,
for example, users to set up rules to manage their windows nicely:

.. code-block:: c

   puglSetWorldString(world, PUGL_CLASS_NAME, "MyAwesomeProject")

.. _setting-application-data:

************************
Setting Application Data
************************

Pugl will call an event handler in the application with only a view pointer and an event,
so there needs to be some way to access the data you use in your application.
This is done by setting an opaque handle on the world with :func:`puglSetWorldHandle`,
for example:

.. code-block:: c

   puglSetWorldHandle(world, myApp);

The handle can be later retrieved with :func:`puglGetWorldHandle`:

.. code-block:: c

   MyApp* app = (MyApp*)puglGetWorldHandle(world);

All non-constant data should be accessed via this handle,
to avoid problems associated with static mutable data.

