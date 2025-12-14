/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2025 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

/*
 This is a modified version of the SDL_events.h header from SDL3,
 customized for SDL_Event to serve as the event structure for iPlug2's X11 backend.
*/

/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uintptr_t SDL_WindowID;
typedef uint32_t SDL_KeyboardID;
typedef uint32_t SDL_Keycode;
typedef uint16_t SDL_Keymod;
typedef uint32_t SDL_MouseID;

/**
 * Scroll direction types for the Scroll event
 *
 * \since This enum is available since SDL 3.2.0.
 */
typedef enum SDL_MouseWheelDirection
{
    SDL_MOUSEWHEEL_NORMAL,    /**< The scroll direction is normal */
    SDL_MOUSEWHEEL_FLIPPED    /**< The scroll direction is flipped / natural */
} SDL_MouseWheelDirection;

/**
 * A bitmask of pressed mouse buttons, as reported by SDL_GetMouseState, etc.
 *
 * - Button 1: Left mouse button
 * - Button 2: Middle mouse button
 * - Button 3: Right mouse button
 * - Button 4: Side mouse button 1
 * - Button 5: Side mouse button 2
 *
 * \since This datatype is available since SDL 3.2.0.
 *
 * \sa SDL_GetMouseState
 * \sa SDL_GetGlobalMouseState
 * \sa SDL_GetRelativeMouseState
 */
typedef uint32_t SDL_MouseButtonFlags;

#define SDL_BUTTON_LEFT     1
#define SDL_BUTTON_MIDDLE   2
#define SDL_BUTTON_RIGHT    3
#define SDL_BUTTON_X1       4
#define SDL_BUTTON_X2       5

#define SDL_BUTTON_MASK(X)  (1u << ((X)-1))
#define SDL_BUTTON_LMASK    SDL_BUTTON_MASK(SDL_BUTTON_LEFT)
#define SDL_BUTTON_MMASK    SDL_BUTTON_MASK(SDL_BUTTON_MIDDLE)
#define SDL_BUTTON_RMASK    SDL_BUTTON_MASK(SDL_BUTTON_RIGHT)
#define SDL_BUTTON_X1MASK   SDL_BUTTON_MASK(SDL_BUTTON_X1)
#define SDL_BUTTON_X2MASK   SDL_BUTTON_MASK(SDL_BUTTON_X2)

/**
 * Valid key modifiers (possibly OR'd together).
 *
 * \since This datatype is available since SDL 3.2.0.
 */
typedef uint16_t SDL_Keymod;

#define SDL_KMOD_NONE   0x0000u /**< no modifier is applicable. */
#define SDL_KMOD_LSHIFT 0x0001u /**< the left Shift key is down. */
#define SDL_KMOD_RSHIFT 0x0002u /**< the right Shift key is down. */
#define SDL_KMOD_LEVEL5 0x0004u /**< the Level 5 Shift key is down. */
#define SDL_KMOD_LCTRL  0x0040u /**< the left Ctrl (Control) key is down. */
#define SDL_KMOD_RCTRL  0x0080u /**< the right Ctrl (Control) key is down. */
#define SDL_KMOD_LALT   0x0100u /**< the left Alt key is down. */
#define SDL_KMOD_RALT   0x0200u /**< the right Alt key is down. */
#define SDL_KMOD_LGUI   0x0400u /**< the left GUI key (often the Windows key) is down. */
#define SDL_KMOD_RGUI   0x0800u /**< the right GUI key (often the Windows key) is down. */
#define SDL_KMOD_NUM    0x1000u /**< the Num Lock key (may be located on an extended keypad) is down. */
#define SDL_KMOD_CAPS   0x2000u /**< the Caps Lock key is down. */
#define SDL_KMOD_MODE   0x4000u /**< the !AltGr key is down. */
#define SDL_KMOD_SCROLL 0x8000u /**< the Scroll Lock key is down. */
#define SDL_KMOD_CTRL   (SDL_KMOD_LCTRL | SDL_KMOD_RCTRL)   /**< Any Ctrl key is down. */
#define SDL_KMOD_SHIFT  (SDL_KMOD_LSHIFT | SDL_KMOD_RSHIFT) /**< Any Shift key is down. */
#define SDL_KMOD_ALT    (SDL_KMOD_LALT | SDL_KMOD_RALT)     /**< Any Alt key is down. */
#define SDL_KMOD_GUI    (SDL_KMOD_LGUI | SDL_KMOD_RGUI)     /**< Any GUI key is down. */


/**
 * A unique ID for a touch device.
 *
 * This ID is valid for the time the device is connected to the system, and is
 * never reused for the lifetime of the application.
 *
 * The value 0 is an invalid ID.
 *
 * \since This datatype is available since SDL 3.2.0.
 */
typedef uint64_t SDL_TouchID;

/**
 * A unique ID for a single finger on a touch device.
 *
 * This ID is valid for the time the finger (stylus, etc) is touching and will
 * be unique for all fingers currently in contact, so this ID tracks the
 * lifetime of a single continuous touch. This value may represent an index, a
 * pointer, or some other unique ID, depending on the platform.
 *
 * The value 0 is an invalid ID.
 *
 * \since This datatype is available since SDL 3.2.0.
 */
