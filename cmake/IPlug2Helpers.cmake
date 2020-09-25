include_guard(GLOBAL)

option(IPLUG2_DEVELOPER_CMAKE_DEBUG "Debug output for Iplug2 cmake scripts" OFF)

#------------------------------------------------------------------------------
# helper macros

macro(iplug_set_caller)
    set(_caller ${_caller_override})
    if(NOT _caller)
        set(_caller ${CMAKE_CURRENT_FUNCTION})
    endif()
endmacro()

macro(iplug_syntax_error)
    iplug_set_caller()
    foreach(_str ${ARGN})
        message(STATUS "[ FAIL ]\t<${_caller}> ${_str}")
    endforeach()
    message(FATAL_ERROR "?SYNTAX ERROR\n")
endmacro()

macro(iplug_debug_message)
    if(IPLUG2_DEVELOPER_CMAKE_DEBUG)
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
# iplug_print_list
# debug helper to show content of a list
function(iplug_print_lists)
    cmake_parse_arguments(_arg "" "" "" ${ARGN})
    set(_lists ${_arg_UNPARSED_ARGUMENTS})
    foreach(_list IN ITEMS ${_lists})
        set(index 0)
        foreach(element IN LISTS ${_list})
            message("${_list} [${index}]=\"${element}\"")
            MATH(EXPR index "${index}+1")
        endforeach()
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


#------------------------------------------------------------------------------
# iplug_validate_string(<variable name>)
#   Reads the string value from a variable passed as argument to the function and
#   validates if its conformant to specified options.
#
#   [NOTEMPTY]              Not allowed to be empty.
#   [EXISTS]                Validates that the path or file the string is refering to exists.
#   [ALPHAFIRST]            First character must be an alphabetic character.
#   [INAPOSTROPHES]         Must be surrounded by apostophes.
#   [PREFIX <prefix>]       Prepend <prefix> to the variable name from which the string value is read.
#   [SUFFIX <suffix>]       Append <suffix> to the variable name from which the string value is read.
#   [MINLENGTH <length>]    Minimum length allowed. Ignoring apostophes if [INAPOSTROPHES] is set.
#   [MAXLENGTH <length>]    Maximum length allowed. Ignoring apostophes if [INAPOSTROPHES] is set.
#   [STREQUAL <string>...]  Must be equal to one of the provided strings.
#
#   If any of the following options are set, the function will validate if the string value is allowed
#   to contain specified characters. If none of the options are set, it will skip character testing.
#       [ALPHA]             A-Z (case insensitive)
#       [NUMERIC]           0-9
#       [SPACE]             Spaces
#       [HYPHEN]            -
#       [DOT]               .
#       [SLASH]             /
#       [APOSTROPHE]        '
#       [COMMA]             ,
#       [DELIMITER]         |
#       [UNDERSCORE]        _

function(iplug_validate_string _variable)
    set(_options      NOTEMPTY EXISTS ALPHAFIRST INAPOSTROPHES)
    set(_char_options ALPHA NUMERIC SPACE HYPHEN DOT SLASH APOSTROPHE COMMA DELIMITER UNDERSCORE)
    set(_onevalue     PREFIX SUFFIX MINLENGTH MAXLENGTH)
    set(_multivalue   STREQUAL)
    cmake_parse_arguments(OPTION "${_options};${_char_options}" "${_onevalue}" "${_multivalue}" ${ARGN})

    set(_chars     "")
    set(_esc_chars "")
    set(_var "${_variable}")

    if(OPTION_PREFIX)
        string(PREPEND _var "${OPTION_PREFIX}")
    endif()

    if(OPTION_SUFFIX)
        string(APPEND _var "${OPTION_SUFFIX}")
    endif()

    set(_value "${${_var}}")
    set(_defined "${_variable} = \"${_value}\"")

    set(_noinvalid FALSE)
    foreach(_val IN LISTS _char_options)
        if(OPTION_${_val})
            set(_noinvalid TRUE)
            break()
        endif()
    endforeach()

    if(OPTION_INAPOSTROPHES)
        if(NOT ${_value} MATCHES "^'.*'$")
            iplug_syntax_error("${_defined}. String must be surrounded by apostrophes.")
        endif()
        string(REPLACE "'" "" _value "${_value}")
    endif()

    string(LENGTH "${_value}" _len)

    if(OPTION_STREQUAL)
        foreach(_str IN LISTS OPTION_STREQUAL)
            if(${_value} STREQUAL ${_str})
                return()
            endif()
        endforeach()
        string(REPLACE ";" "\", \"" _str "${OPTION_STREQUAL}")
        iplug_syntax_error("${_defined} is invalid. Valid options are \"${_str}\".")
    endif()

    if(OPTION_NOTEMPTY AND NOT OPTION_MINLENGTH)
        set(OPTION_MINLENGTH 1)
    endif()

    if(OPTION_MINLENGTH AND _len LESS OPTION_MINLENGTH)
        iplug_syntax_error("${_defined}. String length is less than ${OPTION_MINLENGTH} characters.")
    endif()

    if(OPTION_MAXLENGTH AND _len GREATER OPTION_MAXLENGTH)
        iplug_syntax_error("${_defined}. String length exceedes ${OPTION_MAXLENGTH} characters.")
    endif()

    if(OPTION_EXISTS)
        if(NOT _len EQUAL 0)
            get_filename_component(_file "${_value}" ABSOLUTE)
            if(NOT EXISTS ${_file})
                iplug_syntax_error("${_defined}. Path/File does not exist.")
            endif()
        endif()
        return()
    endif()

    if(OPTION_ALPHAFIRST)
        if(${_value} MATCHES "^[^A-Za-z]")
            iplug_syntax_error("${_defined}. String must begin with an alphabetic character.")
        endif()
    endif()

    if(_noinvalid)
        if(OPTION_ALPHA)
            string(APPEND _chars "A-Za-z")
        endif()

        if(OPTION_NUMERIC)
            string(APPEND _chars "0-9")
        endif()

        if(OPTION_SPACE)
            string(APPEND _chars " ")
        endif()

        if(OPTION_SLASH)
            string(APPEND _esc_chars "\\/")
        endif()

        if(OPTION_DOT)
            string(APPEND _esc_chars "\\.")
        endif()

        if(OPTION_HYPHEN)
            string(APPEND _esc_chars "-")
        endif()

        if(OPTION_APOSTROPHE)
            string(APPEND _esc_chars "'")
        endif()

        if(OPTION_COMMA)
            string(APPEND _esc_chars ",")
        endif()

        if(OPTION_DELIMITER)
            string(APPEND _esc_chars "|")
        endif()

        if(OPTION_UNDERSCORE)
            string(APPEND _esc_chars "_")
        endif()

        set(_regex "[^${_chars}\\${_esc_chars}]")
        iplug_debug_message("${_variable}=\"${_value}\" ${_regex}")
        if(${_value} MATCHES "${_regex}")
            iplug_syntax_error("${_defined}. String contains invalid characters.")
        endif()
    endif()
endfunction()
