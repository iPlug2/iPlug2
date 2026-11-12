cmake_minimum_required(VERSION 3.14)
project(iPlug2LinuxTests LANGUAGES C CXX)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
  message(FATAL_ERROR "These regression tests require Linux.")
endif()

enable_testing()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(IPLUG_DEPLOY_PLUGINS OFF CACHE BOOL "Do not deploy regression test plugins" FORCE)
get_filename_component(IPLUG2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)

find_package(X11 REQUIRED)
find_package(ALSA REQUIRED)
find_program(XVFB_RUN xvfb-run)
if(NOT XVFB_RUN)
  message(FATAL_ERROR "Install xvfb and xauth to run the Linux GUI regression tests.")
endif()

add_subdirectory("${IPLUG2_DIR}/Examples/IPlugEffect" IPlugEffect)
add_subdirectory(Dialogs)
add_subdirectory(Idle)
add_subdirectory(OSC)
# Export symbols only in this test build so fault injection can identify the graphics timer.
set_target_properties(IPlugEffect-app PROPERTIES ENABLE_EXPORTS ON)
foreach(target IPlugEffect-app LinuxIdleProbe-app)
  target_sources(${target} PRIVATE MainLoopHooks.cpp)
  target_link_options(${target} PRIVATE "LINKER:--wrap=_Z8SetTimerP6HWND__mjPFvS0_jmjE")
endforeach()

add_library(LinuxStartupHooks SHARED StartupHooks.cpp)
# This interposer is also preloaded into xvfb-run's unsanitized shell/tools.
# Instrument the tested binaries, not the fault-injection shim.
target_compile_options(LinuxStartupHooks PRIVATE -fno-sanitize=all)
target_link_options(LinuxStartupHooks PRIVATE -fno-sanitize=all)
target_include_directories(LinuxStartupHooks PRIVATE ${X11_INCLUDE_DIR} ${ALSA_INCLUDE_DIRS})
target_link_libraries(LinuxStartupHooks PRIVATE ${CMAKE_DL_LIBS} pthread ${ALSA_LIBRARIES})

add_executable(LinuxGuiTest GuiTest.cpp)
target_include_directories(LinuxGuiTest PRIVATE ${X11_INCLUDE_DIR})
target_link_libraries(LinuxGuiTest PRIVATE ${X11_LIBRARIES})
add_subdirectory(Reaper)

if(TARGET IPlugEffect-vst3)
  set(VST3_SDK "${IPLUG2_DIR}/Dependencies/IPlug/VST3_SDK")
  add_executable(LinuxVst3HostTest Vst3HostTest.cpp
    "${VST3_SDK}/pluginterfaces/base/funknown.cpp"
    "${VST3_SDK}/pluginterfaces/base/coreiids.cpp"
    "${VST3_SDK}/public.sdk/source/common/commoniids.cpp"
    "${VST3_SDK}/public.sdk/source/vst/vstinitiids.cpp")
  target_include_directories(LinuxVst3HostTest PRIVATE "${VST3_SDK}" ${X11_INCLUDE_DIR})
  target_link_libraries(LinuxVst3HostTest PRIVATE ${X11_LIBRARIES} ${CMAKE_DL_LIBS} pthread)
  add_dependencies(LinuxVst3HostTest IPlugEffect-vst3)
  foreach(mode audio-state scale normal context-loop no-loop timer-fail fd-fail window-fail frame-switch native-close)
    set(scale_screen_args)
    if(mode STREQUAL "scale")
      list(APPEND scale_screen_args -s "-screen 0 1600x1600x24")
    endif()
    add_test(NAME linux-vst3-${mode}
      COMMAND "${XVFB_RUN}" -a ${scale_screen_args} "$<TARGET_FILE:LinuxVst3HostTest>"
        "$<TARGET_FILE:IPlugEffect-vst3>" "${mode}")
    set_tests_properties(linux-vst3-${mode} PROPERTIES TIMEOUT 30
      ENVIRONMENT "LIBGL_ALWAYS_SOFTWARE=1;LD_PRELOAD=$<TARGET_FILE:LinuxStartupHooks>")
  endforeach()
