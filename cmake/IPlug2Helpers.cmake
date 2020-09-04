include_guard(GLOBAL)

option(_DEBUG "Debug output for Iplug2 cmake scripts" OFF)

#------------------------------------------------------------------------------
# helper macros

macro(iplug_set_caller)
    set(_caller ${CMAKE_CURRENT_FUNCTION})
#    if(NOT _caller)
#        set(_caller "CMakeLists.txt")
#    endif()
endmacro()

macro(iplug_syntax_error)
    iplug_set_caller()
    foreach(_str ${ARGN})
        message(STATUS "[ FAIL ]\t<${_caller}> ${_str}")
    endforeach()
    message(FATAL_ERROR "?SYNTAX ERROR\n")
endmacro()

macro(iplug_debug_message)
    if(_DEBUG)
        set(_msg ${ARGN})
        iplug_set_caller()
        message(STATUS "[  OK  ]\t<${_caller}> ${_msg}")
    endif()
endmacro()

macro(iplug_info)
    set(_msg ${ARGN})
    iplug_set_caller()
    message(STATUS "[ INFO ]\t<${_caller}> ${_msg}")
endmacro()

macro(iplug_warning)
    set(_msg ${ARGN})
    iplug_set_caller()
    message(STATUS "[ WARN ]\t<${_caller}> ${_msg}")
endmacro()

#------------------------------------------------------------------------------
# cmake_print_list
# debug helper to show content of a list
function(cmake_print_list _list)
	set(index 0)
	foreach(element ${${_list}})
		message(${_list} "[${index}]=\"${element}\"")
		MATH(EXPR index "${index}+1")
	endforeach()
endfunction()

#------------------------------------------------------------------------------
# string_assert
#
# Aborts script execution with fatal error message if string is empty
#
#     _str - String to test
#     _msg - Message to write if (_str) is empty
#

function(string_assert _str _msg)
	set(SYNTAX_ERROR "${_str}-Guru Meditation")
	if(_str STREQUAL "NOTFOUND" OR SYNTAX_ERROR STREQUAL "-Guru Meditation")
        iplug_syntax_error("${_msg}")
    endif()
    iplug_debug_message("${_str}")
endfunction()


#------------------------------------------------------------------------------
# validate_path
function(validate_path _path _msg)
    if(NOT _path OR NOT IS_DIRECTORY "${_path}" )
        iplug_syntax_error("${_msg}")
    endif()
    iplug_debug_message("${_path}")
endfunction()
