.. default-domain:: c
.. highlight:: c

######################
Driving the Event Loop
######################

Pugl does not contain any threads or other event loop "magic".
For flexibility, the event loop is driven explicitly by repeatedly calling :func:`puglUpdate`,
which processes events from the window system and dispatches them to views when necessary.

The exact use of :func:`puglUpdate` depends on the application.
Plugins should call it with a ``timeout`` of 0 in a callback driven by the host.
This avoids blocking the main loop,
since other plugins and the host itself need to run as well.

A program can use whatever timeout is appropriate:
event-driven applications may wait forever by using a ``timeout`` of -1,
while those that draw continuously may use a significant fraction of the frame period
(with enough time left over to render).

*********
Redrawing
*********

Occasional redrawing can be requested by calling :func:`puglPostRedisplay` or :func:`puglPostRedisplayRect`.
After these are called,
a :struct:`PuglExposeEvent` will be dispatched on the next call to :func:`puglUpdate`.

For continuous redrawing,
call :func:`puglPostRedisplay` while handling a :struct:`PuglUpdateEvent` event.
This event is sent just before views are redrawn,
so it can be used as a hook to expand the update region right before the view is exposed.
Anything else that needs to be done every frame can be handled similarly.

*****************
Event Dispatching
*****************

Ideally, pending events are dispatched during a call to :func:`puglUpdate`,
directly within the scope of that call.

Unfortunately, this is not universally true due to differences between platforms.

MacOS
=====

On MacOS, drawing is handled specially and not by the normal event queue mechanism.
This means that configure and expose events,
and possibly others,
may be dispatched to a view outside the scope of a :func:`puglUpdate` call.
In general, you can not rely on coherent event dispatching semantics on MacOS:
the operating system can call into application code at "random" times,
and these calls may result in Pugl events being dispatched.

An application that follows the Pugl guidelines should work fine,
but there is one significant inconsistency you may encounter on MacOS:
posting a redisplay will not wake up a blocked :func:`puglUpdate` call.

Windows
=======

On Windows, the application has relatively tight control over the event loop,
so events are typically dispatched explicitly by :func:`puglUpdate`.
Drawing is handled by events,
so posting a redisplay will wake up a blocked :func:`puglUpdate` call.

However, it is possible for the system to dispatch events at other times.
So,
it is possible for events to be dispatched outside the scope of a :func:`puglUpdate` call,
but this does not happen in normal circumstances and can largely be ignored.

X11
===

On X11, the application strictly controls event dispatching,
and there is no way for the system to call into application code at surprising times.
So, all events are dispatched in the scope of a :func:`puglUpdate` call.

*********************
Recursive Event Loops
*********************

On Windows and MacOS,
the event loop is stalled while the user is resizing the window or,
on Windows,
has displayed the window menu.
This means that :func:`puglUpdate` will block until the resize is finished,
or the menu is closed.

Pugl dispatches :struct:`PuglLoopEnterEvent` and :struct:`PuglLoopLeaveEvent` events to notify the application of this situation.
If you want to continuously redraw during resizing on these platforms,
you can schedule a timer with :func:`puglStartTimer` when the recursive loop is entered,
and post redisplays when handling the :struct:`PuglTimerEvent`.
Be sure to remove the timer with :func:`puglStopTimer` when the recursive loop is finished.

On X11, there are no recursive event loops,
and everything works as usual while the user is resizing the window.
There is nothing special about a "live resize" on X11,
and the above loop events will never be dispatched.