endif()

if(TARGET LinuxIdleProbe-vst3)
  add_dependencies(LinuxVst3HostTest LinuxIdleProbe-vst3 LinuxIdleHeadless-vst3)
  foreach(mode idle idle-frame idle-missing idle-reject close-order close-order-poll)
    add_test(NAME linux-vst3-${mode}
      COMMAND "${XVFB_RUN}" -a "$<TARGET_FILE:LinuxVst3HostTest>"
        "$<TARGET_FILE:LinuxIdleProbe-vst3>" "${mode}")
    set_tests_properties(linux-vst3-${mode} PROPERTIES TIMEOUT 30
      ENVIRONMENT "LIBGL_ALWAYS_SOFTWARE=1;LD_PRELOAD=$<TARGET_FILE:LinuxStartupHooks>")
  endforeach()
  add_test(NAME linux-vst3-idle-headless
    COMMAND "$<TARGET_FILE:LinuxVst3HostTest>" "$<TARGET_FILE:LinuxIdleHeadless-vst3>" idle-headless)
  set_tests_properties(linux-vst3-idle-headless PROPERTIES TIMEOUT 15)
endif()

add_dependencies(LinuxGuiTest LinuxIdleProbe-app)
foreach(mode idle close-order close-fail ui-thread callback-close)
  add_test(NAME linux-app-${mode}
    COMMAND "${XVFB_RUN}" -a "$<TARGET_FILE:LinuxGuiTest>"
      "$<TARGET_FILE:LinuxIdleProbe-app>" "$<TARGET_FILE:LinuxStartupHooks>" "${mode}")
  set_tests_properties(linux-app-${mode} PROPERTIES TIMEOUT 30 ENVIRONMENT "LIBGL_ALWAYS_SOFTWARE=1")
endforeach()

foreach(mode normal main-loop timer-fail window-fail)
  add_test(NAME linux-${mode}
    COMMAND "${XVFB_RUN}" -a "$<TARGET_FILE:LinuxGuiTest>"
      "$<TARGET_FILE:IPlugEffect-app>" "$<TARGET_FILE:LinuxStartupHooks>" "${mode}")
  set_tests_properties(linux-${mode} PROPERTIES TIMEOUT 60 ENVIRONMENT "LIBGL_ALWAYS_SOFTWARE=1")
endforeach()

