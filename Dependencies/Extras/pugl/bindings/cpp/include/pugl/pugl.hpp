// Copyright 2012-2023 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef PUGL_PUGL_HPP
#define PUGL_PUGL_HPP

#include "pugl/pugl.h"

#include <cstddef> // IWYU pragma: keep
#include <cstdint> // IWYU pragma: keep

#if defined(PUGL_HPP_THROW_FAILED_CONSTRUCTION)
#  include <exception>
#elif defined(PUGL_HPP_ASSERT_CONSTRUCTION)
#  include <cassert>
#endif

namespace pugl {

/**
   @defgroup puglpp Pugl C++ API
   Pugl C++ API wrapper.
   @{
*/

namespace detail {

/// Free function for a C object
template<typename T>
using FreeFunc = void (*)(T*);

/// Generic C++ wrapper for a C object
template<class T, FreeFunc<T> Free>
class Wrapper
{
public:
  Wrapper(const Wrapper&)            = delete;
  Wrapper& operator=(const Wrapper&) = delete;

  Wrapper(Wrapper&& wrapper) noexcept
    : _ptr{wrapper._ptr}
  {
    wrapper._ptr = nullptr;
  }

  Wrapper& operator=(Wrapper&& wrapper) noexcept
  {
    _ptr         = wrapper._ptr;
    wrapper._ptr = nullptr;
    return *this;
  }

  ~Wrapper() noexcept { Free(_ptr); }

  T*       cobj() noexcept { return _ptr; }
  const T* cobj() const noexcept { return _ptr; }

protected:
  explicit Wrapper(T* ptr) noexcept
    : _ptr{ptr}
  {}

private:
  T* _ptr;
};

} // namespace detail

/// @copydoc PuglRect
using Rect = PuglRect;

/// @copydoc PuglStringHint
enum class StringHint {
  className = 1, ///< @copydoc PUGL_CLASS_NAME
  windowTitle,   ///< @copydoc PUGL_WINDOW_TITLE
};

static_assert(static_cast<StringHint>(PUGL_WINDOW_TITLE) ==
                StringHint::windowTitle,
              "");

/**
   @defgroup puglpp_events Events
   @{
*/

/**
   A strongly-typed analogue of PuglEvent.

   This is bit-for-bit identical to the corresponding PuglEvent.

   @tparam t The `type` field of the corresponding PuglEvent.

   @tparam Base The specific struct type of the corresponding PuglEvent.
*/
template<PuglEventType t, class Base>
struct Event final : Base {
  /// The type of the corresponding C event structure
  using BaseEvent = Base;

  /// The `type` field of the corresponding C event structure
  static constexpr const PuglEventType type = t;

  explicit Event(const Base& base)
    : Base{base}
  {}

