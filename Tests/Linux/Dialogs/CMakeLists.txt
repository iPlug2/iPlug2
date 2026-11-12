include("${IPLUG2_DIR}/iPlug2.cmake")
find_package(iPlug2 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED IMPORTED_TARGET gtk+-3.0)

add_executable(LinuxDialogTest DialogTest.cpp)
target_link_libraries(LinuxDialogTest PRIVATE iPlug2::IGraphics::NanoVG::GL3 PkgConfig::GTK3)
target_compile_definitions(LinuxDialogTest PRIVATE IPLUG_EDITOR=1 IPLUG_DSP=0)

add_executable(LinuxClipboardTest ClipboardTest.cpp)
target_link_libraries(LinuxClipboardTest PRIVATE iPlug2::IGraphics::NanoVG::GL3 PkgConfig::GTK3)
target_compile_definitions(LinuxClipboardTest PRIVATE IPLUG_EDITOR=1 IPLUG_DSP=0)
add_test(NAME linux-clipboard-gtk-host
  COMMAND "${XVFB_RUN}" -a "$<TARGET_FILE:LinuxClipboardTest>")
set_tests_properties(linux-clipboard-gtk-host PROPERTIES TIMEOUT 15
  ENVIRONMENT "LIBGL_ALWAYS_SOFTWARE=1;GSETTINGS_BACKEND=memory")

foreach(mode normal no-gtk)
  add_test(NAME linux-dialog-${mode}
    COMMAND "${XVFB_RUN}" -a "$<TARGET_FILE:LinuxDialogTest>" "${mode}")
  set_tests_properties(linux-dialog-${mode} PROPERTIES
    TIMEOUT 60 ENVIRONMENT "LIBGL_ALWAYS_SOFTWARE=1;GSETTINGS_BACKEND=memory")
endforeach()

add_executable(LinuxRuntimeTest RuntimeTest.cpp)
add_dependencies(LinuxRuntimeTest LinuxClipboardTest)
target_link_libraries(LinuxRuntimeTest PRIVATE iPlug2::IGraphics::NanoVG::GL3)
target_compile_definitions(LinuxRuntimeTest PRIVATE IPLUG_EDITOR=1 IPLUG_DSP=0)
set_target_properties(LinuxRuntimeTest PROPERTIES ENABLE_EXPORTS ON)

add_executable(LinuxTimerTest ../TimerTest.cpp "${IPLUG2_DIR}/IPlug/IPlugTimer.cpp")
target_include_directories(LinuxTimerTest PRIVATE "${IPLUG2_DIR}/IPlug" "${IPLUG2_DIR}/WDL")
target_compile_options(LinuxTimerTest PRIVATE -fsigned-char -include stdlib.h)
target_link_libraries(LinuxTimerTest PRIVATE pthread)
add_dependencies(LinuxRuntimeTest LinuxTimerTest)
add_test(NAME linux-timer-lifecycle COMMAND LinuxTimerTest)
set_tests_properties(linux-timer-lifecycle PROPERTIES TIMEOUT 15)

foreach(mode focus close glx-context resize resize-roundtrip double-click scaled-keys keyboard keyboard-no-im keyboard-no-im-c parent-validation dnd tooltip
  native-close host-native-close clipboard clipboard-instances clipboard-fail
  clipboard-disappearing clipboard-host-handler core-timer core-timer-early core-timer-fail)
  add_test(NAME linux-${mode}
    COMMAND "${XVFB_RUN}" -a "$<TARGET_FILE:LinuxRuntimeTest>" "${mode}")
  set_tests_properties(linux-${mode} PROPERTIES TIMEOUT 15
    ENVIRONMENT "LIBGL_ALWAYS_SOFTWARE=1;LD_PRELOAD=$<TARGET_FILE:LinuxStartupHooks>;IPLUG_LINUX_TEST_MODE=${mode}")
endforeach()

if(CMAKE_CXX_FLAGS MATCHES "-fsanitize=[^ ]*address")
  # ASan substitutes the pthread entry point inspected by the injection shim.
  set_tests_properties(linux-core-timer-fail linux-core-timer-early PROPERTIES DISABLED TRUE)
endif()