typedef uint64_t SDL_FingerID;

/**
 * An enum that describes the type of a touch device.
 *
 * \since This enum is available since SDL 3.2.0.
 */
typedef enum SDL_TouchDeviceType
{
    SDL_TOUCH_DEVICE_INVALID = -1,
    SDL_TOUCH_DEVICE_DIRECT,            /**< touch screen with window-relative coordinates */
    SDL_TOUCH_DEVICE_INDIRECT_ABSOLUTE, /**< trackpad with absolute device coordinates */
    SDL_TOUCH_DEVICE_INDIRECT_RELATIVE  /**< trackpad with screen cursor-relative coordinates */
} SDL_TouchDeviceType;


/**
 * SDL pen instance IDs.
 *
 * Zero is used to signify an invalid/null device.
 *
 * These show up in pen events when SDL sees input from them. They remain
 * consistent as long as SDL can recognize a tool to be the same pen; but if a
 * pen physically leaves the area and returns, it might get a new ID.
 *
 * \since This datatype is available since SDL 3.2.0.
 */
typedef uint32_t SDL_PenID;

/**
 * The SDL_MouseID for mouse events simulated with pen input.
 *
 * \since This macro is available since SDL 3.2.0.
 */
#define SDL_PEN_MOUSEID ((SDL_MouseID)-2)

/**
 * The SDL_TouchID for touch events simulated with pen input.
 *
 * \since This macro is available since SDL 3.2.0.
 */
#define SDL_PEN_TOUCHID ((SDL_TouchID)-2)


/**
 * Pen input flags, as reported by various pen events' `pen_state` field.
 *
 * \since This datatype is available since SDL 3.2.0.
 */
typedef uint32_t SDL_PenInputFlags;

#define SDL_PEN_INPUT_DOWN       (1u << 0)  /**< pen is pressed down */
#define SDL_PEN_INPUT_BUTTON_1   (1u << 1)  /**< button 1 is pressed */
#define SDL_PEN_INPUT_BUTTON_2   (1u << 2)  /**< button 2 is pressed */
#define SDL_PEN_INPUT_BUTTON_3   (1u << 3)  /**< button 3 is pressed */
#define SDL_PEN_INPUT_BUTTON_4   (1u << 4)  /**< button 4 is pressed */
#define SDL_PEN_INPUT_BUTTON_5   (1u << 5)  /**< button 5 is pressed */
#define SDL_PEN_INPUT_ERASER_TIP (1u << 30) /**< eraser tip is used */

/**
 * Pen axis indices.
 *
 * These are the valid values for the `axis` field in SDL_PenAxisEvent. All
 * axes are either normalised to 0..1 or report a (positive or negative) angle
 * in degrees, with 0.0 representing the centre. Not all pens/backends support
 * all axes: unsupported axes are always zero.
 *
 * To convert angles for tilt and rotation into vector representation, use
 * SDL_sinf on the XTILT, YTILT, or ROTATION component, for example:
 *
 * `SDL_sinf(xtilt * SDL_PI_F / 180.0)`.
 *
 * \since This enum is available since SDL 3.2.0.
 */
typedef enum SDL_PenAxis
{
    SDL_PEN_AXIS_PRESSURE,  /**< Pen pressure.  Unidirectional: 0 to 1.0 */
    SDL_PEN_AXIS_XTILT,     /**< Pen horizontal tilt angle.  Bidirectional: -90.0 to 90.0 (left-to-right). */
    SDL_PEN_AXIS_YTILT,     /**< Pen vertical tilt angle.  Bidirectional: -90.0 to 90.0 (top-to-down). */
    SDL_PEN_AXIS_DISTANCE,  /**< Pen distance to drawing surface.  Unidirectional: 0.0 to 1.0 */
    SDL_PEN_AXIS_ROTATION,  /**< Pen barrel rotation.  Bidirectional: -180 to 179.9 (clockwise, 0 is facing up, -180.0 is facing down). */
    SDL_PEN_AXIS_SLIDER,    /**< Pen finger wheel or slider (e.g., Airbrush Pen).  Unidirectional: 0 to 1.0 */
    SDL_PEN_AXIS_TANGENTIAL_PRESSURE,    /**< Pressure from squeezing the pen ("barrel pressure"). */
    SDL_PEN_AXIS_COUNT       /**< Total known pen axis types in this version of SDL. This number may grow in future releases! */
} SDL_PenAxis;


enum SDL_EventType
{
  /// @brief Unused, do not remove
  SDL_EVENT_NONE = 0,

  /// @brief User-requested quit
  SDL_EVENT_QUIT = 0x100,

  /* Window events */

