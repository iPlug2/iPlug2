.. default-domain:: cpp
.. highlight:: cpp
.. namespace:: pugl

################
Creating a World
################

The world is the top-level object which represents an instance of Pugl.
It handles the connection to the window system,
and manages views and the event loop.

An application typically has a single world,
which is constructed once on startup and used to drive the main event loop.

************
Construction
************

A world must be created before any views, and it must outlive all of its views.
The world constructor requires an argument to specify the application type:

.. code-block:: cpp

   pugl::World world{pugl::WorldType::program};

For a plugin, specify :enumerator:`WorldType::module` instead.
In some cases, it is necessary to pass additional flags.
For example, Vulkan requires thread support:

.. code-block:: cpp

   pugl::World world{pugl::WorldType::program, pugl::WorldFlag::threads};

It is a good idea to set a class name for your project with :func:`World::setClassName`.
This allows the window system to distinguish different applications and,
for example, users to set up rules to manage their windows nicely:

.. code-block:: cpp

   world.setClassName("MyAwesomeProject");