  template<class... Args>
  explicit Event(const PuglEventFlags f, Args... args)
    : Base{t, f, args...}
  {}
};

/// @copydoc PuglMod
using Mod = PuglMod;

/// @copydoc PuglMods
using Mods = PuglMods;

/// @copydoc PuglKey
using Key = PuglKey;

/// @copydoc PuglEventType
using EventType = PuglEventType;

/// @copydoc PuglEventFlag
using EventFlag = PuglEventFlag;

/// @copydoc PuglEventFlags
using EventFlags = PuglEventFlags;

/// @copydoc PuglCrossingMode
using CrossingMode = PuglCrossingMode;

/// @copydoc PuglViewStyleFlag
using ViewStyleFlag = PuglViewStyleFlag;

/// @copydoc PuglViewStyleFlags
using ViewStyleFlags = PuglViewStyleFlags;

/// @copydoc PuglRealizeEvent
using RealizeEvent = Event<PUGL_REALIZE, PuglRealizeEvent>;

/// @copydoc PuglUnrealizeEvent
using UnrealizeEvent = Event<PUGL_UNREALIZE, PuglUnrealizeEvent>;

/// @copydoc PuglConfigureEvent
using ConfigureEvent = Event<PUGL_CONFIGURE, PuglConfigureEvent>;

/// @copydoc PuglUpdateEvent
using UpdateEvent = Event<PUGL_UPDATE, PuglUpdateEvent>;

/// @copydoc PuglExposeEvent
using ExposeEvent = Event<PUGL_EXPOSE, PuglExposeEvent>;

/// @copydoc PuglCloseEvent
using CloseEvent = Event<PUGL_CLOSE, PuglCloseEvent>;

/// @copydoc PuglFocusEvent
using FocusInEvent = Event<PUGL_FOCUS_IN, PuglFocusEvent>;

/// @copydoc PuglFocusEvent
using FocusOutEvent = Event<PUGL_FOCUS_OUT, PuglFocusEvent>;

/// @copydoc PuglKeyEvent
using KeyPressEvent = Event<PUGL_KEY_PRESS, PuglKeyEvent>;

/// @copydoc PuglKeyEvent
using KeyReleaseEvent = Event<PUGL_KEY_RELEASE, PuglKeyEvent>;

/// @copydoc PuglTextEvent
using TextEvent = Event<PUGL_TEXT, PuglTextEvent>;

/// @copydoc PuglCrossingEvent
using PointerInEvent = Event<PUGL_POINTER_IN, PuglCrossingEvent>;

/// @copydoc PuglCrossingEvent
using PointerOutEvent = Event<PUGL_POINTER_OUT, PuglCrossingEvent>;

/// @copydoc PuglButtonEvent
using ButtonPressEvent = Event<PUGL_BUTTON_PRESS, PuglButtonEvent>;

/// @copydoc PuglButtonEvent
using ButtonReleaseEvent = Event<PUGL_BUTTON_RELEASE, PuglButtonEvent>;

/// @copydoc PuglMotionEvent
using MotionEvent = Event<PUGL_MOTION, PuglMotionEvent>;

/// @copydoc PuglScrollEvent
using ScrollEvent = Event<PUGL_SCROLL, PuglScrollEvent>;

/// @copydoc PuglClientEvent
using ClientEvent = Event<PUGL_CLIENT, PuglClientEvent>;

/// @copydoc PuglTimerEvent
using TimerEvent = Event<PUGL_TIMER, PuglTimerEvent>;

/// @copydoc PuglLoopEnterEvent
using LoopEnterEvent = Event<PUGL_LOOP_ENTER, PuglLoopEnterEvent>;

/// @copydoc PuglLoopLeaveEvent
using LoopLeaveEvent = Event<PUGL_LOOP_LEAVE, PuglLoopLeaveEvent>;

/// @copydoc PuglDataOfferEvent
using DataOfferEvent = Event<PUGL_DATA_OFFER, PuglDataOfferEvent>;

/// @copydoc PuglDataEvent
using DataEvent = Event<PUGL_DATA, PuglDataEvent>;

/**
   @}
   @defgroup puglpp_status Status
   @{
*/

/// @copydoc PuglStatus
enum class Status {
  success,             ///< @copydoc PUGL_SUCCESS
  failure,             ///< @copydoc PUGL_FAILURE
  unknownError,        ///< @copydoc PUGL_UNKNOWN_ERROR
  badBackend,          ///< @copydoc PUGL_BAD_BACKEND
  badConfiguration,    ///< @copydoc PUGL_BAD_CONFIGURATION
  badParameter,        ///< @copydoc PUGL_BAD_PARAMETER
  backendFailed,       ///< @copydoc PUGL_BACKEND_FAILED
  registrationFailed,  ///< @copydoc PUGL_REGISTRATION_FAILED
  realizeFailed,       ///< @copydoc PUGL_REALIZE_FAILED
  setFormatFailed,     ///< @copydoc PUGL_SET_FORMAT_FAILED
  createContextFailed, ///< @copydoc PUGL_CREATE_CONTEXT_FAILED
  unsupported,         ///< @copydoc PUGL_UNSUPPORTED
};

static_assert(static_cast<Status>(PUGL_UNSUPPORTED) == Status::unsupported, "");

/// @copydoc puglStrerror
inline const char*
strerror(const Status status) noexcept
{
  return puglStrerror(static_cast<PuglStatus>(status));
}

/**
   @}
   @defgroup puglpp_world World
   @{
*/

/// @copydoc PuglWorldType
enum class WorldType {
  program, ///< @copydoc PUGL_PROGRAM
  module,  ///< @copydoc PUGL_MODULE
};

static_assert(static_cast<WorldType>(PUGL_MODULE) == WorldType::module, "");

/// @copydoc PuglWorldFlag
enum class WorldFlag {
  threads = PUGL_WORLD_THREADS, ///< @copydoc PUGL_WORLD_THREADS
};

static_assert(static_cast<WorldFlag>(PUGL_WORLD_THREADS) == WorldFlag::threads,
              "");

/// @copydoc PuglWorldFlags
using WorldFlags = PuglWorldFlags;

#if defined(PUGL_HPP_THROW_FAILED_CONSTRUCTION)

/// An exception thrown when construction fails
class FailedConstructionError : public std::exception
{
public:
  FailedConstructionError(const char* const msg) noexcept
    : _msg{msg}
  {}

