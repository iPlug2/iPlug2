#  ==============================================================================
#
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

# AAX target configuration for iPlug2

include(${CMAKE_CURRENT_LIST_DIR}/IPlug.cmake)

if(NOT TARGET iPlug2::AAX)
  # Define SDK path (can be overridden via cache for FetchContent usage)
  set(IPLUG2_AAX_SDK_PATH "${IPLUG_DEPS_DIR}/AAX_SDK" CACHE PATH "Path to AAX SDK")
  set(AAX_SDK_DIR ${IPLUG2_AAX_SDK_PATH})

  # Check if AAX SDK exists (must have CMakeLists.txt, not just placeholder directory)
  if(NOT EXISTS ${AAX_SDK_DIR}/CMakeLists.txt)
    message(STATUS "AAX SDK not found at ${AAX_SDK_DIR}. AAX targets will not be available.")
    set(IPLUG2_AAX_SUPPORTED FALSE CACHE INTERNAL "AAX SDK available")
    # Create a dummy iPlug2::AAX target so projects can link to it without errors
    add_library(iPlug2::AAX INTERFACE IMPORTED)
    # Define stub function that excludes the target from default build
    function(iplug_configure_aax target project_name)
      message(STATUS "Skipping AAX target '${target}' - AAX SDK not available")
      set_target_properties(${target} PROPERTIES EXCLUDE_FROM_ALL TRUE)
    endfunction()
    return()
  endif()

  set(IPLUG2_AAX_SUPPORTED TRUE CACHE INTERNAL "AAX SDK available")

  # Build AAX SDK library via add_subdirectory (only once)
  # Use global property to prevent race condition in parallel builds
  get_property(_aax_sdk_added GLOBAL PROPERTY _IPLUG2_AAX_SDK_ADDED)
  if(NOT _aax_sdk_added AND NOT TARGET AAX_SDK::AAXLibrary)
    set_property(GLOBAL PROPERTY _IPLUG2_AAX_SDK_ADDED TRUE)
    # Disable examples and other optional components for faster builds
    set(AAX_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(AAX_BUILD_PTSL_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(AAX_BUILD_JUCE_GUI_EXTENSION OFF CACHE BOOL "" FORCE)
    set(AAX_BUILD_TI_INTERFACE OFF CACHE BOOL "" FORCE)
    # Force static runtime (MT) for AAX SDK on Windows
    # The AAX SDK's cmake_minimum_required(3.13) uses CMP0091 OLD policy, which ignores
    # CMAKE_MSVC_RUNTIME_LIBRARY and instead uses the default /MD flags from CMAKE_CXX_FLAGS.
    # We must temporarily replace /MD with /MT in the default flags before add_subdirectory.
    if(MSVC)
      set(_saved_cxx_flags_release "${CMAKE_CXX_FLAGS_RELEASE}")
      set(_saved_cxx_flags_debug "${CMAKE_CXX_FLAGS_DEBUG}")
      set(_saved_cxx_flags_relwithdebinfo "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
      set(_saved_cxx_flags_minsizerel "${CMAKE_CXX_FLAGS_MINSIZEREL}")
      string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
      string(REPLACE "/MDd" "/MTd" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
      string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
      string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL}")
    endif()
    add_subdirectory(${AAX_SDK_DIR} ${CMAKE_BINARY_DIR}/AAX_SDK EXCLUDE_FROM_ALL)
    # Restore original flags for any subsequent targets
    if(MSVC)
      set(CMAKE_CXX_FLAGS_RELEASE "${_saved_cxx_flags_release}")
      set(CMAKE_CXX_FLAGS_DEBUG "${_saved_cxx_flags_debug}")
      set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${_saved_cxx_flags_relwithdebinfo}")
      set(CMAKE_CXX_FLAGS_MINSIZEREL "${_saved_cxx_flags_minsizerel}")
    endif()
  endif()

  add_library(iPlug2::AAX INTERFACE IMPORTED)

  # iPlug2 AAX wrapper sources
  set(AAX_IPLUG_SRC
    ${IPLUG_DIR}/AAX/IPlugAAX.cpp
    ${IPLUG_DIR}/AAX/IPlugAAX_Describe.cpp
    ${IPLUG_DIR}/AAX/IPlugAAX_Parameters.cpp
  )

  target_sources(iPlug2::AAX INTERFACE ${AAX_IPLUG_SRC})

  target_include_directories(iPlug2::AAX INTERFACE
    ${IPLUG_DIR}/AAX
    ${AAX_SDK_DIR}/Interfaces
    ${AAX_SDK_DIR}/Interfaces/ACF
    ${AAX_SDK_DIR}/Libs/AAXLibrary/include
  )

  target_compile_definitions(iPlug2::AAX INTERFACE
    AAX_API
    IPLUG_EDITOR=1
    IPLUG_DSP=1
    IPLUG2_CMAKE_BUILD  # Disables #pragma comment(lib) in IPlugAAX.h - CMake handles linking
  )

  # Suppress AAX SDK related warnings
  target_compile_options(iPlug2::AAX INTERFACE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wno-multichar>
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wno-incompatible-ms-struct>
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wno-pragma-pack>
  )

  if(WIN32)
    # Windows: link against the static library and object library
    target_link_libraries(iPlug2::AAX INTERFACE
      AAX_SDK::AAXLibrary
      AAX_SDK::AAX_Export
    )
  elseif(APPLE)
    # macOS: Set Obj-C++ for iPlug2 AAX wrapper
    set_source_files_properties(${AAX_IPLUG_SRC} PROPERTIES LANGUAGE OBJCXX)

    target_link_libraries(iPlug2::AAX INTERFACE
      AAX_SDK::AAXLibrary
      AAX_SDK::AAX_Export
      "-framework Cocoa"
      "-framework CoreFoundation"
    )
  endif()

  target_link_libraries(iPlug2::AAX INTERFACE iPlug2::IPlug)
endif()

# Configuration function for AAX targets
function(iplug_configure_aax target project_name)
  target_link_libraries(${target} PUBLIC iPlug2::AAX)

  if(WIN32)
    # Windows: AAX bundle structure is ProjectName.aaxplugin/Contents/x64/ProjectName.aaxplugin
    # Determine architecture (64-bit only)
    if(CMAKE_GENERATOR_PLATFORM STREQUAL "ARM64EC")
      set(AAX_ARCH "ARM64EC")
    elseif(CMAKE_GENERATOR_PLATFORM STREQUAL "ARM64")
      set(AAX_ARCH "ARM64")
    else()
      set(AAX_ARCH "x64")
    endif()

    set(AAX_OUTPUT_DIR "${IPLUG2_OUTPUT_DIR}/${project_name}.aaxplugin/Contents/${AAX_ARCH}")

    set_target_properties(${target} PROPERTIES
      OUTPUT_NAME "${project_name}"
      SUFFIX ".aaxplugin"
      LIBRARY_OUTPUT_DIRECTORY "${AAX_OUTPUT_DIR}"
      LIBRARY_OUTPUT_DIRECTORY_DEBUG "${AAX_OUTPUT_DIR}"
      LIBRARY_OUTPUT_DIRECTORY_RELEASE "${AAX_OUTPUT_DIR}"
      LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${AAX_OUTPUT_DIR}"
      LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "${AAX_OUTPUT_DIR}"
    )

    # Create Resources folder and copy Pages.xml if it exists
    set(PAGES_XML ${PLUG_RESOURCES_DIR}/${project_name}-Pages.xml)
    if(EXISTS ${PAGES_XML})
      add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${IPLUG2_OUTPUT_DIR}/${project_name}.aaxplugin/Contents/Resources"
        COMMAND ${CMAKE_COMMAND} -E copy "${PAGES_XML}" "${IPLUG2_OUTPUT_DIR}/${project_name}.aaxplugin/Contents/Resources/${project_name}-Pages.xml"
        COMMENT "Creating AAX bundle structure and copying Pages.xml for ${project_name}"
      )
    else()
      add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${IPLUG2_OUTPUT_DIR}/${project_name}.aaxplugin/Contents/Resources"
        COMMENT "Creating AAX bundle structure for ${project_name}"
      )
    endif()

    # Copy icon file if present
    set(ICON_FILE ${PLUG_RESOURCES_DIR}/${project_name}.ico)
    if(EXISTS ${ICON_FILE})
      add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${ICON_FILE}" "${IPLUG2_OUTPUT_DIR}/${project_name}.aaxplugin/Contents/PlugIn.ico"
        COMMENT "Copying AAX icon for ${project_name}"
      )
    endif()

  elseif(APPLE)
    # macOS: AAX is a bundle with .aaxplugin extension
    set_target_properties(${target} PROPERTIES
      BUNDLE TRUE
      BUNDLE_EXTENSION "aaxplugin"
      MACOSX_BUNDLE_INFO_PLIST ${PLUG_RESOURCES_DIR}/${project_name}-AAX-Info.plist
      LIBRARY_OUTPUT_DIRECTORY "${IPLUG2_OUTPUT_DIR}"
      MACOSX_BUNDLE_BUNDLE_NAME "${project_name}"
      OUTPUT_NAME "${project_name}"
      XCODE_ATTRIBUTE_WRAPPER_EXTENSION "aaxplugin"
      XCODE_ATTRIBUTE_GENERATE_PKGINFO_FILE "NO"
    )

    # Create AAX-specific PkgInfo file (TDMwPTul instead of BNDL????)
    file(MAKE_DIRECTORY "${IPLUG2_GENERATED_DIR}")
    set(AAX_PKGINFO_PATH "${IPLUG2_GENERATED_DIR}/AAX_PkgInfo")
    file(WRITE "${AAX_PKGINFO_PATH}" "TDMwPTul")
    if(NOT XCODE)
      set(PKGINFO_DEST "${IPLUG2_OUTPUT_DIR}/${project_name}.aaxplugin/Contents/PkgInfo")
      add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${AAX_PKGINFO_PATH}" "${PKGINFO_DEST}"
        COMMENT "Creating AAX PkgInfo for ${project_name}.aaxplugin"
      )
    else()
      # Xcode: copy to bundle directory at build time
      add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${AAX_PKGINFO_PATH}" "$<TARGET_BUNDLE_DIR:${target}>/Contents/PkgInfo"
        COMMENT "Creating AAX PkgInfo for ${project_name}.aaxplugin"
      )
    endif()

    # Copy Pages.xml to Resources folder
    set(PAGES_XML ${PLUG_RESOURCES_DIR}/${project_name}-Pages.xml)
    if(EXISTS ${PAGES_XML})
      if(XCODE)
        target_sources(${target} PRIVATE ${PAGES_XML})
        set_source_files_properties(${PAGES_XML} PROPERTIES
          MACOSX_PACKAGE_LOCATION Resources
        )
      else()
        add_custom_command(TARGET ${target} POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy "${PAGES_XML}" "${IPLUG2_OUTPUT_DIR}/${project_name}.aaxplugin/Contents/Resources/${project_name}-Pages.xml"
          COMMENT "Copying AAX Pages.xml for ${project_name}"
        )
      endif()
    endif()
  endif()
endfunction()