  /// @brief Window has been shown
  SDL_EVENT_WINDOW_SHOWN = 0x202,
  /// @brief Window has been hidden
  SDL_EVENT_WINDOW_HIDDEN,
  /// @brief Window has been exposed and should be redrawn, and can be redrawn directly from event watchers for this event
  SDL_EVENT_WINDOW_EXPOSED,
  /// @brief Window has been moved to data1, data2
  SDL_EVENT_WINDOW_MOVED,
  /// @brief Window has been resized to data1xdata2
  SDL_EVENT_WINDOW_RESIZED,
  /// @brief The pixel size of the window has changed to data1xdata2
  SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED,
  /// @brief The pixel size of a Metal view associated with the window has changed
  SDL_EVENT_WINDOW_METAL_VIEW_RESIZED,
  /// @brief Window has been minimized
  SDL_EVENT_WINDOW_MINIMIZED,
  /// @brief Window has been maximized
  SDL_EVENT_WINDOW_MAXIMIZED,
  /// @brief Window has been restored to normal size and position
  SDL_EVENT_WINDOW_RESTORED,
  /// @brief Window has gained mouse focus
  SDL_EVENT_WINDOW_MOUSE_ENTER,
  /// @brief Window has lost mouse focus
  SDL_EVENT_WINDOW_MOUSE_LEAVE,
  /// @brief Window has gained keyboard focus
  SDL_EVENT_WINDOW_FOCUS_GAINED,
  /// @brief Window has lost keyboard focus
  SDL_EVENT_WINDOW_FOCUS_LOST,
  /// @brief The window manager requests that the window be closed
  SDL_EVENT_WINDOW_CLOSE_REQUESTED,
  /// @brief Window had a hit test that wasn't SDL_HITTEST_NORMAL
  SDL_EVENT_WINDOW_HIT_TEST,
  /// @brief The ICC profile of the window's display has changed
  SDL_EVENT_WINDOW_ICCPROF_CHANGED,
  /// @brief Window has been moved to display data1
  SDL_EVENT_WINDOW_DISPLAY_CHANGED,
  /// @brief Window display scale has been changed
  SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED,
  /// @brief The window safe area has been changed
  SDL_EVENT_WINDOW_SAFE_AREA_CHANGED,
  /// @brief The window has been occluded
  SDL_EVENT_WINDOW_OCCLUDED,
  /// @brief The window has entered fullscreen mode
  SDL_EVENT_WINDOW_ENTER_FULLSCREEN,
  /// @brief The window has left fullscreen mode
  SDL_EVENT_WINDOW_LEAVE_FULLSCREEN,
  /// @brief The window with the associated ID is being or has been destroyed. If this message is being handled
  /// in an event watcher, the window handle is still valid and can still be used to retrieve any properties
  /// associated with the window. Otherwise, the handle has already been destroyed and all resources
  /// associated with it are invalid
  SDL_EVENT_WINDOW_DESTROYED,
  /// @brief Window HDR properties have changed
  SDL_EVENT_WINDOW_HDR_STATE_CHANGED,
  SDL_EVENT_WINDOW_FIRST = SDL_EVENT_WINDOW_SHOWN,
  SDL_EVENT_WINDOW_LAST = SDL_EVENT_WINDOW_HDR_STATE_CHANGED,

  /* Keyboard events */
  SDL_EVENT_KEY_DOWN        = 0x300, /**< Key pressed */
  SDL_EVENT_KEY_UP,                  /**< Key released */
  SDL_EVENT_TEXT_EDITING,            /**< Keyboard text editing (composition) */
  SDL_EVENT_TEXT_INPUT,              /**< Keyboard text input */
  SDL_EVENT_KEYMAP_CHANGED,          /**< Keymap changed due to a system event such as an
                                          input language or keyboard layout change. */
  SDL_EVENT_KEYBOARD_ADDED,          /**< A new keyboard has been inserted into the system */
  SDL_EVENT_KEYBOARD_REMOVED,        /**< A keyboard has been removed */
  SDL_EVENT_TEXT_EDITING_CANDIDATES, /**< Keyboard text editing candidates */

  /* Mouse events */
  SDL_EVENT_MOUSE_MOTION    = 0x400, /**< Mouse moved */
  SDL_EVENT_MOUSE_BUTTON_DOWN,       /**< Mouse button pressed */
  SDL_EVENT_MOUSE_BUTTON_UP,         /**< Mouse button released */
  SDL_EVENT_MOUSE_WHEEL,             /**< Mouse wheel motion */
  SDL_EVENT_MOUSE_ADDED,             /**< A new mouse has been inserted into the system */
  SDL_EVENT_MOUSE_REMOVED,           /**< A mouse has been removed */

  /* Touch events */
  SDL_EVENT_FINGER_DOWN      = 0x700,
  SDL_EVENT_FINGER_UP,
  SDL_EVENT_FINGER_MOTION,
  SDL_EVENT_FINGER_CANCELED,

  /* 0x800, 0x801, and 0x802 were the Gesture events from SDL2. Do not reuse these values! sdl2-compat needs them! */

  /* Clipboard events */
  SDL_EVENT_CLIPBOARD_UPDATE = 0x900, /**< The clipboard or primary selection changed */

  /* Drag and drop events */
  SDL_EVENT_DROP_FILE        = 0x1000, /**< The system requests a file open */
  SDL_EVENT_DROP_TEXT,                 /**< text/plain drag-and-drop event */
  SDL_EVENT_DROP_BEGIN,                /**< A new set of drops is beginning (NULL filename) */
  SDL_EVENT_DROP_COMPLETE,             /**< Current set of drops is now complete (NULL filename) */
  SDL_EVENT_DROP_POSITION,             /**< Position while moving over the window */

