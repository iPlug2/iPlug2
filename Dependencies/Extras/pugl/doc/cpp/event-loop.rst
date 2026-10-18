.. default-domain:: cpp
.. highlight:: cpp
.. namespace:: pugl

######################
Driving the Event Loop
######################

Pugl does not contain any threads or other event loop "magic".
For flexibility, the event loop is driven manually by repeatedly calling :func:`World::update`,
which processes events from the window system and dispatches them to views when necessary.

The exact use of :func:`World::update` depends on the application.
Plugins typically call it with a ``timeout`` of 0 in a callback driven by the host.
This avoids blocking the main loop,
since other plugins and the host itself need to run as well.

A program can use whatever timeout is appropriate:
event-driven applications may wait forever by using a ``timeout`` of -1,
while those that draw continuously may use a significant fraction of the frame period
(with enough time left over to render).

*********
Redrawing
*********

Occasional redrawing can be requested by calling :func:`View::postRedisplay` or :func:`View::postRedisplayRect`.
After these are called,
a :type:`ExposeEvent` will be dispatched on the next call to :func:`World::update`.
Note, however, that this will not wake up a blocked :func:`World::update` call on MacOS
(which does not handle drawing via events).

For continuous redrawing,
call :func:`View::postRedisplay` while handling a :type:`UpdateEvent`.
This event is sent just before views are redrawn,
so it can be used as a hook to expand the update region right before the view is exposed.
Anything else that needs to be done every frame can be handled similarly.
