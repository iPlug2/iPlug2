
set(_faust_names faust.exe faust)
find_program(Faust_EXECUTABLE
    NAMES faust.exe faust
    PATHS "C:/Program Files/Faust/bin" "C:/Program Files (x86)/Faust/bin"
          "/usr/bin" "/usr/local/bin"
)
unset(_faust_names)

get_filename_component(_tmp "${Faust_EXECUTABLE}" DIRECTORY)
get_filename_component(FAUST_DIR "${_tmp}" DIRECTORY CACHE)
set(FAUST_INCLUDE_DIR "${FAUST_DIR}/include" CACHE PATH "Path to Faust include directory")

function(add_faust_target target)
  set(inc_dir "${PROJECT_BINARY_DIR}/${target}.dir")
  set("${target}_INCLUDE_DIR" ${inc_dir} PARENT_SCOPE)
  set(src_list "")
  set(out_list "")

  # Make sure the output directory exists
  file(MAKE_DIRECTORY "${inc_dir}")

  list(LENGTH ARGN argcnt)
  math(EXPR argcnt "${argcnt} - 1")
  foreach (i1 RANGE 0 ${argcnt} 2)
    math(EXPR i2 "${i1} + 1")
    list(GET ARGN ${i1} dsp_file)
    list(GET ARGN ${i2} class_name)
    set(out_file "${inc_dir}/Faust${class_name}.hpp")
    add_custom_command(
      OUTPUT "${out_file}"
      COMMAND "${Faust_EXECUTABLE}" -lang cpp -cn "${class_name}" -a "${IPLUG2_DIR}/IPlug/Extras/Faust/IPlugFaust_arch.cpp" -o "${out_file}" "${dsp_file}"
      DEPENDS "${dsp_file}"
    )
    list(APPEND src_list "${dsp_file}")
    list(APPEND out_list "${out_file}")
  endforeach()
  add_custom_target(${target} ALL DEPENDS ${out_list} SOURCES ${src_list})
endfunction()
