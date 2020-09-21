include_guard(GLOBAL)

#------------------------------------------------------------------------------
# git_update_submodules
# Updates all IPlug2 repository submodules

# function(iplug_git_update_submodules)
#     # Get/Set IPLUG2_UPDATE_GIT_SUBMODULES cache option
#     option(IPLUG2_UPDATE_GIT_SUBMODULES "Automatically update git submodules at generation." OFF)

#     # return if we're not updating submodules
#     if(NOT IPLUG2_UPDATE_GIT_SUBMODULES)
#         return()
#     endif()

#     if(GIT_FOUND AND EXISTS "${IPLUG2_ROOT_PATH}/.git")
#         execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
#                         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
#                         COMMAND_ECHO STDOUT
#                         RESULT_VARIABLE GIT_SUBMOD_RESULT)
#         if(NOT GIT_SUBMOD_RESULT EQUAL "0")
#             message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
#         endif()
#     endif()
# endfunction()


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

    add_executable(${_target})
    _iplug_add_target_lib(${_target} IPlug_APP)
    target_link_libraries(${_target} PRIVATE ${_target}-static)

    if(PLATFORM_WINDOWS AND _arg_SUBSYSTEM STREQUAL "GUI")
        set_target_properties(${_target} PROPERTIES WIN32_EXECUTABLE TRUE)
    endif()

    if(NOT IPLUG2_EXTERNAL_PROJECT)
        set_target_properties(${_target} PROPERTIES FOLDER "Examples/${PROJECT_NAME}")
    endif()
endmacro()


#------------------------------------------------------------------------------
# iplug_add_vst3