  /* Pressure-sensitive pen events */
  SDL_EVENT_PEN_PROXIMITY_IN = 0x1300,  /**< Pressure-sensitive pen has become available */
  SDL_EVENT_PEN_PROXIMITY_OUT,          /**< Pressure-sensitive pen has become unavailable */
  SDL_EVENT_PEN_DOWN,                   /**< Pressure-sensitive pen touched drawing surface */
  SDL_EVENT_PEN_UP,                     /**< Pressure-sensitive pen stopped touching drawing surface */
  SDL_EVENT_PEN_BUTTON_DOWN,            /**< Pressure-sensitive pen button pressed */
  SDL_EVENT_PEN_BUTTON_UP,              /**< Pressure-sensitive pen button released */
  SDL_EVENT_PEN_MOTION,                 /**< Pressure-sensitive pen is moving on the tablet */
  SDL_EVENT_PEN_AXIS,                   /**< Pressure-sensitive pen angle/pressure/etc changed */

  /* Render events */
  SDL_EVENT_RENDER_TARGETS_RESET = 0x2000, /**< The render targets have been reset and their contents need to be updated */
  SDL_EVENT_RENDER_DEVICE_RESET, /**< The device has been reset and all textures need to be recreated */
  SDL_EVENT_RENDER_DEVICE_LOST, /**< The device has been lost and can't be recovered. */

  /* Reserved events for private platforms */
  SDL_EVENT_PRIVATE0 = 0x4000,
  SDL_EVENT_PRIVATE1,
  SDL_EVENT_PRIVATE2,
  SDL_EVENT_PRIVATE3,

  /* Internal events */
  SDL_EVENT_POLL_SENTINEL = 0x7F00, /**< Signals the end of an event poll cycle */

  /** Events SDL_EVENT_USER through SDL_EVENT_LAST are for your use,
   *  and should be allocated with SDL_RegisterEvents()
   */
  SDL_EVENT_USER    = 0x8000,

  /**
   *  This last event is only for bounding internal arrays
   */
  SDL_EVENT_LAST    = 0xFFFF,

  /* This just makes sure the enum is the size of uint32_t */
  SDL_EVENT_ENUM_PADDING = 0x7FFFFFFF
};

/**
 * Fields shared by every event
 */
typedef struct SDL_CommonEvent
{
    /// @brief Event type, shared with all events, uint32_t to cover user events which are not in the SDL_EventType enumeration
    SDL_EventType type;
    uint32_t reserved;
    /// @brief In nanoseconds, populated either from the system event or using SDL_GetTicksNS()
    uint64_t timestamp;
} SDL_CommonEvent;

/**
 * Display state change event data (event.display.*)
 */
typedef struct SDL_DisplayEvent
{
    /// @brief SDL_DISPLAYEVENT_*
    SDL_EventType type;
    uint32_t reserved;
    /// @brief In nanoseconds, populated either from the system event or using SDL_GetTicksNS()
    uint64_t timestamp;
    uint32_t displayID;  /**< The associated display */
    int32_t data1;       /**< event dependent data */
    int32_t data2;       /**< event dependent data */
} SDL_DisplayEvent;

/**
 * Window state change event data (event.window.*)
 *
 * \remark `type` is SDL_EVENT_WINDOW_*
 * \since This struct is available since SDL 3.2.0.
 */
typedef struct SDL_WindowEvent
{
    /// @brief Event type, shared with all events, uint32_t to cover user events which are not in the SDL_EventType enumeration
    SDL_EventType type;
    uint32_t reserved;
    /// @brief In nanoseconds, populated either from the system event or using SDL_GetTicksNS()
    uint64_t timestamp;
    SDL_WindowID windowID; /**< The associated window */
    int32_t data1;       /**< event dependent data */
    int32_t data2;       /**< event dependent data */
} SDL_WindowEvent;

/**
 * Keyboard device event structure (event.kdevice.*)
 *
 * \remark `type` is SDL_EVENT_KEYBOARD_ADDED or SDL_EVENT_KEYBOARD_REMOVED
 * \since This struct is available since SDL 3.2.0.
 */
typedef struct SDL_KeyboardDeviceEvent
{
    /// @brief Event type, shared with all events, uint32_t to cover user events which are not in the SDL_EventType enumeration
    SDL_EventType type;
    uint32_t reserved;
    /// @brief In nanoseconds, populated either from the system event or using SDL_GetTicksNS()
    uint64_t timestamp;
    SDL_KeyboardID which;   /**< The keyboard instance id */
} SDL_KeyboardDeviceEvent;

/**
 * Keyboard button event structure (event.key.*)
 *
 * The `key` is the base SDL_Keycode generated by pressing the `scancode`
 * using the current keyboard layout, applying any options specified in
 * SDL_HINT_KEYCODE_OPTIONS. You can get the SDL_Keycode corresponding to the
 * event scancode and modifiers directly from the keyboard layout, bypassing
 * SDL_HINT_KEYCODE_OPTIONS, by calling SDL_GetKeyFromScancode().
 *
 * \remark `type` is SDL_EVENT_KEY_DOWN or SDL_EVENT_KEY_UP
 *
 * \sa SDL_GetKeyFromScancode
 * \sa SDL_HINT_KEYCODE_OPTIONS
 */
