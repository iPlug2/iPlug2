# About the project level preprocessor macros
\todo improve this!

IPlug uses preprocessor macros to select certain APIs and functionality at compile time. The following macros can be defined at project level (in the visual studio project/.props file, xcode project/xconfig file, or CMake script). 

##IPlug
* VST_API | VST3_API | AU_API | AUV3_API | AAX_API | APP_API | WAM_API | WEB_API | VST3C_API | VST3P_API
* USE_IDLE_CALLS: if this is enabled as a preprocessor macro IPlug::OnIdle() will be called in VST2 plug-ins
* IPLUG1_COMPATIBILITY: if you're upgrading an existing product, you should define this so that compatibility is maintained with your existing state
* PARAMS_MUTEX: lock a mutex when accessing mParams
 
##IGraphics
* NO_IGRAPHICS: define this to build your plug-in without IGraphics UI functionality. you can also use it to quickly test the plug-in without interface:

  `#ifdef NO_IGRAPHICS
  IGraphics* pGraphics = MakeGraphics(*this, kWidth, kHeight, 60);
  pGraphics->AttachPanelBackground(COLOR_GRAY);
  ...#endif`


* IGRAPHICS_LICE: (default) define this in order to build your plug-in using LICE as the drawing API
* IGRAPHICS_CAIRO: define this in order to build your plug-in using CAIRO as the drawing API
* IGRAPHICS_NANOVG: define this in order to build your plug-in using 9 avg as the drawing API
* IGRAPHICS_AGG: define this in order to build your plug-in using Anti-Grain Geometry as the drawing API
* IGRAPHICS_SKIA: define this in order to build your plug-in using Skia as the drawing API

* IGRAPHICS_GL | IGRAPHICS_METAL | IGRAPHICS_CPU

* CONTROL_BOUNDS_COLOR=COLOR_WHITE : you can define this 2 change the color of control bounds when IGraphics::ShowControlBounds(true) is set (debug mode only). The default color is green. 
* USE_IDLE_CALLS: if this is enabled as a preprocessor macro IGraphics::OnGUIIdle() will be called
* IGRAPHICS_NO_CONTEXT_MENU: if this is enabled as a preprocessor macro right clicking control will mean IControl::CreateContextMenu() and IControl::OnContextSelection() do not function on right clicking control. VST3 provides contextual menu support which is hard wired to right click controls by default. You can add custom items to the menu by implementing IControl::CreateContextMenu() and handle them in IControl::OnContextSelection(). In non-VST 3 hosts right clicking will still create the menu, but it will not feature entries added by the host. 
* IPLUG_JPEG_SUPPORT: Declare and include libjpeg files from WDL folder and lice jpg files if you need to load JPGs with IGRAPHICS_LICE
##TRACER
* TRACETOSTDOUT
