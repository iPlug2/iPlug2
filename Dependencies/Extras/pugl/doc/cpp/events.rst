.. default-domain:: cpp
.. highlight:: cpp
.. namespace:: pugl

###############   
Handling Events
###############

Events are sent to a view when it has received user input,
must be drawn, or in other situations that may need to be handled such as resizing.

Events are sent to the ``onEvent`` method that takes the matching event type.
The application must handle at least :type:`ConfigureEvent`
and :type:`ExposeEvent` to draw anything,
but there are many other :type:`event types <pugl::EventType>`.

For example, basic event handling for our above class might look something like:

.. code-block:: cpp

   pugl::Status
   MyView::onEvent(const pugl::ConfigureEvent& event) noexcept
   {
     return resize(event.width, event.height);
   }

   pugl::Status
   MyView::onEvent(const pugl::ExposeEvent& event) noexcept
   {
     return drawMyAwesomeInterface(event.x, event.y, event.width, event.height);
   }

*******
Drawing
*******

Note that Pugl uses a different drawing model than many libraries,
particularly those designed for game-style main loops like `SDL <https://libsdl.org/>`_ and `GLFW <https://www.glfw.org/>`_.

In that style of code, drawing is performed imperatively in the main loop,
but with Pugl, the application must draw only while handling an expose event.
This is because Pugl supports event-driven applications that only draw the damaged region when necessary,
and handles exposure internally to provide optimized and consistent behavior across platforms.
