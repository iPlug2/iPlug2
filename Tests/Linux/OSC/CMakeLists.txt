cmake_minimum_required(VERSION 3.14)
project(iPlug2OSCTests LANGUAGES CXX)
enable_testing()

get_filename_component(IPLUG2_OSC_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(OSC_DIR "${IPLUG2_OSC_ROOT}/IPlug/Extras/OSC")
set(OSC_TEST_SOURCES
  OSCTest.cpp
  "${OSC_DIR}/IPlugOSC.cpp"
  "${OSC_DIR}/IPlugOSC_internal.cpp"
  "${OSC_DIR}/IPlugOSC_msg.cpp"
  "${IPLUG2_OSC_ROOT}/WDL/jnetlib/util.cpp")

function(configure_osc_test target)
  target_compile_features(${target} PRIVATE cxx_std_17)
  target_include_directories(${target} PRIVATE
    "${IPLUG2_OSC_ROOT}/IPlug" "${IPLUG2_OSC_ROOT}/WDL" "${OSC_DIR}")
  target_link_libraries(${target} PRIVATE pthread)
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_options(${target} PRIVATE -fsigned-char)
  endif()
  add_test(NAME ${target} COMMAND ${target})
  set_tests_properties(${target} PROPERTIES TIMEOUT 15)
endfunction()

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT APPLE)
  message(FATAL_ERROR "OSC socket regression tests require Linux or macOS.")
endif()

# Deliberately omit IPlugTimer.cpp: the Linux path must not depend on a timer.
add_executable(LinuxOscTest ${OSC_TEST_SOURCES})
target_compile_definitions(LinuxOscTest PRIVATE OS_LINUX)
configure_osc_test(LinuxOscTest)
if(APPLE)
  message(STATUS "LinuxOscTest exercises Linux OSC polling on macOS sockets.")
  add_executable(OSCTimerCompatibilityTest ${OSC_TEST_SOURCES}
    "${IPLUG2_OSC_ROOT}/IPlug/IPlugTimer.cpp")
  configure_osc_test(OSCTimerCompatibilityTest)
  target_link_libraries(OSCTimerCompatibilityTest PRIVATE "-framework CoreFoundation")
endif()
