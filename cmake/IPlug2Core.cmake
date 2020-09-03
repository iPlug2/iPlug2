include_guard(GLOBAL)

#------------------------------------------------------------------------------
# git_update_submodules
# Updates all IPlug2 repository submodules

function(iplug_git_update_submodules)
	# Get/Set IPLUG2_UPDATE_GIT_SUBMODULES cache option
	option(IPLUG2_UPDATE_GIT_SUBMODULES "Automatically update git submodules at generation." OFF)

	# return if we're not updating submodules
	if(NOT IPLUG2_UPDATE_GIT_SUBMODULES)
		return()
	endif()

	if(GIT_FOUND AND EXISTS "${IPLUG2_ROOT_PATH}/.git")
		execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
						WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
						COMMAND_ECHO STDOUT
						RESULT_VARIABLE GIT_SUBMOD_RESULT)
		if(NOT GIT_SUBMOD_RESULT EQUAL "0")
			message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
		endif()
	endif()
endfunction()


#------------------------------------------------------------------------------

function(iplug_get_version _file _outvar)
    # Get IPlug2 verion number from /IPlug/IPlugVersion.h
    # This will make sure that the version number is correct in the source
    # without regenerating every time it changes

    # TODO: change to "^#define IPLUG_VERSION[ \t]+([0-9]+)\\.([0-9]+)\\.([0-9]+)$" to get
    # x.x.x matches to \\1 \\2 \\3 instead of 3 different defines for major, minor and patch

    set(_version_regex "#define IPLUG_VERSION_.*[ \t]+(.+)")
    file(STRINGS "${_file}" _iplug_version REGEX ${_version_regex})
    list(TRANSFORM _iplug_version REPLACE ${_version_regex} "\\1")
    string(JOIN "." _iplug_version ${_iplug_version})
    set(${_outvar} ${_iplug_version} PARENT_SCOPE)
endfunction()


#------------------------------------------------------------------------------
# iplug_configure_project

macro(iplug_configure_project)
    _iplug_check_initialized()

    string_assert(${PROJECT_NAME} "PROJECT_NAME is empty. Make sure to call configure after project declarations.")

    # IPlug2_BINARY_DIR is declared in IPlug2/CMakeLists.txt.
    # If we're adding IPlug with add_subdirectory it never gets declared since we're using
    # a external project as superproject and hitchhiking into that binary directory instead.
    if(NOT IPlug2_BINARY_DIR)
        set(IPLUG2_EXTERNAL_PROJECT TRUE)
        add_subdirectory("${IPLUG2_ROOT_PATH}" "${CMAKE_CURRENT_BINARY_DIR}/IPlug2")
    endif()

    _iplug_generate_source_groups()
endmacro()


#------------------------------------------------------------------------------
# iplug_add_aax

macro(iplug_add_aax _target)
    iplug_syntax_error("iplug_add_aax() is not implemented.")
endmacro()

#------------------------------------------------------------------------------
# iplug_add_au

macro(iplug_add_au _target)
    iplug_syntax_error("iplug_add_au() is not implemented.")
endmacro()

#------------------------------------------------------------------------------
# iplug_add_auv3

macro(iplug_add_auv3 _target)
    iplug_syntax_error("iplug_add_auv3() is not implemented.")
endmacro()

#------------------------------------------------------------------------------
# iplug_add_web

macro(iplug_add_web _target)
    iplug_syntax_error("iplug_add_web() is not implemented.")
endmacro()

#------------------------------------------------------------------------------
# iplug_add_vst3c

macro(iplug_add_vst3c _target)
    iplug_syntax_error("iplug_add_vst3c() is not implemented.")
endmacro()

#------------------------------------------------------------------------------
# iplug_add_vst3p

macro(iplug_add_vst3p _target)
    iplug_syntax_error("iplug_add_vst3p() is not implemented.")
endmacro()

#------------------------------------------------------------------------------
# iplug_add_vst2

macro(iplug_add_vst2 _target)
    iplug_syntax_error("iplug_add_vst2() is not implemented.")
endmacro()


#------------------------------------------------------------------------------
# iplug_add_application

macro(iplug_add_application _target)
    _iplug_check_initialized()

    set(_oneValueArgs "SUBSYSTEM")
    set(_multiValueArgs "")
    cmake_parse_arguments(_arg "" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})

    if(NOT _arg_SUBSYSTEM)
        set(_arg_SUBSYSTEM "GUI")
    endif()

    string(TOUPPER ${_arg_SUBSYSTEM} _arg_SUBSYSTEM )
    if(NOT _arg_SUBSYSTEM STREQUAL "GUI" OR NOT STREQUAL "CONSOLE")
        iplug_syntax_error("Unknown subsystem \"${_arg_SUBSYSTEM}\". Valid options are 'GUI' or 'Console'")
    endif()

    _iplug_add_target_lib(${_target} IPlug_APP)
    add_executable(${_target})
    target_link_libraries(${_target} PUBLIC ${_target}-static)

    if(PLATFORM_WINDOWS AND _arg_SUBSYSTEM STREQUAL "GUI")
        set_property(TARGET ${_target} PROPERTY WIN32_EXECUTABLE TRUE)
    endif()

    if(NOT IPLUG2_EXTERNAL_PROJECT)
        set_target_properties(${_target} PROPERTIES FOLDER "Examples/${PROJECT_NAME}")
    endif()
endmacro()



#------------------------------------------------------------------------------
# iplug_add_vst3

macro(iplug_add_vst3 _target)
    _iplug_check_initialized()

    set(_oneValueArgs "EXTENSION")
    set(_multiValueArgs "")
    cmake_parse_arguments(_arg "" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})

    if(NOT _arg_EXTENSION)
        set(_arg_EXTENSION "vst3")
    endif()

    if(NOT HAVESDK_VST3)
        iplug_warning("VST3 Plugins requires Steinberg VST3 SDK to be installed.")
    endif()

    _iplug_add_target_lib(${_target} IPlug_VST3)
    add_library(${_target} SHARED)
    target_link_libraries(${_target} PUBLIC ${_target}-static IVST3SDK) # IVST3SDK is needed for vst3 export

    set_target_properties(${_target} PROPERTIES SUFFIX ".${_arg_EXTENSION}")

    if(NOT IPLUG2_EXTERNAL_PROJECT)
        set_target_properties(${_target} PROPERTIES FOLDER "Examples/${PROJECT_NAME}")
    endif()

endmacro()

#------------------------------------------------------------------------------