add_test(NAME linux-vst3-sdk
  COMMAND "${CMAKE_COMMAND}"
    "-DIPLUG2_DIR=${IPLUG2_DIR}"
    "-DTEST_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}/sdk-check"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/CheckSDK.cmake")
set_tests_properties(linux-vst3-sdk PROPERTIES TIMEOUT 60)

add_test(NAME linux-webview-configuration
  COMMAND "${CMAKE_COMMAND}" "-DIPLUG2_DIR=${IPLUG2_DIR}"
    "-DTEST_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}/webview-check"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/CheckWebView.cmake")
set_tests_properties(linux-webview-configuration PROPERTIES TIMEOUT 60)

add_test(NAME linux-deployment
  COMMAND "${CMAKE_COMMAND}" "-DIPLUG2_DIR=${IPLUG2_DIR}"
    "-DTEST_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}/deployment-check"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/CheckDeployment.cmake")
set_tests_properties(linux-deployment PROPERTIES TIMEOUT 120)

add_test(NAME linux-deployment-symlink
  COMMAND "${CMAKE_COMMAND}" "-DIPLUG2_DIR=${IPLUG2_DIR}"
    "-DTEST_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}/deployment-symlink-check"
    -DTEST_DEPLOY_METHOD=SYMLINK
    -P "${CMAKE_CURRENT_SOURCE_DIR}/CheckDeployment.cmake")
set_tests_properties(linux-deployment-symlink PROPERTIES TIMEOUT 120)

foreach(method COPY SYMLINK)
  string(TOLOWER "${method}" method_name)
  add_test(NAME linux-deployment-ninja-${method_name}
    COMMAND "${CMAKE_COMMAND}" "-DIPLUG2_DIR=${IPLUG2_DIR}"
      "-DTEST_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}/deployment-ninja-${method_name}"
      -DTEST_GENERATOR=Ninja "-DTEST_DEPLOY_METHOD=${method}"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/CheckDeployment.cmake")
  set_tests_properties(linux-deployment-ninja-${method_name} PROPERTIES TIMEOUT 120)
endforeach()

if(TARGET IPlugEffect-clap)
  add_executable(LinuxClapHostTest ClapHostTest.cpp)
  target_include_directories(LinuxClapHostTest PRIVATE ${X11_INCLUDE_DIR}
    "${IPLUG2_DIR}/Dependencies/IPlug/CLAP_SDK/include")
  target_link_libraries(LinuxClapHostTest PRIVATE ${X11_LIBRARIES} ${CMAKE_DL_LIBS} pthread)
  add_dependencies(LinuxClapHostTest IPlugEffect-clap)
  foreach(mode audio-state scale extensions none timer-only fd-only timer-fail fd-fail both-fail window-fail fallback-fail
    native-close native-close-fallback)
    set(scale_screen_args)
    if(mode STREQUAL "scale")
      list(APPEND scale_screen_args -s "-screen 0 1600x1600x24")
    endif()
    add_test(NAME linux-clap-${mode}
      COMMAND "${XVFB_RUN}" -a ${scale_screen_args} "$<TARGET_FILE:LinuxClapHostTest>"
        "$<TARGET_FILE:IPlugEffect-clap>" "${mode}")
    set_tests_properties(linux-clap-${mode} PROPERTIES TIMEOUT 30
      ENVIRONMENT "LIBGL_ALWAYS_SOFTWARE=1;LD_PRELOAD=$<TARGET_FILE:LinuxStartupHooks>")
  endforeach()
endif()

if(TARGET LinuxClapHostTest)
  add_subdirectory(WebView)
endif()

if(TARGET LinuxIdleProbe-clap)
  add_dependencies(LinuxClapHostTest LinuxIdleProbe-clap LinuxIdleHeadless-clap)
  foreach(mode idle idle-none idle-fail close-order close-order-poll close-order-fallback)
    add_test(NAME linux-clap-${mode}
      COMMAND "${XVFB_RUN}" -a "$<TARGET_FILE:LinuxClapHostTest>"
        "$<TARGET_FILE:LinuxIdleProbe-clap>" "${mode}")
    set_tests_properties(linux-clap-${mode} PROPERTIES TIMEOUT 30
      ENVIRONMENT "LIBGL_ALWAYS_SOFTWARE=1;LD_PRELOAD=$<TARGET_FILE:LinuxStartupHooks>")
  endforeach()
  add_test(NAME linux-clap-idle-headless
    COMMAND "$<TARGET_FILE:LinuxClapHostTest>" "$<TARGET_FILE:LinuxIdleHeadless-clap>" idle-headless)
  set_tests_properties(linux-clap-idle-headless PROPERTIES TIMEOUT 15)
endif()

if(CMAKE_CXX_FLAGS MATCHES "-fsanitize=[^ ]*address")
  # ASan wraps the pthread entry point, hiding Timer_impl::ThreadProc from the
  # preload shim's dladdr-based fault injection. GCC jobs run these four cases;
  # the actual timer lifecycle tests still run under ASan (including leaks).
  if(TARGET LinuxClapHostTest)
    set_tests_properties(linux-clap-fallback-fail linux-clap-idle-fail PROPERTIES DISABLED TRUE)
  endif()
endif()
