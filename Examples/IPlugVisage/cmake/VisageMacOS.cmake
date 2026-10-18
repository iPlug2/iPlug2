# bgfx owns drawable acquisition and presentation. Asking MTKView for its
# currentDrawable first reserves another drawable from the same CAMetalLayer,
# starving bgfx's drawable pool and stalling the UI thread in nextDrawable.
# Build a corrected copy so FetchContent and local Visage checkouts stay clean.
function(iplug_visage_fix_macos_drawable)
  set(_source "${visage_SOURCE_DIR}/visage_windowing/macos/windowing_macos.mm")
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${_source}")
  file(READ "${_source}" _contents)
  set(_old [=[  if (!view.currentDrawable || !view.currentRenderPassDescriptor)
    return;
]=])
  string(FIND "${_contents}" "${_old}" _position)
  if(_position EQUAL -1)
    message(STATUS "Visage MTKView drawable guard not found; macOS workaround not applied")
    return()
  endif()
  string(REPLACE "${_old}"
    "  // bgfx acquires and presents the drawable; MTKView only drives frame timing.\n"
    _contents "${_contents}")
  set(_patched "${CMAKE_CURRENT_BINARY_DIR}/visage_macos/windowing_macos.mm")
  file(CONFIGURE OUTPUT "${_patched}" CONTENT "${_contents}" @ONLY)
  set_source_files_properties("${_source}" TARGET_DIRECTORY VisageWindowing
    PROPERTIES HEADER_FILE_ONLY TRUE)
  target_sources(VisageWindowing PRIVATE "${_patched}")
  target_include_directories(VisageWindowing PRIVATE "${visage_SOURCE_DIR}/visage_windowing/macos")
endfunction()

iplug_visage_fix_macos_drawable()
