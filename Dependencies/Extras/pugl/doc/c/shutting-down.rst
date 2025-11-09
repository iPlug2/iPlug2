.. default-domain:: c
.. highlight:: c

#############
Shutting Down
#############

When a view is closed,
it will receive a :struct:`PuglCloseEvent`.
An application may also set a flag based on user input or other conditions,
which can be used to break out of the main loop and stop calling :func:`puglUpdate`.

When the main event loop has finished running,
any views and the world need to be destroyed, in that order.
For example:

.. code-block:: c

   puglFreeView(view);
   puglFreeWorld(world);