  const char* what() const noexcept override;

private:
  const char* _msg;
};

#  define PUGL_CHECK_CONSTRUCTION(cond, msg) \
    do {                                     \
      if (!(cond)) {                         \
        throw FailedConstructionError(msg);  \
      }                                      \
    } while (0)

#elif defined(PUGL_HPP_ASSERT_CONSTRUCTION)
#  define PUGL_CHECK_CONSTRUCTION(cond, msg) assert(cond);
#else
/**
   Configurable macro for handling construction failure.

   If `PUGL_HPP_THROW_FAILED_CONSTRUCTION` is defined, then this throws a
   `pugl::FailedConstructionError` if construction fails.

   If `PUGL_HPP_ASSERT_CONSTRUCTION` is defined, then this asserts if
   construction fails.

   Otherwise, this does nothing.
*/
#  define PUGL_CHECK_CONSTRUCTION(cond, msg)
#endif

/// @copydoc PuglWorld
class World : public detail::Wrapper<PuglWorld, puglFreeWorld>
{
public:
  World(const World&)            = delete;
  World& operator=(const World&) = delete;

  World(World&&)            = delete;
  World& operator=(World&&) = delete;

  ~World() = default;

  World(WorldType type, WorldFlag flag)
    : Wrapper{puglNewWorld(static_cast<PuglWorldType>(type),
                           static_cast<PuglWorldFlags>(flag))}
  {
    PUGL_CHECK_CONSTRUCTION(cobj(), "Failed to create pugl::World");
  }

  World(WorldType type, WorldFlags flags)
    : Wrapper{puglNewWorld(static_cast<PuglWorldType>(type), flags)}
  {
    PUGL_CHECK_CONSTRUCTION(cobj(), "Failed to create pugl::World");
  }

  explicit World(WorldType type)
    : World{type, WorldFlags{}}
  {}

  /// @copydoc puglGetNativeWorld
  void* nativeWorld() noexcept { return puglGetNativeWorld(cobj()); }

  /// @copydoc puglSetWorldString
  Status setString(StringHint key, const char* const value) noexcept
  {
    return static_cast<Status>(
      puglSetWorldString(cobj(), static_cast<PuglStringHint>(key), value));
  }

  /// @copydoc puglGetWorldString
  const char* getString(StringHint key) noexcept
  {
    return puglGetWorldString(cobj(), static_cast<PuglStringHint>(key));
  }

  /// @copydoc puglGetTime
  double time() const noexcept { return puglGetTime(cobj()); }

