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

option(IPLUG_LINUX_WEBVIEW_EDITOR_TEST "Build the WebKitGTK CLAP editor regression test" OFF)
if(IPLUG_LINUX_WEBVIEW_EDITOR_TEST)
  iplug_add_plugin(LinuxWebEditor
    SOURCES WebEditor.cpp WebEditor.h
    WEB_RESOURCES resources/web/index.html
    FORMATS CLAP UI WEBVIEW)
  target_include_directories(LinuxWebEditor-clap BEFORE PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/EditorConfig")
  set_target_properties(LinuxWebEditor-clap PROPERTIES ENABLE_EXPORTS ON)
  add_dependencies(LinuxClapHostTest LinuxWebEditor-clap)
  add_test(NAME linux-webview-editor
    COMMAND "${XVFB_RUN}" -a "$<TARGET_FILE:LinuxClapHostTest>" "$<TARGET_FILE:LinuxWebEditor-clap>" webview)
  set_tests_properties(linux-webview-editor PROPERTIES TIMEOUT 45
    ENVIRONMENT "WEBKIT_DISABLE_SANDBOX_THIS_IS_DANGEROUS=1;LIBGL_ALWAYS_SOFTWARE=1")
  if(TARGET LinuxVst3HostTest)
    iplug_add_plugin(LinuxWebEditorVST3
      SOURCES WebEditor.cpp WebEditor.h
      WEB_RESOURCES resources/web/index.html
      FORMATS VST3 UI WEBVIEW)
    target_include_directories(LinuxWebEditorVST3-vst3 BEFORE PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/EditorConfig")
    set_target_properties(LinuxWebEditorVST3-vst3 PROPERTIES ENABLE_EXPORTS ON)
    add_dependencies(LinuxVst3HostTest LinuxWebEditorVST3-vst3)
    add_test(NAME linux-webview-vst3
      COMMAND "${XVFB_RUN}" -a "$<TARGET_FILE:LinuxVst3HostTest>" "$<TARGET_FILE:LinuxWebEditorVST3-vst3>" webview)
    set_tests_properties(linux-webview-vst3 PROPERTIES TIMEOUT 45
      ENVIRONMENT "WEBKIT_DISABLE_SANDBOX_THIS_IS_DANGEROUS=1;LIBGL_ALWAYS_SOFTWARE=1")
  endif()
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