macro(iplug_add_vst3 _target)
    _iplug_check_initialized()

    set(_oneValueArgs "EXTENSION" "ICON")
    set(_multiValueArgs "")
    cmake_parse_arguments(_arg "" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})

    if(NOT _arg_EXTENSION)
        set(_arg_EXTENSION "vst3")
    endif()
    if(NOT _arg_ICON)
        set(_arg_ICON "${VST3_SDK_PATH}/doc/artwork/VST_Logo_Steinberg.ico")
    endif()

    add_library(${_target} MODULE)

    set(PLUGIN_BUNDLE_NAME ${PROJECT_NAME}) # TODO: change to bundle name
    set(PLUGIN_NAME ${PROJECT_NAME})        # TODO: change to name from project configuration

    if(NOT HAVESDK_VST3)
        iplug_warning("VST3 Plugins requires a valid path to Steinberg VST3 SDK. '${_target}' will not be able to compile.")
        set_target_properties(${_target} PROPERTIES
            EXCLUDE_FROM_ALL TRUE
            VS_CONFIGURATION_TYPE Utility) # does an equivalent option for the other generators exist?
    else()
        _iplug_add_target_lib(${_target} IPlug_VST3)
        target_link_libraries(${_target} PRIVATE ${_target}-static)

        # Set default application to use when debugging a vst3. Does not override any existing user settings
        set_target_properties(${_target} PROPERTIES VS_DEBUGGER_COMMAND "${VST3_DEFAULT_DEBUG_APPLICATION}")


        # Implements modified smtg_add_vst3plugin_with_pkgname() / smtg_make_plugin_package()
        # Does *not* include compile with debug information in release mode, nor does it set /DEBUG linker option
        set(_path "${VST3_SDK_PATH}/public.sdk/source/main")
        if(MSVC)
            set_target_properties(${_target} PROPERTIES LINK_FLAGS "/DEF:\"${_path}/winexport.def\"")
            set_target_properties(${_target} PROPERTIES PDB_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/PDB")
        elseif(XCODE)
            set_target_properties(${_target} PROPERTIES XCODE_ATTRIBUTE_EXPORTED_SYMBOLS_FILE "${_path}/macexport.exp")

            # modified smtg_setup_universal_binary()
            if(SMTG_BUILD_UNIVERSAL_BINARY)
                if(XCODE_VERSION VERSION_GREATER_EQUAL 12)
                    set_target_properties(${_target} PROPERTIES XCODE_ATTRIBUTE_OSX_ARCHITECTURES "x86_64;arm64;arm64e")
                    set_target_properties(${_target} PROPERTIES XCODE_ATTRIBUTE_ARCHS "$(ARCHS_STANDARD_64_BIT)")
                else()
                    set_target_properties(${_target} PROPERTIES XCODE_ATTRIBUTE_OSX_ARCHITECTURES "x86_64;i386")
                    set_target_properties(${_target} PROPERTIES XCODE_ATTRIBUTE_ARCHS "$(ARCHS_STANDARD_32_64_BIT)")
                endif()
                # diff: adding $<$<CONFIG:Distributed>:NO>
                set_target_properties(${_target} PROPERTIES XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "$<$<CONFIG:Debug>:YES>$<$<CONFIG:Release>:NO>$<$<CONFIG:Distributed>:NO>")
            endif()

            if(SMTG_IOS_DEVELOPMENT_TEAM)
                set_target_properties(${_target} PROPERTIES
                    XCODE_ATTRIBUTE_DEVELOPMENT_TEAM ${SMTG_IOS_DEVELOPMENT_TEAM}
                    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${SMTG_CODE_SIGN_IDENTITY_MAC}"
                )
            endif()
        else()
            set_target_properties(${_target} PROPERTIES LINK_FLAGS "-exported_symbols_list \"${_path}/macexport.exp\"")
        endif()


        set(VST3_CONFIG_PATH $<$<CONFIG:Debug>:VST3-Debug>$<$<CONFIG:Release>:VST3-Release>$<$<CONFIG:Distributed>:VST3-Distributed>)
        set(PLUGIN_PACKAGE_PATH ${PROJECT_BINARY_DIR}/bin/${VST3_CONFIG_PATH}/${PLUGIN_BUNDLE_NAME})

        set_target_properties(${_target} PROPERTIES LIBRARY_OUTPUT_NAME "${PLUGIN_NAME}")

        if(PLATFORM_WINDOWS)
            foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
                string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG_UPPER)
                set_target_properties(${_target} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} "${PLUGIN_PACKAGE_PATH}")
            endforeach()

            if(EXISTS ${_arg_ICON})
                add_custom_command(TARGET ${_target}
                    COMMENT "Copy PlugIn.ico and desktop.ini and change their attributes."
                    POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy
                        ${_arg_ICON}
                        ${PLUGIN_PACKAGE_PATH}/PlugIn.ico
                    COMMAND ${CMAKE_COMMAND} -E copy
                        ${SMTG_DESKTOP_INI_PATH}
                        ${PLUGIN_PACKAGE_PATH}/desktop.ini
                    COMMAND attrib +h ${PLUGIN_PACKAGE_PATH}/desktop.ini
                    COMMAND attrib +h ${PLUGIN_PACKAGE_PATH}/PlugIn.ico
                    COMMAND attrib +s ${PLUGIN_PACKAGE_PATH}
                )
            endif()
        elseif(PLATFORM_APPLE)
            # !!! this is untested code
            set_target_properties(${_target} PROPERTIES BUNDLE TRUE)
            if(XCODE)
                set_target_properties(${_target} PROPERTIES
                    XCODE_ATTRIBUTE_GENERATE_PKGINFO_FILE   YES
                    XCODE_ATTRIBUTE_WRAPPER_EXTENSION       ${_arg_EXTENSION}
                    XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS $<$<CONFIG:Debug>YES>$<$<CONFIG:Release>:NO>$<$<CONFIG:Distributed>:NO>
                    XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT $<$<CONFIG:Debug>:dwarf>$<$<CONFIG:Release>:dwarf-with-dsym>$<$<CONFIG:Distributed>:dwarf-with-dsym>
                )
            else()
                set_target_properties(${target} PROPERTIES
                    BUNDLE_EXTENSION            ${_arg_EXTENSION}
                    LIBRARY_OUTPUT_DIRECTORY    ${PLUGIN_PACKAGE_PATH}
                )
            endif()

        elseif(PLATFORM_LINUX)
            # !!! this is untested code
            smtg_get_linux_architecture_name() # Sets var LINUX_ARCHITECTURE_NAME
            iplug_info("Linux architecture name is ${LINUX_ARCHITECTURE_NAME}.")
            set_target_properties(${_target} PROPERTIES
                PREFIX                   ""
                LIBRARY_OUTPUT_DIRECTORY ${PLUGIN_PACKAGE_PATH}
            )
            if(${CMAKE_BUILD_TYPE} MATCHES "Release|Distributed")
                smtg_strip_symbols(${_target})
            endif()
        endif()


        if(SMTG_RUN_VST_VALIDATOR)
            smtg_run_vst_validator(${_target})
        endif()

        # set(SMTG_CREATE_PLUGIN_LINK ON)
        if(SMTG_CREATE_PLUGIN_LINK)
            if(${SMTG_PLUGIN_TARGET_PATH} STREQUAL "")
                message(FATAL_ERROR "Define a proper value for SMTG_PLUGIN_TARGET_PATH")
            endif()

            set(TARGET_SOURCE ${PLUGIN_PACKAGE_PATH})
            set(TARGET_DESTINATION ${SMTG_PLUGIN_TARGET_PATH}/${PLUGIN_BUNDLE_NAME})

            if(SMTG_WIN)
                file(TO_NATIVE_PATH "${TARGET_DESTINATION}" SRC_NATIVE_PATH)
                file(TO_NATIVE_PATH "${PLUGIN_PACKAGE_PATH}" TARGET_DESTINATION)
                add_custom_command(
                    TARGET ${_target} POST_BUILD
                    COMMAND del "${SRC_NATIVE_PATH}"
                    COMMAND mklink
                        "${SRC_NATIVE_PATH}"
                        "${TARGET_DESTINATION}"
                )
            else()
                add_custom_command(
                    TARGET ${_target} POST_BUILD
                    COMMAND mkdir -p "${TARGET_DESTINATION}"
                    COMMAND ln -svfF "${TARGET_SOURCE}" "${TARGET_DESTINATION}"
                )
            endif()
        endif()

    endif()

    set_target_properties(${_target} PROPERTIES SUFFIX ".${_arg_EXTENSION}")
    if(NOT IPLUG2_EXTERNAL_PROJECT)
        set_target_properties(${_target} PROPERTIES FOLDER "Examples/${PROJECT_NAME}")
    endif()
endmacro()

#------------------------------------------------------------------------------
