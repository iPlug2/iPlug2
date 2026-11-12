cmake_minimum_required(VERSION 3.14)

if(NOT IS_ABSOLUTE "${TEST_BINARY_DIR}" OR NOT EXISTS "${IPLUG2_DIR}/Tests/Linux/SDK/CMakeLists.txt")
  message(FATAL_ERROR "Pass an absolute TEST_BINARY_DIR and a valid IPLUG2_DIR.")
endif()

set(fixture "${IPLUG2_DIR}/Tests/Linux/SDK")
set(deps "${TEST_BINARY_DIR}/deps")

function(check_sdk label expected)
  execute_process(COMMAND "${CMAKE_COMMAND}" -S "${fixture}" -B "${TEST_BINARY_DIR}/${label}"
    "-DIPLUG2_DIR=${IPLUG2_DIR}" "-DTEST_DEPS_DIR=${deps}" "-DEXPECT_SUPPORTED=${expected}"
    RESULT_VARIABLE result)
  if(NOT result EQUAL 0)
    message(FATAL_ERROR "${label}: SDK configuration regression")
  endif()
  execute_process(COMMAND "${CMAKE_COMMAND}" --build "${TEST_BINARY_DIR}/${label}" --target SDKProbe-app
    RESULT_VARIABLE result)
  if(NOT result EQUAL 0)
    message(FATAL_ERROR "${label}: standalone build regression")
  endif()
endfunction()

# Use a fresh, bounded fixture on every run, including repeated CTest invocations.
file(REMOVE_RECURSE "${deps}/VST3_SDK")
check_sdk(absent FALSE)
file(MAKE_DIRECTORY "${deps}/VST3_SDK")
file(WRITE "${deps}/VST3_SDK/README.md" "SDK placeholder\n")
check_sdk(placeholder FALSE)

file(READ "${TEST_BINARY_DIR}/absent/required-files.txt" required_files)
foreach(path IN LISTS required_files)
  get_filename_component(directory "${path}" DIRECTORY)
  file(MAKE_DIRECTORY "${directory}")
  file(WRITE "${path}" "")
endforeach()
check_sdk(complete TRUE)
file(REMOVE "${deps}/VST3_SDK/public.sdk/source/main/linuxmain.cpp")
check_sdk(missing-source FALSE)
file(WRITE "${deps}/VST3_SDK/public.sdk/source/main/linuxmain.cpp" "")
file(REMOVE "${deps}/VST3_SDK/pluginterfaces/vst/ivstaudioprocessor.h")
check_sdk(missing-header FALSE)