  /// @copydoc puglUpdate
  Status update(const double timeout) noexcept
  {
    return static_cast<Status>(puglUpdate(cobj(), timeout));
  }
};

/**
   @}
   @defgroup puglpp_view View
   @{
*/

/// @copydoc PuglBackend
using Backend = PuglBackend;

/// @copydoc PuglNativeView
using NativeView = PuglNativeView;

/// @copydoc PuglSizeHint
enum class SizeHint {
  defaultSize, ///< @copydoc PUGL_DEFAULT_SIZE
  minSize,     ///< @copydoc PUGL_MIN_SIZE
  maxSize,     ///< @copydoc PUGL_MAX_SIZE
  minAspect,   ///< @copydoc PUGL_MIN_ASPECT
  maxAspect,   ///< @copydoc PUGL_MAX_ASPECT
};

/// @copydoc PuglViewHint
enum class ViewHint {
  contextAPI,          ///< @copydoc PUGL_CONTEXT_API
  contextVersionMajor, ///< @copydoc PUGL_CONTEXT_VERSION_MAJOR
  contextVersionMinor, ///< @copydoc PUGL_CONTEXT_VERSION_MINOR
  contextProfile,      ///< @copydoc PUGL_CONTEXT_PROFILE
  contextDebug,        ///< @copydoc PUGL_CONTEXT_DEBUG
  redBits,             ///< @copydoc PUGL_RED_BITS
  greenBits,           ///< @copydoc PUGL_GREEN_BITS
  blueBits,            ///< @copydoc PUGL_BLUE_BITS
  alphaBits,           ///< @copydoc PUGL_ALPHA_BITS
  depthBits,           ///< @copydoc PUGL_DEPTH_BITS
  stencilBits,         ///< @copydoc PUGL_STENCIL_BITS
  sampleBuffers,       ///< @copydoc PUGL_SAMPLE_BUFFERS
  samples,             ///< @copydoc PUGL_SAMPLES
  doubleBuffer,        ///< @copydoc PUGL_DOUBLE_BUFFER
  swapInterval,        ///< @copydoc PUGL_SWAP_INTERVAL
  resizable,           ///< @copydoc PUGL_RESIZABLE
  ignoreKeyRepeat,     ///< @copydoc PUGL_IGNORE_KEY_REPEAT
  refreshRate,         ///< @copydoc PUGL_REFRESH_RATE
  viewType,            ///< @copydoc PUGL_VIEW_TYPE
  darkFrame,           ///< @copydoc PUGL_DARK_FRAME
};

static_assert(static_cast<ViewHint>(PUGL_DARK_FRAME) == ViewHint::darkFrame,
              "");

/// @copydoc PuglViewHintValue
using ViewHintValue = PuglViewHintValue;

/// @copydoc PuglCursor
enum class Cursor {
  arrow,           ///< @copydoc PUGL_CURSOR_ARROW
  caret,           ///< @copydoc PUGL_CURSOR_CARET
  crosshair,       ///< @copydoc PUGL_CURSOR_CROSSHAIR
  hand,            ///< @copydoc PUGL_CURSOR_HAND
  no,              ///< @copydoc PUGL_CURSOR_NO
  leftRight,       ///< @copydoc PUGL_CURSOR_LEFT_RIGHT
  upDown,          ///< @copydoc PUGL_CURSOR_UP_DOWN
  upLeftDownRight, ///< @copydoc PUGL_CURSOR_UP_LEFT_DOWN_RIGHT
  upRightDownLeft, ///< @copydoc PUGL_CURSOR_UP_RIGHT_DOWN_LEFT
  allScroll,       ///< @copydoc PUGL_CURSOR_ALL_SCROLL
};

static_assert(static_cast<Cursor>(PUGL_CURSOR_ALL_SCROLL) == Cursor::allScroll,
              "");

/// @copydoc PuglShowCommand
enum class ShowCommand {
  passive,    ///< @copydoc PUGL_SHOW_PASSIVE
  raise,      ///< @copydoc PUGL_SHOW_RAISE
  forceRaise, ///< @copydoc PUGL_SHOW_FORCE_RAISE
};

static_assert(static_cast<ShowCommand>(PUGL_SHOW_FORCE_RAISE) ==
                ShowCommand::forceRaise,
              "");

/// @copydoc PuglView
class View : protected detail::Wrapper<PuglView, puglFreeView>
{
public:
  /**
     @name Setup
     Methods for creating and destroying a view.
     @{
  */

