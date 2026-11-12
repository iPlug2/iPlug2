cmake_minimum_required(VERSION 3.14)

if(NOT IS_ABSOLUTE "${TEST_BINARY_DIR}" OR
   NOT EXISTS "${IPLUG2_DIR}/Tests/Linux/Deployment/CMakeLists.txt")
  message(FATAL_ERROR "Pass an absolute TEST_BINARY_DIR and a valid IPLUG2_DIR.")
endif()

if(NOT TEST_DEPLOY_METHOD)
  set(TEST_DEPLOY_METHOD COPY)
endif()
if(NOT TEST_GENERATOR)
  set(TEST_GENERATOR "Unix Makefiles")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}"
  -G "${TEST_GENERATOR}"
  -S "${IPLUG2_DIR}/Tests/Linux/Deployment" -B "${TEST_BINARY_DIR}"
  "-DIPLUG2_DIR=${IPLUG2_DIR}" "-DTEST_DEPLOY_METHOD=${TEST_DEPLOY_METHOD}"
  RESULT_VARIABLE result)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "Deployment fixture configuration failed")
endif()

foreach(target Direct Deferred High-clap)
  file(REMOVE "${TEST_BINARY_DIR}/deploy/${target}.clap")
endforeach()
file(REMOVE "${TEST_BINARY_DIR}/deploy/High.clap"
  "${TEST_BINARY_DIR}/deploy/resources/txt/late.txt"
  "${TEST_BINARY_DIR}/out/resources/txt/late.txt"
  "${TEST_BINARY_DIR}/out/Deferred.clap.resources/txt/late.txt"
  "${TEST_BINARY_DIR}/deploy/Deferred.clap.resources/txt/late.txt"
  "${TEST_BINARY_DIR}/out/High.clap.resources/txt/late.txt"
  "${TEST_BINARY_DIR}/deploy/High.clap.resources/txt/late.txt"
  "${TEST_BINARY_DIR}/deploy/reaper/reaper_Reaper.so"
  "${TEST_BINARY_DIR}/deploy/reaper/Reaper/index.html")
execute_process(COMMAND "${CMAKE_COMMAND}" --build "${TEST_BINARY_DIR}"
  --clean-first --target Direct Deferred High-clap Alpha-clap Bundled Reaper ResourceTest
  RESULT_VARIABLE result)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "Deployment fixture build failed")
endif()
foreach(name Direct Deferred High)
  if(NOT EXISTS "${TEST_BINARY_DIR}/deploy/${name}.clap")
    message(FATAL_ERROR "${name}: configured target was not automatically deployed")
  endif()
endforeach()
if(TEST_DEPLOY_METHOD STREQUAL "COPY")
  file(READ "${TEST_BINARY_DIR}/deploy/Deferred.clap.resources/txt/late.txt" deployed)
  if(NOT deployed STREQUAL "deployment resource\n")
    message(FATAL_ERROR "Resources added after configuration were not deployed")
  endif()
endif()
function(check_resource module expected)
  foreach(location out deploy)
    execute_process(COMMAND "${TEST_BINARY_DIR}/ResourceTest"
      "${TEST_BINARY_DIR}/${location}/${module}" "${expected}" RESULT_VARIABLE result)
    if(NOT result EQUAL 0)
      message(FATAL_ERROR "${TEST_DEPLOY_METHOD} ${location}/${module}: resource lookup mismatch")
    endif()
  endforeach()
endfunction()

check_resource(Deferred.clap "deployment resource\n")
check_resource(Alpha.clap "alpha resource\n")
check_resource("Bundled.vst3/Contents/test-linux/Bundled.so" "bundle resource\n")
# Mutate build-local inputs, leaving the repository fixtures untouched. Make's
# timestamp resolution requires the edit to follow the first link by a second.
execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 1)
file(WRITE "${TEST_BINARY_DIR}/Mutable/late.txt" "updated flat resource\n")
file(WRITE "${TEST_BINARY_DIR}/Bundled/late.txt" "updated bundle resource\n")
execute_process(COMMAND "${CMAKE_COMMAND}" --build "${TEST_BINARY_DIR}" --target Deferred Bundled
  RESULT_VARIABLE result)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "Resource-only incremental build failed")
endif()
check_resource(Deferred.clap "updated flat resource\n")
check_resource("Bundled.vst3/Contents/test-linux/Bundled.so" "updated bundle resource\n")
execute_process(COMMAND "${CMAKE_COMMAND}" --build "${TEST_BINARY_DIR}" --target Beta-clap
  RESULT_VARIABLE result)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "Second colliding-resource fixture build failed")
endif()
check_resource(Alpha.clap "alpha resource\n")
check_resource(Beta.clap "beta resource\n")

set(alpha_resources "${TEST_BINARY_DIR}/out/Alpha.clap.resources/txt/late.txt")
if(TEST_DEPLOY_METHOD STREQUAL "COPY")
  list(APPEND alpha_resources "${TEST_BINARY_DIR}/deploy/Alpha.clap.resources/txt/late.txt")
endif()
foreach(resource IN LISTS alpha_resources)
  file(RENAME "${resource}" "${resource}.saved")
endforeach()
set(decoys
  "${TEST_BINARY_DIR}/out/resources/txt/late.txt"
  "${TEST_BINARY_DIR}/resources/txt/late.txt"
  "${TEST_BINARY_DIR}/ResourceTest.resources/txt/late.txt")
foreach(decoy IN LISTS decoys)
  get_filename_component(directory "${decoy}" DIRECTORY)
  file(MAKE_DIRECTORY "${directory}")
  file(WRITE "${decoy}" "unrelated resource\n")
endforeach()
check_resource(Alpha.clap --missing)
check_resource(Beta.clap "beta resource\n")
foreach(resource IN LISTS alpha_resources)
  file(RENAME "${resource}.saved" "${resource}")
endforeach()
file(REMOVE ${decoys})
check_resource(Alpha.clap "alpha resource\n")

if(NOT EXISTS "${TEST_BINARY_DIR}/out/reaper_Reaper.so" OR
   NOT EXISTS "${TEST_BINARY_DIR}/deploy/reaper/reaper_Reaper.so")
  message(FATAL_ERROR "Linux REAPER extension output and deployment names do not match")
endif()
file(READ "${TEST_BINARY_DIR}/deploy/reaper/Reaper/index.html" deployed_web)
if(NOT deployed_web STREQUAL "<p>REAPER deployment test</p>\n")
  message(FATAL_ERROR "REAPER extension web resources were not deployed")
endif()
