cmake_minimum_required(VERSION 3.14)

if(NOT IS_ABSOLUTE "${TEST_BINARY_DIR}" OR
   NOT EXISTS "${IPLUG2_DIR}/Tests/Linux/WebView/Configure/CMakeLists.txt")
  message(FATAL_ERROR "Pass an absolute TEST_BINARY_DIR and a valid IPLUG2_DIR.")
endif()

foreach(mode core examples examples-darwin module plugin embedded direct)
  execute_process(COMMAND "${CMAKE_COMMAND}"
    -S "${IPLUG2_DIR}/Tests/Linux/WebView/Configure"
    -B "${TEST_BINARY_DIR}/${mode}"
    "-DIPLUG2_DIR=${IPLUG2_DIR}" "-DTEST_MODE=${mode}"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
  if(mode STREQUAL "core" OR mode MATCHES "^examples")
    if(NOT result EQUAL 0)
      message(FATAL_ERROR "${mode}: supported configuration failed\n${output}\n${error}")
    endif()
  else()
    if(result EQUAL 0)
      message(FATAL_ERROR "${mode}: unsupported WebView configuration succeeded")
    endif()
    if(mode STREQUAL "direct")
      set(expected "iPlug2::Extras::IWebViewControl")
    elseif(mode STREQUAL "embedded")
      set(expected "IWebViewControl is not implemented on Linux")
    else()
      set(expected "WebView is not implemented on Linux")
    endif()
    string(REGEX REPLACE "[ \t\r\n]+" " " diagnostic "${output}\n${error}")
    string(FIND "${diagnostic}" "${expected}" position)
    if(position EQUAL -1)
      message(FATAL_ERROR "${mode}: missing unsupported-WebView diagnostic\n${output}\n${error}")
    endif()
  endif()
  message(STATUS "WebView configuration ${mode}: passed")
endforeach()