  explicit View(World& world)
    : Wrapper{puglNewView(world.cobj())}
    , _world(world)
  {
    PUGL_CHECK_CONSTRUCTION(cobj(), "Failed to create pugl::View");
  }

  const World& world() const noexcept { return _world; }
  World&       world() noexcept { return _world; }

  /**
     Set the object that will be called to handle events.

     This is a type-safe wrapper for the C functions puglSetHandle() and
     puglSetEventFunc() that will automatically dispatch events to the
     `onEvent` method of `handler` that takes the appropriate event type.
     The handler must have such a method defined for every event type, but if
     the handler is the view itself, a `using` declaration can be used to
     "inherit" the default implementation to avoid having to define every
     method.  For example:

     @code
     class MyView : public pugl::View
     {
     public:
       explicit MyView(pugl::World& world)
         : pugl::View{world}
       {
         setEventHandler(*this);
       }

       using pugl::View::onEvent;

       pugl::Status onEvent(const pugl::ConfigureEvent& event) noexcept;
       pugl::Status onEvent(const pugl::ExposeEvent& event) noexcept;
     };
     @endcode

     This facility is just a convenience, applications may use the C API
     directly to set a handle and event function to set up a different
     approach for event handling.
  */
  template<class Handler>
  Status setEventHandler(Handler& handler)
  {
    puglSetHandle(cobj(), &handler);
    return static_cast<Status>(puglSetEventFunc(cobj(), eventFunc<Handler>));
  }

  /// @copydoc puglSetBackend
  Status setBackend(const PuglBackend* backend) noexcept
  {
    return static_cast<Status>(puglSetBackend(cobj(), backend));
  }

  /// @copydoc puglSetViewHint
  Status setHint(ViewHint hint, int value) noexcept
  {
    return static_cast<Status>(
      puglSetViewHint(cobj(), static_cast<PuglViewHint>(hint), value));
  }

  /// @copydoc puglGetViewHint
  int getHint(ViewHint hint) noexcept
  {
    return puglGetViewHint(cobj(), static_cast<PuglViewHint>(hint));
  }

  /// @copydoc puglSetViewString
  Status setString(StringHint key, const char* const value) noexcept
  {
    return static_cast<Status>(
      puglSetViewString(cobj(), static_cast<PuglStringHint>(key), value));
  }

  /// @copydoc puglGetViewString
  const char* getString(StringHint key) noexcept
  {
    return puglGetViewString(cobj(), static_cast<PuglStringHint>(key));
  }

  /**
     @}
     @name Frame
     Methods for working with the position and size of a view.
     @{
  */

  /// @copydoc puglGetFrame
  Rect frame() const noexcept { return puglGetFrame(cobj()); }

  /// @copydoc puglSetFrame
  Status setFrame(const Rect& frame) noexcept
  {
    return static_cast<Status>(puglSetFrame(cobj(), frame));
  }

  /// @copydoc puglSetSizeHint
  Status setSizeHint(SizeHint hint, PuglSpan width, PuglSpan height) noexcept
  {
    return static_cast<Status>(
      puglSetSizeHint(cobj(), static_cast<PuglSizeHint>(hint), width, height));
  }

  /**
     @}
     @name Windows
     Methods for working with top-level windows.
     @{
  */

  /// @copydoc puglSetParentWindow
  Status setParentWindow(NativeView parent) noexcept
  {
    return static_cast<Status>(puglSetParentWindow(cobj(), parent));
  }

  /// @copydoc puglSetTransientParent
  Status setTransientParent(NativeView parent) noexcept
  {
    return static_cast<Status>(puglSetTransientParent(cobj(), parent));
  }

  /// @copydoc puglRealize
  Status realize() noexcept { return static_cast<Status>(puglRealize(cobj())); }

