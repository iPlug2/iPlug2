cmake_minimum_required(VERSION 3.14)
project(iPlug2Examples)

# Standard IGraphics examples
set(CMAKE_FOLDER "Examples/IGraphics")
add_subdirectory(IPlugEffect)
add_subdirectory(IPlugInstrument)
add_subdirectory(IPlugControls)
add_subdirectory(IPlugConvoEngine)
add_subdirectory(IPlugChunks)
add_subdirectory(IPlugDrumSynth)
add_subdirectory(IPlugMidiEffect)
add_subdirectory(IPlugOSCEditor)
add_subdirectory(IPlugResponsiveUI)
add_subdirectory(IPlugSideChain)
add_subdirectory(IPlugSurroundEffect)
add_subdirectory(IPlugVisualizer)

# WebView examples
set(CMAKE_FOLDER "Examples/WebView")
add_subdirectory(IPlugWebUI)
add_subdirectory(IPlugP5js)
add_subdirectory(IPlugSvelteUI)

# Swift/Cocoa examples (requires Xcode generator)
if(APPLE AND XCODE)
  set(CMAKE_FOLDER "Examples/Native UI")
  add_subdirectory(IPlugSwiftUI)
  add_subdirectory(IPlugCocoaUI)
endif()

# REAPER examples
set(CMAKE_FOLDER "Examples/REAPER")
add_subdirectory(IPlugReaperExtension)
add_subdirectory(IPlugReaperPlugin)
