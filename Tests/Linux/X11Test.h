#pragma once

#include <X11/Xlib.h>
#include <X11/Xutil.h>

inline void SendNativeClose(Display* display, Window window)
{
  XEvent event {};
  event.xclient.type = ClientMessage;
  event.xclient.window = window;
  event.xclient.message_type = XInternAtom(display, "WM_PROTOCOLS", False);
  event.xclient.format = 32;
  event.xclient.data.l[0] = XInternAtom(display, "WM_DELETE_WINDOW", False);
  XSendEvent(display, window, False, 0, &event);
  XSync(display, False);
}

inline Window FindEditor(Display* display, Window parent)
{
  Window root, owner, *children = nullptr;
  unsigned count = 0;
  if (!XQueryTree(display, parent, &root, &owner, &children, &count))
    return 0;
  Window result = 0;
  for (unsigned i = 0; i < count; ++i)
  {
    XWindowAttributes attributes{};
    if (XGetWindowAttributes(display, children[i], &attributes) &&
        attributes.width >= 500 && attributes.height >= 500)
    {
      const Window nested = FindEditor(display, children[i]);
      result = nested ? nested : children[i];
      break;
    }
  }
  if (children)
    XFree(children);
  return result;
}

inline bool IsRendered(Display* display, Window window)
{
  XWindowAttributes attributes{};
  if (!XGetWindowAttributes(display, window, &attributes) || attributes.map_state != IsViewable)
    return false;
  XImage* image = XGetImage(display, window, 0, 0, attributes.width, attributes.height,
                            AllPlanes, ZPixmap);
  if (!image)
    return false;
  const unsigned long background = XGetPixel(image, 8, 8);
  unsigned different = 0;
  for (int y = 8; y < attributes.height; y += 8)
    for (int x = 8; x < attributes.width; x += 8)
      different += XGetPixel(image, x, y) != background;
  XDestroyImage(image);
  return different >= 20;
}