  /// @copydoc puglShow
  Status show(const ShowCommand command) noexcept
  {
    return static_cast<Status>(
      puglShow(cobj(), static_cast<PuglShowCommand>(command)));
  }

  /// @copydoc puglHide
  Status hide() noexcept { return static_cast<Status>(puglHide(cobj())); }

  /// @copydoc puglGetVisible
  bool visible() const noexcept { return puglGetVisible(cobj()); }

  /// @copydoc puglGetNativeView
  NativeView nativeView() noexcept { return puglGetNativeView(cobj()); }

  /**
     @}
     @name Graphics
     Methods for working with the graphics context and scheduling
     redisplays.
     @{
  */

  /// @copydoc puglGetContext
  void* context() noexcept { return puglGetContext(cobj()); }

  /// @copydoc puglPostRedisplay
  Status postRedisplay() noexcept
  {
    return static_cast<Status>(puglPostRedisplay(cobj()));
  }

  /// @copydoc puglPostRedisplayRect
  Status postRedisplayRect(const Rect& rect) noexcept
  {
    return static_cast<Status>(puglPostRedisplayRect(cobj(), rect));
  }

  /**
     @}
     @name Interaction
     Methods for interacting with the user and window system.
     @{
  */

  /// @copydoc puglGrabFocus
  Status grabFocus() noexcept
  {
    return static_cast<Status>(puglGrabFocus(cobj()));
  }

  /// @copydoc puglHasFocus
  bool hasFocus() const noexcept { return puglHasFocus(cobj()); }

  /// @copydoc puglSetCursor
  Status setCursor(const Cursor cursor) noexcept
  {
    return static_cast<Status>(
      puglSetCursor(cobj(), static_cast<PuglCursor>(cursor)));
  }

  /// @copydoc puglGetNumClipboardTypes
  uint32_t numClipboardTypes() const
  {
    return puglGetNumClipboardTypes(cobj());
  }

  /// @copydoc puglGetClipboardType
  const char* clipboardType(const uint32_t typeIndex) const
  {
    return puglGetClipboardType(cobj(), typeIndex);
  }

  /**
     Accept data offered from a clipboard.

     To accept data, this must be called while handling a #PUGL_DATA_OFFER
     event. Doing so will request the data from the source as the specified
     type.  When the data is available, a #PUGL_DATA event will be sent to the
     view which can then retrieve the data with puglGetClipboard().

     @param offer The data offer event.

     @param typeIndex The index of the type that the view will accept.  This is
     the `typeIndex` argument to the call of puglGetClipboardType() that
     returned the accepted type.
  */
  Status acceptOffer(const DataOfferEvent& offer, const uint32_t typeIndex)
  {
    return static_cast<Status>(puglAcceptOffer(cobj(), &offer, typeIndex));
  }

  /// @copydoc puglSetViewStyle
  Status setViewStyle(const PuglViewStyleFlags flags)
  {
    return static_cast<Status>(puglSetViewStyle(cobj(), flags));
  }

  /**
     Activate a repeating timer event.

     This starts a timer which will send a timer event to `view` every
     `timeout` seconds.  This can be used to perform some action in a view at a
     regular interval with relatively low frequency.  Note that the frequency
     of timer events may be limited by how often update() is called.

     If the given timer already exists, it is replaced.

     @param id The identifier for this timer.  This is an application-specific
     ID that should be a low number, typically the value of a constant or `enum`
     that starts from 0.  There is a platform-specific limit to the number of
     supported timers, and overhead associated with each, so applications should
     create only a few timers and perform several tasks in one if necessary.

     @param timeout The period, in seconds, of this timer.  This is not
     guaranteed to have a resolution better than 10ms (the maximum timer
     resolution on Windows) and may be rounded up if it is too short.  On X11
     and MacOS, a resolution of about 1ms can usually be relied on.

     @return #PUGL_FAILURE if timers are not supported by the system,
     #PUGL_UNKNOWN_ERROR if setting the timer failed.
  */
  Status startTimer(const std::uintptr_t id, const double timeout) noexcept
  {
    return static_cast<Status>(puglStartTimer(cobj(), id, timeout));
  }

