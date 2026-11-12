cmake_minimum_required(VERSION 3.14)
project(LinuxDeploymentRegression LANGUAGES C CXX)

include("${IPLUG2_DIR}/iPlug2.cmake")
add_library(iPlug2::CLAP INTERFACE IMPORTED)
find_package(iPlug2 REQUIRED)
set(IPLUG2_CLAP_SUPPORTED TRUE)
set(IPLUG_DEPLOY_PLUGINS ON)
set(TEST_DEPLOY_METHOD COPY CACHE STRING "Deployment method under test")
set(IPLUG_DEPLOY_METHOD "${TEST_DEPLOY_METHOD}")
set(IPLUG_CLAP_DEPLOY_PATH "${CMAKE_BINARY_DIR}/deploy" CACHE PATH "" FORCE)

add_library(Direct MODULE Probe.cpp)
iplug_configure_target(Direct CLAP Direct)

add_library(Deferred MODULE Probe.cpp)
target_sources(Deferred PRIVATE ResourceLookup.cpp "${IPLUG2_DIR}/IPlug/IPlugPaths.cpp")
target_include_directories(Deferred PRIVATE "${IPLUG2_DIR}/IPlug" "${IPLUG2_DIR}/WDL")
target_compile_definitions(Deferred PRIVATE OS_LINUX)
target_compile_options(Deferred PRIVATE -fsigned-char -include stdlib.h)
target_link_libraries(Deferred PRIVATE ${CMAKE_DL_LIBS})
iplug_configure_target(Deferred CLAP Deferred DEFER_DEPLOYMENT)
configure_file(late.txt Mutable/late.txt COPYONLY)
_iplug_add_resources(Deferred "${CMAKE_CURRENT_BINARY_DIR}/Mutable/late.txt")
iplug_deploy_target(Deferred CLAP Deferred)

_iplug_create_desktop_targets(High "CLAP" "${CMAKE_CURRENT_SOURCE_DIR}/Probe.cpp"
  "" "${CMAKE_CURRENT_SOURCE_DIR}/late.txt" "" "")

set(resource_sources "${CMAKE_CURRENT_SOURCE_DIR}/Probe.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/ResourceLookup.cpp" "${IPLUG2_DIR}/IPlug/IPlugPaths.cpp")
foreach(name Alpha Beta)
  _iplug_create_desktop_targets(${name} "CLAP" "${resource_sources}"
    "" "${CMAKE_CURRENT_SOURCE_DIR}/${name}/late.txt" "" "")
endforeach()

add_library(Bundled MODULE ${resource_sources})
set_target_properties(Bundled PROPERTIES PREFIX ""
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out/Bundled.vst3/Contents/test-linux")
set(IPLUG_VST3_DEPLOY_PATH "${CMAKE_BINARY_DIR}/deploy" CACHE PATH "" FORCE)
target_compile_definitions(Bundled PRIVATE VST3_API)
configure_file(Bundled/late.txt Bundled/late.txt COPYONLY)
_iplug_add_resources(Bundled "${CMAKE_CURRENT_BINARY_DIR}/Bundled/late.txt")
iplug_deploy_target(Bundled VST3 Bundled)

foreach(target Alpha-clap Beta-clap Bundled)
  target_include_directories(${target} PRIVATE "${IPLUG2_DIR}/IPlug" "${IPLUG2_DIR}/WDL")
  target_compile_definitions(${target} PRIVATE OS_LINUX)
  target_compile_options(${target} PRIVATE -fsigned-char -include stdlib.h)
  target_link_libraries(${target} PRIVATE ${CMAKE_DL_LIBS})
endforeach()

add_library(iPlug2::ReaperExt INTERFACE IMPORTED)
include("${IPLUG2_CMAKE_DIR}/ReaperExt.cmake")
set(IPLUG_REAPEREXT_DEPLOY_PATH "${CMAKE_BINARY_DIR}/deploy/reaper" CACHE PATH "" FORCE)
add_library(Reaper MODULE Probe.cpp)
iplug_configure_reaperext(Reaper Reaper
  WEB_RESOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/web")

add_executable(ResourceTest ResourceTest.cpp)
target_link_libraries(ResourceTest PRIVATE ${CMAKE_DL_LIBS})
