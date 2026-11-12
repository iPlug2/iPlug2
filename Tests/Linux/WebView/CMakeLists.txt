include("${IPLUG2_DIR}/iPlug2.cmake")
find_package(iPlug2 REQUIRED)

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
