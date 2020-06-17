
set(_faust_names faust.exe faust)
find_program(FAUST_EXECUTABLE
    NAMES faust.exe faust
    PATHS "C:/Program Files/Faust/bin" "C:/Program Files (x86)/Faust/bin"
)
unset(_faust_names)

get_filename_component(_tmp "${FAUST_EXECUTABLE}" DIRECTORY)
get_filename_component(FAUST_DIR "${_tmp}" DIRECTORY CACHE)
set(FAUST_INCLUDE_DIR "${FAUST_DIR}/include" CACHE PATH "Path to Faust include directory")