typedef struct SDL_KeyboardEvent
{
    /// @brief Event type, shared with all events, uint32_t to cover user events which are not in the SDL_EventType enumeration
    SDL_EventType type;
    uint32_t reserved;
    /// @brief In nanoseconds, populated either from the system event or using SDL_GetTicksNS()
    uint64_t timestamp;
    SDL_WindowID windowID;     /**< The window with keyboard focus, if any */
    SDL_KeyboardID which;   /**< The keyboard instance id, or 0 if unknown or virtual */
    uint32_t scancode;      /**< SDL physical key code */
    SDL_Keycode key;        /**< SDL virtual key code */
    SDL_Keymod mod;         /**< current key modifiers */
    uint16_t raw;           /**< The platform dependent scancode for this event */
    bool down;              /**< true if the key is pressed */
    bool repeat;            /**< true if this is a key repeat */
} SDL_KeyboardEvent;

/**
 * Keyboard text editing event structure (event.edit.*)
 *
 * The start cursor is the position, in UTF-8 characters, where new typing
 * will be inserted into the editing text. The length is the number of UTF-8
 * characters that will be replaced by new typing.
 *
 * \remark `type` is SDL_EVENT_TEXT_EDITING
 * \since This struct is available since SDL 3.2.0.
 */
typedef struct SDL_TextEditingEvent
{
    /// @brief Event type, shared with all events, uint32_t to cover user events which are not in the SDL_EventType enumeration
    SDL_EventType type;
    uint32_t reserved;
    /// @brief In nanoseconds, populated either from the system event or using SDL_GetTicksNS()
    uint64_t timestamp;
    SDL_WindowID windowID;      /**< The window with keyboard focus, if any */
    const char *text;        /**< The editing text */
    int32_t start;           /**< The start cursor of selected editing text, or -1 if not set */
    int32_t length;          /**< The length of selected editing text, or -1 if not set */
} SDL_TextEditingEvent;

/**
 * Keyboard IME candidates event structure (event.edit_candidates.*)
 *
 * \remark `type` is SDL_EVENT_TEXT_EDITING_CANDIDATES
 * \since This struct is available since SDL 3.2.0.
 */
typedef struct SDL_TextEditingCandidatesEvent
{
    /// @brief Event type, shared with all events, uint32_t to cover user events which are not in the SDL_EventType enumeration
    SDL_EventType type;
    uint32_t reserved;
    /// @brief In nanoseconds, populated either from the system event or using SDL_GetTicksNS()
    uint64_t timestamp;
    SDL_WindowID windowID;      /**< The window with keyboard focus, if any */
    const char * const *candidates;    /**< The list of candidates, or NULL if there are no candidates available */
    int32_t num_candidates;      /**< The number of strings in `candidates` */
    int32_t selected_candidate;  /**< The index of the selected candidate, or -1 if no candidate is selected */
    bool horizontal;          /**< true if the list is horizontal, false if it's vertical */
    uint8_t padding1;
    uint8_t padding2;
    uint8_t padding3;
} SDL_TextEditingCandidatesEvent;

/**
 * Keyboard text input event structure (event.text.*)
 *
 * This event will never be delivered unless text input is enabled by calling
 * SDL_StartTextInput(). Text input is disabled by default!
 *
 * \since This struct is available since SDL 3.2.0.
 *
 * \sa SDL_StartTextInput
 * \sa SDL_StopTextInput
 */
typedef struct SDL_TextInputEvent
{
    SDL_EventType type; /**< SDL_EVENT_TEXT_INPUT */
    uint32_t reserved;
    uint64_t timestamp;   /**< In nanoseconds, populated using SDL_GetTicksNS() */
    SDL_WindowID windowID; /**< The window with keyboard focus, if any */
    const char *text;   /**< The input text, UTF-8 encoded */
} SDL_TextInputEvent;

/**
 * Mouse motion event structure (event.motion.*)
 *
 * \since This struct is available since SDL 3.2.0.
 */
typedef struct SDL_MouseMotionEvent
{
    SDL_EventType type; /**< SDL_EVENT_MOUSE_MOTION */
    uint32_t reserved;
    uint64_t timestamp;   /**< In nanoseconds, populated using SDL_GetTicksNS() */
    SDL_WindowID windowID; /**< The window with mouse focus, if any */
    SDL_MouseID which;  /**< The mouse instance id in relative mode, SDL_TOUCH_MOUSEID for touch events, or 0 */
    SDL_MouseButtonFlags state;       /**< The current button state */
    float x;            /**< X coordinate, relative to window */
    float y;            /**< Y coordinate, relative to window */
    float xrel;         /**< The relative motion in the X direction */
    float yrel;         /**< The relative motion in the Y direction */
} SDL_MouseMotionEvent;

/**
 * Mouse button event structure (event.button.*)
 *
 * \since This struct is available since SDL 3.2.0.
 */
