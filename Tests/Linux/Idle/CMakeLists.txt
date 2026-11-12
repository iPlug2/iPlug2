include("${IPLUG2_DIR}/iPlug2.cmake")
find_package(iPlug2 REQUIRED)

iplug_add_plugin(LinuxIdleProbe
  SOURCES IdleProbe.cpp IdleProbe.h
  FORMATS APP VST3 CLAP)

iplug_add_plugin(LinuxIdleHeadless
  SOURCES IdleProbe.cpp IdleProbe.h
  FORMATS VST3 CLAP
  UI NONE
  DEFINES NO_IGRAPHICS)

foreach(format app vst3 clap)
  if(TARGET LinuxIdleProbe-${format})
    target_include_directories(LinuxIdleProbe-${format} BEFORE PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}"
      "${IPLUG2_DIR}/Examples/IPlugEffect"
      "${IPLUG2_DIR}/Examples/IPlugEffect/resources")
  endif()
endforeach()

foreach(format vst3 clap)
  if(TARGET LinuxIdleHeadless-${format})
    target_include_directories(LinuxIdleHeadless-${format} BEFORE PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
  endif()
endforeach()

set_target_properties(LinuxIdleProbe-app PROPERTIES ENABLE_EXPORTS ON)
pkg_check_modules(PROBE_GTK REQUIRED IMPORTED_TARGET gtk+-3.0)
target_link_libraries(LinuxIdleProbe-app PRIVATE PkgConfig::PROBE_GTK)
