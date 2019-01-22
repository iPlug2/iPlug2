# Advanced Documentation

Here are some docs for experienced developers who want to try and understand the inner workings of IPlug better, fix bugs and contribute pull requests.

## What is WDL?

IPlug depends on [WDL](https://www.cockos.com/wdl/) (whittle), which is Cockos' open source library of lightweight reusable code for making cross-platform software. WDL makes very little use of the STL or of modern C++ features. The underlying code is a challenge to understand. It is arcane in places, and poorly documented but it generally works very well. It is developed by some great people who make legendary software and it powers what is undoubtably the best DAW - Reaper (at least from a programmer's perspective)! There is a reason Reaper is blazingly fast and lightweight.
The key parts of WDL that IPlug uses are: 

- [WDL_String](https://github.com/justinfrankel/WDL/blob/master/WDL/wdlstring.h): a simple class for variable-length string manipulation
- [WDL_PtrList](https://github.com/justinfrankel/WDL/blob/master/WDL/ptrlist.h): a simple templated class for a list of pointers
- [WDL_Heapbuf](https://github.com/justinfrankel/WDL/blob/master/WDL/heapbuf.h): a class and templated class (WDL_Typedbuf) for managing a block of memory
- [WDL_Mutex](https://github.com/justinfrankel/WDL/blob/master/WDL/mutex.h): a simple class that abstracts a mutex or critical section object
- [SWELL](https://github.com/justinfrankel/WDL/tree/master/WDL/swell): a wrapper that translates win32 API code to work on macOS Cocoa (and Linux GTK). In IPlug it is only used in order to build stand-alone versions of plug-ins, although it is also possible to use win32 controls as your UI and swell to make them work on macOS using native Cocoa controls. Pro tip: don't look at the swell code you will have nightmares.

IPlug makes extensive use of preprocessor macros in order to switch functionality, without bloating binaries. This can get ugly at times, but is unavoidable in a cross platform multi-format/api bridge such as this.

## IPlug

### IPlugBase
IPlugBase is the base class for an IPlug plug-in, that will be inherited by classes that deal with the various plug-in format API stuff (e.g. IPlugVST), and eventually inherited by your plug-in implementation. It knows nothing about drawing or platform specific stuff such as locations of common folders. It facilitates plug-in set up, creation of parameters, managing of state and factory presets. 

### TODO: IPlugProcessor

IPlugProcessor is a lightweight base class to handle purely the audio processing side of the plug-in. This is primarily for WAMs, for which we require as little C++ as necessary to be compiled into web assembly, in order to minimise binary sizes. Normally for desktop plug-ins you will not deal with this class. 

## IGraphics (optional)

IGraphics is the pure virtual *interface* primarily for drawing but also for doing platform specific stuff such as locating certain folders or creating native controls such as pop-up menus. There are several different classes that inherit the IGraphics interface depending on which drawing API we want to use and which platform we are running on. To make this more complicated - on macOS, there are two different types of platform API: Cocoa (which uses Objective-C) and Carbon. Carbon is a deprecated API which does not support 64 -bit but you may decide it is necessary to support this for compatibility with some older 32 bit hosts. IPlug is designed to abstract these things away from plug-in implementation but it's good to know about them.

Different drawing APIs have different benefits:

* IGraphicsLice: This is the only drawing API that was in IPlug1. It uses Cockos' LICE (Lightweight Image Compositing Engine), which is fast, lightweight and functional, but does not offer much in the way of vector graphics. Text is drawn using the operating system font routines, which means that fonts look different on different platforms. This requires usage of SWELL to access OS font functionality.
* IGraphicsCairo: This uses the popular fast and powerful Cairo vector graphics library, but is quite heavyweight and will make your binaries bigger than if you use LICE. It can also be difficult to build the Cairo static libraries. Text is drawn with freetype.
* IGraphicsAGG: This uses Anti-Grain Geometry 2.4. This is an experimental IGraphics API class that can produce very nice results but appears to be quite a lot slower than other options. The drawing code is extremely verbose and complicated. Text is drawn with freetype which is quite a big dependency.
* IGraphicsNanoVG: TODO

### Selecting API
In order to provide support for multiple plug-in, drawing and platform APIs IPlug uses preprocessor macros extensively to switch different base classes depending on the target platform, drawing library. On macOS, IGraphicsMac is the top level class which inherits from IGRAPHICS_DRAW_CLASS. The value of this is changed depending on what preprocessor macros are set. Usually the preprocessor macros would be set in the MyPlugin.xcconfig file (on Mac) and MyPlugin.props (on Windows). These are simple files that are read by the IDE projects, and allow you to set build settings for multiple targets once rather than repeating the process manually inside the IDE for each target. For example to use cairo you would define IGRAPHICS_CAIRO in EXTRA_ALL_DEFS, which would set that preprocessor macro for all targets.

### Drawing
You create and attach a graphics object in your plug-ins constructor, using MakeGraphics() and IPlugBaseGraphics::AttachGraphics(). Depending on the frame rate that you specify when calling MakeGraphics, a platform timer will be created that triggers a callback at regular intervals (by default 30 times per second or every ~33 milliseconds). On macOS this callback is IGraphicsView::onTimer. This method will call mGraphics::IsDirty(), to ascertain which rectangular region of the plug-in's user interface needs to be drawn on this frame. IGraphics::IsDirty() loops through the plug-in controls checking if any of them are set "dirty", and if they are adding the rectangular region they occupy to the region that is reported back. The OS will then call a method such as IGraphicsView::drawRect(), notifying that that part of the screen is to be drawn. This might be different to what we have explicitly specified needs drawing in IGraphics::IsDirty(). This will result in IGraphics::Draw() being called which will loop through the controls trying to find the ones which match this region that the OS is requesting be painted.

* Strict mode on (default): draw everything within the smallest rectangle that contains everything dirty.
// Every control is guaranteed to get no more than one Draw() call per cycle.

* Strict mode off: draw only controls that intersect something dirty.
// If there are overlapping controls, fast drawing can generate multiple Draw() calls per cycle
// (a control may be asked to draw multiple parts of itself, if it intersects with something dirty.)

There is no Z order testing - so if you bring up an "about box" or panel on top of controls that are being set dirty, those controls will still redraw. This is obviously important if the control on top has an alpha channel (see-through parts), but wasteful if not. In practice this is not a problem for the majority of plug-in apis, but if it causes problems in your plug-in, the solution is to explicitly hide controls that are not on screen. 

-->