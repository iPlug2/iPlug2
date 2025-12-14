/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#include "IPlugPlatform.h"
#include <cstdint>
#include <heapbuf.h>
#include "SDL_events.hpp"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

class PlatformX11;

/// @brief A window-rectangle, representing screen bounds or redraw-areas.
struct WRect
{
  /// @brief left bound
  int32_t x;
  /// @brief top bound
  int32_t y;
  /// @brief width
  uint32_t w;
  /// @brief height
  uint32_t h;
};

/// @brief Indicates how to position the cursor when calling \c X11Window::MoveMouse
enum EMouseMoveMode {
  /// @brief The coordinates are inside the window
  MOUSE_MOVE_WINDOW,
  /// @brief The coordinates are relative to the screen
  MOUSE_MOVE_SCREEN,
  /// @brief The coordinates are an offset to the current cursor position
  MOUSE_MOVE_RELATIVE,
};

enum PixelFormat {
  /// @brief Pixels are RGB with 8 bits per channel (3 bpp)
  kPF_RGB8 = 1,
  /// @brief Pixels are RGBA with 8 bits per channel (4 bpp)
  kPF_RGBA8 = 2,
};

/// @brief Lists the supported clipboard data formats.
enum EClipboardFormat {
  CLIPBOARD_FORMAT_UNKNOWN = 0,
  CLIPBOARD_FORMAT_UTF8,
};

/// @brief The configuration options available when creating a new window.
struct WindowOptions
{
  /// @brief Bitmask options for \c flags
  enum EFlags {
    /// @brief Create a window with a GLES context instead of the default OpenGL glx
    USE_GLES = 1<<0,
    /// @brief Enable debug mode for the opengl context
    GL_DEBUG = 1<<1,
  };

  /// @brief The parent window ID, 0 means no parent
  void* parent = 0;
  /// @brief The major OpenGL / GLES version, 0 means no GL context
  uint16_t glMajor = 0;
  /// @brief The minor OpenGL / GLES version
  uint16_t glMinor = 0;
  /// @brief Any OpenGL / GLES context creation flags
  uint32_t glFlags = 0;
  /// @brief The desired bounds of the window
  WRect bounds = WRect{0, 0, 0, 0};
  /// @brief Other flags
  uint32_t flags = 0;
};

class X11Window
{
protected:
  /// @brief Pointer to the platform object
  PlatformX11* mPlatform;
  // Remaining implementation fields hidden to simplify the public API.
  // ONLY use this object as a pointer, do NOT try to copy it directly.

  X11Window();
  ~X11Window();

public:
  void Close();

  bool DrawBegin();
  void DrawEnd();

  bool DrawImage(const WRect& bounds, int format, const uint8_t* data);

  /// @brief Set the window as visible or not visible.
  /// @param show true to make this window visible, false to hide it
  void SetVisible(bool show);

  /// @brief Check if the window is currently visible or not.
  /// @return true if visible, false if not
  bool IsVisible() const;

  /**
   * Set the window's title.
   */
  void SetTitle(const char* title);

  void Resize(uint32_t w, uint32_t h);
  void Move(int32_t x, int32_t y);
  void RequestFocus();

  void SetCursorVisible(bool visible);
  bool IsCursorVisible() const;

  void SetCursorGrabbed(bool grab);
  bool IsCursorGrabbed() const;

  void MoveMouse(EMouseMoveMode mode, int x, int y);

  /// @brief Get the platform's native window handle.
  void* GetHandle() const;

  /// @brief Take the next event from the list of queued events for
  /// this window, placing it into \c event .
  /// @param event destination for event data
  /// @return true if there was an event, false if not
  bool PollEvent(SDL_Event* event);
};

class PlatformX11
{
private:
  // implementation fields hidden to simplify public API.
  // This struct MUST NOT be copied, it's a pointer-only object.
  char hidden[sizeof(void*)];

  PlatformX11();
  PlatformX11(const PlatformX11&) = delete;
  PlatformX11(PlatformX11&&) = delete;

public:
  /// @brief Create an instance of the platform object.
  static PlatformX11* Create();

  ~PlatformX11();

  /// @brief Create a window using the provided options.
  /// @param options
  /// @return The newly created window, or \c NULL on failure
  X11Window* CreateWindow(const WindowOptions& options);

  /// @brief Set the clipboard contents
  /// @param format the format of the data
  /// @param data the clipboard data
  /// @param data_length length of \c data
  /// @return true if setting the clipboard succeeded, false if it failed
  bool SetClipboard(EClipboardFormat format, const void* data, uint32_t data_length);

  /// @brief Get the clipboard format and, optionally, the contents.
  /// @details
  /// If \c *pFormat is \c CLIPBOARD_FORMAT_UNKNOWN then it will be set
  /// to the correct format, otherwise it acts as a request for that format,
  /// and thus \c pData will only be set if it's not \c NULL and if the
  /// data in the clipboard matches the requested format.
  /// @param pFormat the clipboard format; used to either obtain the current
  /// format, or request a specific format
  /// @param pData the clipboard data; may be \c NULL
  /// @return true if there was clipboard data, false if not
  bool GetClipboard(EClipboardFormat* pFormat, WDL_TypedBuf<uint8_t>* pData);

  /// @brief Request that the platform commit any pending changes.
  void Flush();

  /// @brief Process platform-specific events and update all windows
  /// attached to this platform. This is the "main loop" function
  /// and should be called frequently.
  void ProcessEvents();
};


END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