typedef struct SDL_MouseButtonEvent
{
    SDL_EventType type; /**< SDL_EVENT_MOUSE_BUTTON_DOWN or SDL_EVENT_MOUSE_BUTTON_UP */
    uint32_t reserved;
    uint64_t timestamp;   /**< In nanoseconds, populated using SDL_GetTicksNS() */
    SDL_WindowID windowID; /**< The window with mouse focus, if any */
    SDL_MouseID which;  /**< The mouse instance id in relative mode, SDL_TOUCH_MOUSEID for touch events, or 0 */
    uint8_t button;       /**< The mouse button index */
    bool down;          /**< true if the button is pressed */
    uint8_t clicks;       /**< 1 for single-click, 2 for double-click, etc. */
    uint8_t padding;
    float x;            /**< X coordinate, relative to window */
    float y;            /**< Y coordinate, relative to window */
} SDL_MouseButtonEvent;

/**
 * Mouse wheel event structure (event.wheel.*)
 *
 * \since This struct is available since SDL 3.2.0.
 */
typedef struct SDL_MouseWheelEvent
{
    SDL_EventType type; /**< SDL_EVENT_MOUSE_WHEEL */
    uint32_t reserved;
    uint64_t timestamp;   /**< In nanoseconds, populated using SDL_GetTicksNS() */
    SDL_WindowID windowID; /**< The window with mouse focus, if any */
    SDL_MouseID which;  /**< The mouse instance id in relative mode or 0 */
    float x;            /**< The amount scrolled horizontally, positive to the right and negative to the left */
    float y;            /**< The amount scrolled vertically, positive away from the user and negative toward the user */
    SDL_MouseWheelDirection direction; /**< Set to one of the SDL_MOUSEWHEEL_* defines. When FLIPPED the values in X and Y will be opposite. Multiply by -1 to change them back */
    float mouse_x;      /**< X coordinate, relative to window */
    float mouse_y;      /**< Y coordinate, relative to window */
} SDL_MouseWheelEvent;

/**
 * Renderer event structure (event.render.*)
 *
 * \since This struct is available since SDL 3.2.0.
 */
typedef struct SDL_RenderEvent
{
    SDL_EventType type; /**< SDL_EVENT_RENDER_TARGETS_RESET, SDL_EVENT_RENDER_DEVICE_RESET, SDL_EVENT_RENDER_DEVICE_LOST */
    uint32_t reserved;
    uint64_t timestamp;   /**< In nanoseconds, populated using SDL_GetTicksNS() */
    SDL_WindowID windowID; /**< The window containing the renderer in question. */
} SDL_RenderEvent;


/**
 * Touch finger event structure (event.tfinger.*)
 *
 * Coordinates in this event are normalized. `x` and `y` are normalized to a
 * range between 0.0f and 1.0f, relative to the window, so (0,0) is the top
 * left and (1,1) is the bottom right. Delta coordinates `dx` and `dy` are
 * normalized in the ranges of -1.0f (traversed all the way from the bottom or
 * right to all the way up or left) to 1.0f (traversed all the way from the
 * top or left to all the way down or right).
 *
 * Note that while the coordinates are _normalized_, they are not _clamped_,
 * which means in some circumstances you can get a value outside of this
 * range. For example, a renderer using logical presentation might give a
 * negative value when the touch is in the letterboxing. Some platforms might
 * report a touch outside of the window, which will also be outside of the
 * range.
 *
 * \since This struct is available since SDL 3.2.0.
 */
typedef struct SDL_TouchFingerEvent
{
    SDL_EventType type; /**< SDL_EVENT_FINGER_DOWN, SDL_EVENT_FINGER_UP, SDL_EVENT_FINGER_MOTION, or SDL_EVENT_FINGER_CANCELED */
    uint32_t reserved;
    uint64_t timestamp;   /**< In nanoseconds, populated using SDL_GetTicksNS() */
    SDL_TouchID touchID; /**< The touch device id */
    SDL_FingerID fingerID;
    float x;            /**< Normalized in the range 0...1 */
    float y;            /**< Normalized in the range 0...1 */
    float dx;           /**< Normalized in the range -1...1 */
    float dy;           /**< Normalized in the range -1...1 */
    float pressure;     /**< Normalized in the range 0...1 */
    SDL_WindowID windowID; /**< The window underneath the finger, if any */
} SDL_TouchFingerEvent;

/**
 * Pressure-sensitive pen proximity event structure (event.pmotion.*)
 *
 * When a pen becomes visible to the system (it is close enough to a tablet,
 * etc), SDL will send an SDL_EVENT_PEN_PROXIMITY_IN event with the new pen's
 * ID. This ID is valid until the pen leaves proximity again (has been removed
 * from the tablet's area, the tablet has been unplugged, etc). If the same
 * pen reenters proximity again, it will be given a new ID.
 *
 * Note that "proximity" means "close enough for the tablet to know the tool
 * is there." The pen touching and lifting off from the tablet while not
 * leaving the area are handled by SDL_EVENT_PEN_DOWN and SDL_EVENT_PEN_UP.
 *
 * \since This struct is available since SDL 3.2.0.
 */
typedef struct SDL_PenProximityEvent
{
    SDL_EventType type; /**< SDL_EVENT_PEN_PROXIMITY_IN or SDL_EVENT_PEN_PROXIMITY_OUT */
    uint32_t reserved;
    uint64_t timestamp;   /**< In nanoseconds, populated using SDL_GetTicksNS() */
    SDL_WindowID windowID; /**< The window with pen focus, if any */
    SDL_PenID which;        /**< The pen instance id */
} SDL_PenProximityEvent;

