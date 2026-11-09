include("${IPLUG2_DIR}/iPlug2.cmake")
find_package(iPlug2 REQUIRED)

# A standalone feasibility probe for GtkPlug inside a raw X11 host window.
# Keep WebKit optional: the supported Linux plugins do not depend on it.
option(IPLUG_LINUX_WEBKIT_EMBED_PROBE "Build the experimental X11 WebKitGTK embedding probe" OFF)
if(IPLUG_LINUX_WEBKIT_EMBED_PROBE)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(WEBKITGTK REQUIRED IMPORTED_TARGET webkit2gtk-4.1)
  add_executable(LinuxWebKitEmbedProbe EmbedProbe.cpp)
  target_link_libraries(LinuxWebKitEmbedProbe PRIVATE PkgConfig::WEBKITGTK ${X11_LIBRARIES})
  add_test(NAME linux-webkit-embed
    COMMAND "${XVFB_RUN}" -a "$<TARGET_FILE:LinuxWebKitEmbedProbe>")
  set_tests_properties(linux-webkit-embed PROPERTIES TIMEOUT 20)
endif()

iplug_add_plugin(LinuxFailedEditor
  SOURCES FailedEditor.cpp FailedEditor.h
  FORMATS CLAP
  UI NONE
  DEFINES NO_IGRAPHICS)

if(TARGET LinuxFailedEditor-clap)
  target_include_directories(LinuxFailedEditor-clap BEFORE PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
  add_dependencies(LinuxClapHostTest LinuxFailedEditor-clap)
  add_test(NAME linux-clap-editor-fail
    COMMAND "$<TARGET_FILE:LinuxClapHostTest>" "$<TARGET_FILE:LinuxFailedEditor-clap>" editor-fail)
  set_tests_properties(linux-clap-editor-fail PROPERTIES TIMEOUT 15)
endif()