  /**
     Stop an active timer.

     @param id The ID previously passed to startTimer().

     @return #PUGL_FAILURE if timers are not supported by this system,
     #PUGL_UNKNOWN_ERROR if stopping the timer failed.
  */
  Status stopTimer(const std::uintptr_t id) noexcept
  {
    return static_cast<Status>(puglStopTimer(cobj(), id));
  }

  template<PuglEventType t, class Base>
  Status sendEvent(const Event<t, Base>& event) noexcept
  {
    PuglEvent cEvent{{t, 0}};

    *reinterpret_cast<Base*>(&cEvent) = event;

    return static_cast<Status>(puglSendEvent(cobj(), &cEvent));
  }

  /**
     @}
  */

  PuglView*       cobj() noexcept { return Wrapper::cobj(); }
  const PuglView* cobj() const noexcept { return Wrapper::cobj(); }

private:
  template<class Target>
  static Status dispatch(Target& target, const PuglEvent* event)
  {
    switch (event->type) {
    case PUGL_NOTHING:
      return Status::success;
    case PUGL_REALIZE:
      return target.onEvent(RealizeEvent{event->any});
    case PUGL_UNREALIZE:
      return target.onEvent(UnrealizeEvent{event->any});
    case PUGL_CONFIGURE:
      return target.onEvent(ConfigureEvent{event->configure});
    case PUGL_UPDATE:
      return target.onEvent(UpdateEvent{event->any});
    case PUGL_EXPOSE:
      return target.onEvent(ExposeEvent{event->expose});
    case PUGL_CLOSE:
      return target.onEvent(CloseEvent{event->any});
    case PUGL_FOCUS_IN:
      return target.onEvent(FocusInEvent{event->focus});
    case PUGL_FOCUS_OUT:
      return target.onEvent(FocusOutEvent{event->focus});
    case PUGL_KEY_PRESS:
      return target.onEvent(KeyPressEvent{event->key});
    case PUGL_KEY_RELEASE:
      return target.onEvent(KeyReleaseEvent{event->key});
    case PUGL_TEXT:
      return target.onEvent(TextEvent{event->text});
    case PUGL_POINTER_IN:
      return target.onEvent(PointerInEvent{event->crossing});
    case PUGL_POINTER_OUT:
      return target.onEvent(PointerOutEvent{event->crossing});
    case PUGL_BUTTON_PRESS:
      return target.onEvent(ButtonPressEvent{event->button});
    case PUGL_BUTTON_RELEASE:
      return target.onEvent(ButtonReleaseEvent{event->button});
    case PUGL_MOTION:
      return target.onEvent(MotionEvent{event->motion});
    case PUGL_SCROLL:
      return target.onEvent(ScrollEvent{event->scroll});
    case PUGL_CLIENT:
      return target.onEvent(ClientEvent{event->client});
    case PUGL_TIMER:
      return target.onEvent(TimerEvent{event->timer});
    case PUGL_LOOP_ENTER:
      return target.onEvent(LoopEnterEvent{event->any});
    case PUGL_LOOP_LEAVE:
      return target.onEvent(LoopLeaveEvent{event->any});
    case PUGL_DATA_OFFER:
      return target.onEvent(DataOfferEvent{event->offer});
    case PUGL_DATA:
      return target.onEvent(DataEvent{event->data});
    }

    return Status::failure;
  }

  template<class Target>
  static PuglStatus eventFunc(PuglView* view, const PuglEvent* event) noexcept
  {
    auto* target = static_cast<Target*>(puglGetHandle(view));

#ifdef __cpp_exceptions
    try {
      return static_cast<PuglStatus>(dispatch(*target, event));
    } catch (...) {
      return PUGL_UNKNOWN_ERROR;
    }
#else
    return static_cast<PuglStatus>(pugl::dispatch(*target, event));
#endif
  }

  World& _world;
};

/**
   @}
   @}
*/

} // namespace pugl

#endif // PUGL_PUGL_HPP
