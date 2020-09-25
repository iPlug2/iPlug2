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
    set(_caller_override iplug_configure_project)

    string_assert(${PROJECT_NAME} "PROJECT_NAME is empty. Make sure to call configure after project declarations.")

    # TODO: refactor definitions
    set(_oneValueArgs

        "BUNDLE_NAME"
        "BUNDLE_DOMAIN"

        "PLUG_NAME"
        "PLUG_NAME_ABBREVIATION"
        "PLUG_MFR"
        "PLUG_VERSION_HEX"
        "PLUG_VERSION_STR"
        "PLUG_UNIQUE_ID"
        "PLUG_MFR_ID"
        "PLUG_URL_STR"
        "PLUG_EMAIL_STR"
        "PLUG_COPYRIGHT_STR"
        "PLUG_CHANNEL_IO"
        "PLUG_LATENCY"
        "PLUG_DOES_MIDI_IN"
        "PLUG_DOES_MIDI_OUT"
        "PLUG_DOES_MPE"
        "PLUG_DOES_STATE_CHUNKS"
        "PLUG_HAS_UI"
        "PLUG_WIDTH"
        "PLUG_HEIGHT"
        "PLUG_FPS"
        "PLUG_SHARED_RESOURCES"
        "PLUG_TYPE"
        "PLUG_HOST_RESIZE"
        "PLUG_ICON"

        "SHARED_RESOURCES_SUBPATH"
        "VST3_SUBCATEGORY"

        "APP_NUM_CHANNELS"
        "APP_N_VECTOR_WAIT"
        "APP_COPY_AUV3"
        "APP_SIGNAL_VECTOR_SIZE"

        "AAX_TYPE_IDS"
        "AAX_TYPE_IDS_AUDIOSUITE"
        "AAX_PLUG_CATEGORY_STR"
        "AAX_DOES_AUDIOSUITE"

        "AUV2_ENTRY"
        "AUV2_FACTORY"
        "AUV2_VIEW_CLASS"
    )

    set(_multiValueArgs "RESOURCE_DEFINITIONS")
    cmake_parse_arguments(_arg "" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})

    # surrounds string variables with needed quotation marks
    foreach(_var IN LISTS _oneValueArgs)
        _iplug_add_config_variable(${_var} "${_arg_${_var}}")
    endforeach()

    # resource definitions are always strings
    if(_arg_RESOURCE_DEFINITIONS)
        while(TRUE)
            list(LENGTH _arg_RESOURCE_DEFINITIONS _len)
            if(NOT _len OR ${_len} EQUAL 0)
                break()
            endif()
            list(POP_FRONT _arg_RESOURCE_DEFINITIONS _item_name)
            list(POP_FRONT _arg_RESOURCE_DEFINITIONS _item_val)
            _iplug_add_config_variable(${_item_name} "\"${_item_val}\"")
        endwhile()
    endif()

    # additional variables that are based on existing variables
    _iplug_add_config_variable(PLUG_CLASS_NAME     "${PROJECT_NAME}")
    # _iplug_add_config_variable(PLUG_VERSION_HEX    "")  # TODO: generate information from PLUG_VERSION_STR
    _iplug_add_config_variable(BUNDLE_MFR          "${CONFIG_PLUG_MFR}")
    _iplug_add_config_variable(AAX_PLUG_MFR_STR    "${CONFIG_PLUG_MFR}")
    _iplug_add_config_variable(AUV2_ENTRY_STR      "${CONFIG_AUV2_ENTRY}")
    _iplug_add_config_variable(AUV2_VIEW_CLASS_STR "${CONFIG_AUV2_VIEW_CLASS}")
    _iplug_add_config_variable(AAX_PLUG_NAME_STR   "${CONFIG_PLUG_CLASS_NAME}\\n${CONFIG_PLUG_NAME_ABBREVIATION}")

    _iplug_add_config_variable(APP_MULT            1)  # APP_MULT should probably be removed

    # check for unknown configuration variables and provide a warning if found
    if(_arg_UNPARSED_ARGUMENTS)
        set(_arg ${_arg_UNPARSED_ARGUMENTS})
        while(TRUE)
            list(LENGTH _arg _len)
            if(NOT _len OR ${_len} EQUAL 0)
                break()
            endif()
            list(POP_FRONT _arg _item_name)
            list(POP_FRONT _arg _item_val)
            iplug_warning("Unknown configuration ${_item_name} = \"${_item_val}\".")
        endwhile()
    endif()

    # Check validity of config variables.
    iplug_validate_string(PLUG_NAME                PREFIX CONFIG_ NOTEMPTY ALPHA NUMERIC SPACE)
    iplug_validate_string(PLUG_NAME_ABBREVIATION   PREFIX CONFIG_ NOTEMPTY ALPHA NUMERIC MAXLENGTH 4)
    iplug_validate_string(PLUG_MFR                 PREFIX CONFIG_ NOTEMPTY MAXLENGTH 127)
    iplug_validate_string(PLUG_VERSION_STR         PREFIX CONFIG_ NOTEMPTY NUMERIC VERSION 3)
    iplug_validate_string(PLUG_URL_STR             PREFIX CONFIG_ MAXLENGTH 255)
    iplug_validate_string(PLUG_EMAIL_STR           PREFIX CONFIG_ MAXLENGTH 127)
    iplug_validate_string(PLUG_COPYRIGHT_STR       PREFIX CONFIG_ MAXLENGTH 127)
    iplug_validate_string(PLUG_UNIQUE_ID           PREFIX CONFIG_ NOTEMPTY SINGLE_QUOTED ALPHA NUMERIC MAXLENGTH 4)
    iplug_validate_string(PLUG_MFR_ID              PREFIX CONFIG_ NOTEMPTY SINGLE_QUOTED ALPHA NUMERIC MAXLENGTH 4)
    iplug_validate_string(PLUG_CHANNEL_IO          PREFIX CONFIG_ NOTEMPTY NUMERIC HYPHEN SPACE)
    iplug_validate_string(PLUG_LATENCY             PREFIX CONFIG_ NUMERIC)
    iplug_validate_string(PLUG_DOES_MIDI_IN        PREFIX CONFIG_ STREQUAL 0 1)
    iplug_validate_string(PLUG_DOES_MIDI_OUT       PREFIX CONFIG_ STREQUAL 0 1)
    iplug_validate_string(PLUG_DOES_MPE            PREFIX CONFIG_ STREQUAL 0 1)
    iplug_validate_string(PLUG_DOES_STATE_CHUNKS   PREFIX CONFIG_ STREQUAL 0 1)
    iplug_validate_string(PLUG_HAS_UI              PREFIX CONFIG_ STREQUAL 0 1)
    iplug_validate_string(PLUG_WIDTH               PREFIX CONFIG_ NUMERIC)
    iplug_validate_string(PLUG_PLUG_HEIGHT         PREFIX CONFIG_ NUMERIC)
    iplug_validate_string(PLUG_HOST_RESIZE         PREFIX CONFIG_ NOTEMPTY STREQUAL 0 1)
    iplug_validate_string(PLUG_FPS                 PREFIX CONFIG_ NUMERIC)
    iplug_validate_string(PLUG_SHARED_RESOURCES    PREFIX CONFIG_ NUMERIC)
    iplug_validate_string(PLUG_TYPE                PREFIX CONFIG_ NOTEMPTY STREQUAL Effect Instrument MIDIEffect)
    iplug_validate_string(PLUG_ICON                PREFIX CONFIG_ FILE_EXISTS)
    iplug_validate_string(BUNDLE_DOMAIN            PREFIX CONFIG_ NOTEMPTY ALPHAFIRST ALPHA NUMERIC HYPHEN)
    iplug_validate_string(BUNDLE_NAME              PREFIX CONFIG_ NOTEMPTY ALPHAFIRST ALPHA NUMERIC HYPHEN)
    iplug_validate_string(SHARED_RESOURCES_SUBPATH PREFIX CONFIG_ PATH_EXISTS)

    # TODO: Move to iplug_add_application
    iplug_validate_string(APP_NUM_CHANNELS         PREFIX CONFIG_ NOTEMPTY NUMERIC)
    iplug_validate_string(APP_N_VECTOR_WAIT        PREFIX CONFIG_ NOTEMPTY NUMERIC)
    iplug_validate_string(APP_COPY_AUV3            PREFIX CONFIG_ NOTEMPTY STREQUAL 0 1)
    iplug_validate_string(APP_SIGNAL_VECTOR_SIZE   PREFIX CONFIG_ NOTEMPTY NUMERIC MAXLENGTH 6)

    # TODO: Move to iplug_add_aax
    iplug_validate_string(AAX_TYPE_IDS             PREFIX CONFIG_ NOTEMPTY ALPHA NUMERIC SPACE APOSTROPHE COMMA)
    iplug_validate_string(AAX_TYPE_IDS_AUDIOSUITE  PREFIX CONFIG_ NOTEMPTY ALPHA NUMERIC SPACE APOSTROPHE COMMA)
    iplug_validate_string(AAX_DOES_AUDIOSUITE      PREFIX CONFIG_ NOTEMPTY STREQUAL 0 1)
    iplug_validate_string(AAX_PLUG_CATEGORY_STR    PREFIX CONFIG_ NOTEMPTY STREQUAL None EQ Dynamics PitchShift Reverb Delay Modulation Harmonic NoiseReduction Dither SoundField Effect)

    # TODO: Move to iplug_add_vst3
    iplug_validate_string(VST3_SUBCATEGORY         PREFIX CONFIG_ ALPHA DELIMITER MAXLENGTH 127)

    # TODO: Move to iplug_add_auv2
    iplug_validate_string(AUV2_ENTRY               PREFIX CONFIG_ ALPHAFIRST ALPHA NUMERIC UNDERSCORE)
    iplug_validate_string(AUV2_VIEW_CLASS          PREFIX CONFIG_ ALPHAFIRST ALPHA NUMERIC UNDERSCORE)
    iplug_validate_string(AUV2_FACTORY             PREFIX CONFIG_ ALPHAFIRST ALPHA NUMERIC UNDERSCORE)

    set(_caller_override "")

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

    set_target_properties(${_target}
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin/$<$<CONFIG:$<CONFIG>>:APP-$<CONFIG>>")

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

    get_filename_component(VST3_ICON "${CONFIG_PLUG_ICON}" ABSOLUTE)
    if(NOT VST3_ICON OR NOT EXISTS ${VST3_ICON})
        set(VST3_ICON "${VST3_SDK_PATH}/doc/artwork/VST_Logo_Steinberg.ico")
    endif()

    add_library(${_target} MODULE)



    if(NOT HAVESDK_VST3)
        iplug_warning("VST3 Plugins requires a valid path to Steinberg VST3 SDK. '${_target}' will not be able to compile.")
        set_target_properties(${_target} PROPERTIES
            EXCLUDE_FROM_ALL TRUE
            VS_CONFIGURATION_TYPE Utility) # does an equivalent option for the other generators exist?
    else()
        _iplug_add_target_lib(${_target} IPlug_VST3)
        target_link_libraries(${_target} PRIVATE ${_target}-static)


        set(PLUGIN_NAME ${CONFIG_PLUG_NAME})
        set(PLUGIN_EXT ${_arg_EXTENSION})
        set(PLUGIN_NAME_EXT "${PLUGIN_NAME}.${PLUGIN_EXT}")
        set(VST3_CONFIG_PATH $<$<CONFIG:$<CONFIG>>:VST3-$<CONFIG>>)
        set(PLUGIN_PACKAGE_PATH ${PROJECT_BINARY_DIR}/bin/${VST3_CONFIG_PATH}/${CONFIG_BUNDLE_NAME})
        set(_path "${VST3_SDK_PATH}/public.sdk/source/main")

        set_target_properties(${_target} PROPERTIES
            LIBRARY_OUTPUT_NAME                            ${PLUGIN_NAME}
            SUFFIX                                         .${PLUGIN_EXT}
            LIBRARY_OUTPUT_DIRECTORY                       ${PLUGIN_PACKAGE_PATH}
            PDB_OUTPUT_DIRECTORY                           ${PROJECT_BINARY_DIR}/PDB/${VST3_CONFIG_PATH}
            VS_DEBUGGER_COMMAND                            ${VST3_DEFAULT_DEBUG_APPLICATION}
            BUNDLE                                         TRUE
            BUNDLE_EXTENSION                               ${PLUGIN_EXT}
            XCODE_ATTRIBUTE_WRAPPER_EXTENSION              ${PLUGIN_EXT}
            XCODE_ATTRIBUTE_GENERATE_PKGINFO_FILE          YES
            XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS $<$<CONFIG:Debug>YES>$<$<CONFIG:Release>:NO>$<$<CONFIG:Distributed>:NO>
            XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT       $<$<CONFIG:Debug>:dwarf>
        )

        # Implements modified smtg_add_vst3plugin_with_pkgname() / smtg_make_plugin_package()
        # Does *not* include compile with debug information in release mode, nor does it set /DEBUG linker option
        if(MSVC)
            set_target_properties(${_target} PROPERTIES LINK_FLAGS "/DEF:\"${_path}/winexport.def\"")
        elseif(XCODE)
            set_target_properties(${_target} PROPERTIES XCODE_ATTRIBUTE_EXPORTED_SYMBOLS_FILE "${_path}/macexport.exp")
        else()
            set_target_properties(${_target} PROPERTIES LINK_FLAGS "-exported_symbols_list \"${_path}/macexport.exp\"")
        endif()

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
                XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${SMTG_CODE_SIGN_IDENTITY_MAC}")
        endif()

        if(PLATFORM_WINDOWS)
            if(EXISTS ${VST3_ICON})
                add_custom_command(TARGET ${_target}
                    COMMENT "Copy PlugIn.ico and desktop.ini and change their attributes."
                    POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy
                        ${VST3_ICON}
                        ${PLUGIN_PACKAGE_PATH}/PlugIn.ico
                    COMMAND ${CMAKE_COMMAND} -E copy
                        ${CMAKE_BINARY_DIR}/desktop.ini  #${SMTG_DESKTOP_INI_PATH}
                        ${PLUGIN_PACKAGE_PATH}/desktop.ini
                    COMMAND attrib +h ${PLUGIN_PACKAGE_PATH}/desktop.ini
                    COMMAND attrib +h ${PLUGIN_PACKAGE_PATH}/PlugIn.ico
                    COMMAND attrib +s ${PLUGIN_PACKAGE_PATH}
                )
            endif()

        elseif(PLATFORM_LINUX)
            # !!! this is untested code
            smtg_get_linux_architecture_name() # Sets var LINUX_ARCHITECTURE_NAME
            set_target_properties(${_target} PROPERTIES PREFIX "")
            if(${CMAKE_BUILD_TYPE} MATCHES "Release|Distributed")
                smtg_strip_symbols(${_target})
            endif()
        endif()

        if(SMTG_RUN_VST_VALIDATOR)
            smtg_run_vst_validator(${_target})
        endif()

        if(SMTG_CREATE_PLUGIN_LINK)
            if(${SMTG_PLUGIN_TARGET_PATH} STREQUAL "")
                message(FATAL_ERROR "Define a proper value for SMTG_PLUGIN_TARGET_PATH")
            endif()

            set(TARGET_SOURCE ${PLUGIN_PACKAGE_PATH})
            set(TARGET_DESTINATION ${SMTG_PLUGIN_TARGET_PATH}/${CONFIG_BUNDLE_NAME})

            if(SMTG_WIN)
                file(TO_NATIVE_PATH "${TARGET_SOURCE}" SOURCE_NATIVE_PATH)
                file(TO_NATIVE_PATH "${TARGET_DESTINATION}" DESTINATION_NATIVE_PATH)
                # file(TO_NATIVE_PATH "${TARGET_SOURCE}/${PLUGIN_NAME_EXT}" SOURCE_PLUGIN_NATIVE)
                # file(TO_NATIVE_PATH "${TARGET_DESTINATION}/${PLUGIN_NAME_EXT}" DESTINATION_PLUGIN_NATIVE)
                add_custom_command(
                    TARGET ${_target} POST_BUILD
                    COMMAND rmdir "${DESTINATION_NATIVE_PATH}"
                    COMMAND mklink /D
                        "${DESTINATION_NATIVE_PATH}"
                        "${SOURCE_NATIVE_PATH}"
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


    if(NOT IPLUG2_EXTERNAL_PROJECT)
        set_target_properties(${_target} PROPERTIES FOLDER "Examples/${PROJECT_NAME}")
    endif()
endmacro()

#------------------------------------------------------------------------------