/**
 * Pressure-sensitive pen motion event structure (event.pmotion.*)
 *
 * Depending on the hardware, you may get motion events when the pen is not
 * touching a tablet, for tracking a pen even when it isn't drawing. You
 * should listen for SDL_EVENT_PEN_DOWN and SDL_EVENT_PEN_UP events, or check
 * `pen_state & SDL_PEN_INPUT_DOWN` to decide if a pen is "drawing" when
 * dealing with pen motion.
 *
 * \since This struct is available since SDL 3.2.0.
 */
typedef struct SDL_PenMotionEvent
{
    SDL_EventType type; /**< SDL_EVENT_PEN_MOTION */
    uint32_t reserved;
    uint64_t timestamp;   /**< In nanoseconds, populated using SDL_GetTicksNS() */
    SDL_WindowID windowID; /**< The window with pen focus, if any */
    SDL_PenID which;        /**< The pen instance id */
    SDL_PenInputFlags pen_state;   /**< Complete pen input state at time of event */
    float x;                /**< X coordinate, relative to window */
    float y;                /**< Y coordinate, relative to window */
} SDL_PenMotionEvent;

/**
 * Pressure-sensitive pen touched event structure (event.ptouch.*)
 *
 * These events come when a pen touches a surface (a tablet, etc), or lifts
 * off from one.
 *
 * \since This struct is available since SDL 3.2.0.
 */
typedef struct SDL_PenTouchEvent
{
    SDL_EventType type;     /**< SDL_EVENT_PEN_DOWN or SDL_EVENT_PEN_UP */
    uint32_t reserved;
    uint64_t timestamp;       /**< In nanoseconds, populated using SDL_GetTicksNS() */
    SDL_WindowID windowID;  /**< The window with pen focus, if any */
    SDL_PenID which;        /**< The pen instance id */
    SDL_PenInputFlags pen_state;   /**< Complete pen input state at time of event */
    float x;                /**< X coordinate, relative to window */
    float y;                /**< Y coordinate, relative to window */
    bool eraser;        /**< true if eraser end is used (not all pens support this). */
    bool down;          /**< true if the pen is touching or false if the pen is lifted off */
} SDL_PenTouchEvent;

/**
 * Pressure-sensitive pen button event structure (event.pbutton.*)
 *
 * This is for buttons on the pen itself that the user might click. The pen
 * itself pressing down to draw triggers a SDL_EVENT_PEN_DOWN event instead.
 *
 * \since This struct is available since SDL 3.2.0.
 */
typedef struct SDL_PenButtonEvent
{
    SDL_EventType type; /**< SDL_EVENT_PEN_BUTTON_DOWN or SDL_EVENT_PEN_BUTTON_UP */
    uint32_t reserved;
    uint64_t timestamp;   /**< In nanoseconds, populated using SDL_GetTicksNS() */
    SDL_WindowID windowID; /**< The window with mouse focus, if any */
    SDL_PenID which;        /**< The pen instance id */
    SDL_PenInputFlags pen_state;   /**< Complete pen input state at time of event */
    float x;                /**< X coordinate, relative to window */
    float y;                /**< Y coordinate, relative to window */
    uint8_t button;       /**< The pen button index (first button is 1). */
    bool down;      /**< true if the button is pressed */
} SDL_PenButtonEvent;

/**
 * Pressure-sensitive pen pressure / angle event structure (event.paxis.*)
 *
 * You might get some of these events even if the pen isn't touching the
 * tablet.
 *
 * \since This struct is available since SDL 3.2.0.
 */
typedef struct SDL_PenAxisEvent
{
    SDL_EventType type;     /**< SDL_EVENT_PEN_AXIS */
    uint32_t reserved;
    uint64_t timestamp;       /**< In nanoseconds, populated using SDL_GetTicksNS() */
    SDL_WindowID windowID;  /**< The window with pen focus, if any */
    SDL_PenID which;        /**< The pen instance id */
    SDL_PenInputFlags pen_state;   /**< Complete pen input state at time of event */
    float x;                /**< X coordinate, relative to window */
    float y;                /**< Y coordinate, relative to window */
    SDL_PenAxis axis;       /**< Axis that has changed */
    float value;            /**< New value of axis */
} SDL_PenAxisEvent;

/**
 * An event used to drop text or request a file open by the system
 * (event.drop.*)
 *
 * \since This struct is available since SDL 3.2.0.
 */
typedef struct SDL_DropEvent
{
    SDL_EventType type; /**< SDL_EVENT_DROP_BEGIN or SDL_EVENT_DROP_FILE or SDL_EVENT_DROP_TEXT or SDL_EVENT_DROP_COMPLETE or SDL_EVENT_DROP_POSITION */
    uint32_t reserved;
    uint64_t timestamp;   /**< In nanoseconds, populated using SDL_GetTicksNS() */
    SDL_WindowID windowID;    /**< The window that was dropped on, if any */
    float x;            /**< X coordinate, relative to window (not on begin) */
    float y;            /**< Y coordinate, relative to window (not on begin) */
    const char *source; /**< The source app that sent this drop event, or NULL if that isn't available */
    const char *data;   /**< The text for SDL_EVENT_DROP_TEXT and the file name for SDL_EVENT_DROP_FILE, NULL for other events */
} SDL_DropEvent;

