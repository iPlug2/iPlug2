include("${IPLUG2_DIR}/iPlug2.cmake")
find_package(iPlug2 REQUIRED)
include("${IPLUG2_CMAKE_DIR}/ReaperExt.cmake")

add_library(LinuxReaperProbe MODULE Probe.cpp)
target_include_directories(LinuxReaperProbe PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
  "${IPLUG2_DIR}/Examples/IPlugReaperExtension/resources")
target_link_libraries(LinuxReaperProbe PRIVATE iPlug2::IGraphics::NanoVG::GL3 iPlug2::ReaperExt)
target_link_options(LinuxReaperProbe PRIVATE
  "LINKER:--wrap=_Z25SWELL_CreateXBridgeWindowP6HWND__PPvPK4RECT")

# Supply real SWELL main-loop/window functions without compiling the standalone wrapper.
add_executable(LinuxReaperHostTest Host.cpp ${IPLUG2_SWELL_SRC})
get_target_property(host_includes iPlug2::APP INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(host_defines iPlug2::APP INTERFACE_COMPILE_DEFINITIONS)
get_target_property(host_libraries iPlug2::APP INTERFACE_LINK_LIBRARIES)
list(REMOVE_ITEM host_defines APP_API IPLUG_EDITOR=1 IPLUG_DSP=1)
list(REMOVE_ITEM host_libraries iPlug2::IPlug)
target_include_directories(LinuxReaperHostTest PRIVATE ${host_includes} "${REAPER_SDK_DIR}"
  "${IPLUG2_DIR}/IPlug")
target_compile_definitions(LinuxReaperHostTest PRIVATE ${host_defines} NOMINMAX)
target_compile_options(LinuxReaperHostTest PRIVATE -fsigned-char -include stdlib.h -Wno-multichar)
target_link_libraries(LinuxReaperHostTest PRIVATE ${host_libraries})
set_target_properties(LinuxReaperHostTest PROPERTIES ENABLE_EXPORTS ON)
add_dependencies(LinuxReaperHostTest LinuxReaperProbe LinuxStartupHooks)
add_dependencies(LinuxGuiTest LinuxReaperHostTest)
add_test(NAME linux-reaper-main-loop
  COMMAND "${XVFB_RUN}" -a "$<TARGET_FILE:LinuxReaperHostTest>" "$<TARGET_FILE:LinuxReaperProbe>")
set_tests_properties(linux-reaper-main-loop PROPERTIES TIMEOUT 20
  ENVIRONMENT "LIBGL_ALWAYS_SOFTWARE=1")
foreach(mode bridge-fail bridge-empty window-fail)
  add_test(NAME linux-reaper-${mode}
    COMMAND "${XVFB_RUN}" -a "$<TARGET_FILE:LinuxReaperHostTest>" "$<TARGET_FILE:LinuxReaperProbe>" "${mode}")
  set_tests_properties(linux-reaper-${mode} PROPERTIES TIMEOUT 20
    ENVIRONMENT "LIBGL_ALWAYS_SOFTWARE=1;LD_PRELOAD=$<TARGET_FILE:LinuxStartupHooks>;IPLUG_LINUX_TEST_MODE=${mode}")
endforeach()