/**
 * An event triggered when the clipboard contents have changed
 * (event.clipboard.*)
 *
 * \since This struct is available since SDL 3.2.0.
 */
typedef struct SDL_ClipboardEvent
{
    SDL_EventType type; /**< SDL_EVENT_CLIPBOARD_UPDATE */
    uint32_t reserved;
    uint64_t timestamp;   /**< In nanoseconds, populated using SDL_GetTicksNS() */
    bool owner;         /**< are we owning the clipboard (internal update) */
    int32_t num_mime_types;   /**< number of mime types */
    const char **mime_types; /**< current mime types */
} SDL_ClipboardEvent;

/**
 * The "quit requested" event
 *
 * \since This struct is available since SDL 3.2.0.
 */
typedef struct SDL_QuitEvent
{
    SDL_EventType type; /**< SDL_EVENT_QUIT */
    uint32_t reserved;
    uint64_t timestamp;   /**< In nanoseconds, populated using SDL_GetTicksNS() */
} SDL_QuitEvent;

/**
 * A user-defined event type (event.user.*)
 *
 * This event is unique; it is never created by SDL, but only by the
 * application. The event can be pushed onto the event queue using
 * SDL_PushEvent(). The contents of the structure members are completely up to
 * the programmer; the only requirement is that '''type''' is a value obtained
 * from SDL_RegisterEvents().
 *
 * \since This struct is available since SDL 3.2.0.
 */
typedef struct SDL_UserEvent
{
    uint32_t type;        /**< SDL_EVENT_USER through SDL_EVENT_LAST-1, uint32_t because these are not in the SDL_EventType enumeration */
    uint32_t reserved;
    uint64_t timestamp;   /**< In nanoseconds, populated using SDL_GetTicksNS() */
    SDL_WindowID windowID; /**< The associated window if any */
    int32_t code;        /**< User defined event code */
    void *data1;        /**< User defined data pointer */
    void *data2;        /**< User defined data pointer */
} SDL_UserEvent;


/**
 * The structure for all events in SDL.
 *
 * The SDL_Event structure is the core of all event handling in SDL. SDL_Event
 * is a union of all event structures used in SDL.
 *
 * \since This struct is available since SDL 3.2.0.
 */
typedef union SDL_Event
{
    uint32_t type;                          /**< Event type, shared with all events, uint32_t to cover user events which are not in the SDL_EventType enumeration */
    SDL_CommonEvent common;                 /**< Common event data */
    SDL_DisplayEvent display;               /**< Display event data */
    SDL_WindowEvent window;                 /**< Window event data */
    SDL_KeyboardDeviceEvent kdevice;        /**< Keyboard device change event data */
    SDL_KeyboardEvent key;                  /**< Keyboard event data */
    SDL_TextEditingEvent edit;              /**< Text editing event data */
    SDL_TextEditingCandidatesEvent edit_candidates; /**< Text editing candidates event data */
    SDL_TextInputEvent text;                /**< Text input event data */
    SDL_MouseMotionEvent motion;            /**< Mouse motion event data */
    SDL_MouseButtonEvent button;            /**< Mouse button event data */
    SDL_MouseWheelEvent wheel;              /**< Mouse wheel event data */
    SDL_QuitEvent quit;                     /**< Quit request event data */
    SDL_UserEvent user;                     /**< Custom event data */
    SDL_TouchFingerEvent tfinger;           /**< Touch finger event data */
    SDL_PenProximityEvent pproximity;       /**< Pen proximity event data */
    SDL_PenTouchEvent ptouch;               /**< Pen tip touching event data */
    SDL_PenMotionEvent pmotion;             /**< Pen motion event data */
    SDL_PenButtonEvent pbutton;             /**< Pen button event data */
    SDL_PenAxisEvent paxis;                 /**< Pen axis event data */
    SDL_RenderEvent render;                 /**< Render event data */
    SDL_DropEvent drop;                     /**< Drag and drop event data */
    SDL_ClipboardEvent clipboard;           /**< Clipboard event data */

    /* This is necessary for ABI compatibility between Visual C++ and GCC.
       Visual C++ will respect the push pack pragma and use 52 bytes (size of
       SDL_TextEditingEvent, the largest structure for 32-bit and 64-bit
       architectures) for this union, and GCC will use the alignment of the
       largest datatype within the union, which is 8 bytes on 64-bit
       architectures.

       So... we'll add padding to force the size to be the same for both.

       On architectures where pointers are 16 bytes, this needs rounding up to
       the next multiple of 16, 64, and on architectures where pointers are
       even larger the size of SDL_UserEvent will dominate as being 3 pointers.
    */
    uint8_t padding[128];
} SDL_Event;

/* Make sure we haven't broken binary compatibility */
static_assert(sizeof(SDL_Event) == sizeof(SDL_Event::padding));

#ifdef __cplusplus
}
#endif
