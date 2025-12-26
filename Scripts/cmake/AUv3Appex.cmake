#  ==============================================================================
#
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

# AUv3 Appex target configuration for iPlug2
# The appex is an app extension (executable) that hosts the framework

include(${CMAKE_CURRENT_LIST_DIR}/AUv3.cmake)

function(iplug_configure_auv3appex target project_name)
  set(framework_target ${project_name}AU-framework)

  # Appex is an app bundle (executable) with .appex extension
  # Uses NSExtensionMain entry point from Foundation
  # OUTPUT_NAME matches CFBundleExecutable in Info.plist
  # For Ninja: Use separate output dir to avoid collision with APP target
  if(XCODE)
    set(APPEX_OUTPUT_DIR "${CMAKE_BINARY_DIR}/out")
  else()
    set(APPEX_OUTPUT_DIR "${CMAKE_BINARY_DIR}/out/appex-build")
  endif()

  set_target_properties(${target} PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST ${PLUG_RESOURCES_DIR}/${project_name}-macOS-AUv3-Info.plist
    RUNTIME_OUTPUT_DIRECTORY "${APPEX_OUTPUT_DIR}"
    OUTPUT_NAME "${project_name}"
    XCODE_ATTRIBUTE_WRAPPER_EXTENSION "appex"
    # Product type must be app-extension for proper Xcode validation
    XCODE_PRODUCT_TYPE "com.apple.product-type.app-extension"
    # Disable Xcode signing - we sign manually in post-build to ensure correct order
    XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO"
    # Set bundle identifier to match what's in Info.plist
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.AcmeInc.app.${project_name}.AUv3"
    # Skip install paths check
    XCODE_ATTRIBUTE_SKIP_INSTALL "YES"
  )

  # Use NSExtensionMain as entry point (provided by Foundation)
  # Add rpath to find framework at App/Contents/Frameworks/ (two levels up from appex MacOS)
  target_link_options(${target} PRIVATE
    "-e" "_NSExtensionMain"
    "-Wl,-rpath,@executable_path/../../Frameworks"
  )

  # Appex links to the framework binary at build time
  # At runtime, it finds the framework via rpath in the host app
  add_dependencies(${target} ${framework_target})

  # Use generator expression to get the correct framework path
  # For Xcode, LIBRARY_OUTPUT_DIRECTORY gets config subdirectory appended
  target_link_libraries(${target} PRIVATE
    "$<TARGET_FILE:${framework_target}>"
    "-framework Foundation"
  )

  # Enable ARC for appex source
  set(APPEX_SOURCE ${PLUG_RESOURCES_DIR}/${project_name}AUv3Appex.m)
  if(EXISTS ${APPEX_SOURCE})
    set_source_files_properties(${APPEX_SOURCE}
      PROPERTIES COMPILE_FLAGS "-fobjc-arc"
    )
  endif()

  # Post-build: Create PkgInfo and rename bundle to final name
  # NOTE: Framework is NOT embedded in appex - it goes at app level (see iplug_embed_auv3_in_app)
  # NOTE: For Xcode generator, XCODE_ATTRIBUTE_WRAPPER_EXTENSION handles the .appex extension
  set(PKGINFO_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/write_pkginfo_${target}.cmake")
  file(WRITE ${PKGINFO_SCRIPT} "file(WRITE \"\${PKGINFO_PATH}\" \"XPC!????\")")

  if(XCODE)
    # For Xcode: XCODE_ATTRIBUTE_WRAPPER_EXTENSION creates .appex bundle directly
    # Bundle is created as ${project_name}.appex, we rename to ${project_name}AUv3.appex
    set(APPEX_BUNDLE "${CMAKE_BINARY_DIR}/out/$<CONFIG>/${project_name}.appex")
    set(APPEX_FINAL "${CMAKE_BINARY_DIR}/out/$<CONFIG>/${project_name}AUv3.appex")
    if(IPLUG2_DEVELOPMENT_TEAM)
      add_custom_command(TARGET ${target} POST_BUILD
        # Create PkgInfo
        COMMAND ${CMAKE_COMMAND} -DPKGINFO_PATH="${APPEX_BUNDLE}/Contents/PkgInfo"
          -P "${PKGINFO_SCRIPT}"
        # Rename bundle to final name with AUv3 suffix
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${APPEX_FINAL}"
        COMMAND ${CMAKE_COMMAND} -E rename
          "${APPEX_BUNDLE}"
          "${APPEX_FINAL}"
        # Sign the appex with hardened runtime and entitlements (required for AUv3 to register)
        COMMAND codesign --force --sign "Apple Development" -o runtime --timestamp=none
          --entitlements "${CMAKE_CURRENT_SOURCE_DIR}/projects/${project_name}-macOS.entitlements"
          "${APPEX_FINAL}"
        COMMENT "Building and signing ${project_name}AUv3.appex"
      )
    else()
      add_custom_command(TARGET ${target} POST_BUILD
        # Create PkgInfo
        COMMAND ${CMAKE_COMMAND} -DPKGINFO_PATH="${APPEX_BUNDLE}/Contents/PkgInfo"
          -P "${PKGINFO_SCRIPT}"
        # Rename bundle to final name with AUv3 suffix
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${APPEX_FINAL}"
        COMMAND ${CMAKE_COMMAND} -E rename
          "${APPEX_BUNDLE}"
          "${APPEX_FINAL}"
        COMMENT "Building ${project_name}AUv3.appex"
      )
    endif()
  else()
    # For Ninja/other generators: Convert .app to .appex and move to final location
    # Bundle is built in appex-build/ subdir to avoid collision with APP target
    add_custom_command(TARGET ${target} POST_BUILD
      # Create PkgInfo
      COMMAND ${CMAKE_COMMAND} -DPKGINFO_PATH="$<TARGET_BUNDLE_DIR:${target}>/Contents/PkgInfo"
        -P "${PKGINFO_SCRIPT}"
      # Copy Info.plist (CMake may not process it correctly for non-Xcode)
      COMMAND ${CMAKE_COMMAND} -E copy
        "${PLUG_RESOURCES_DIR}/${project_name}-macOS-AUv3-Info.plist"
        "$<TARGET_BUNDLE_DIR:${target}>/Contents/Info.plist"
      # Move and rename .app bundle to .appex in final location
      COMMAND ${CMAKE_COMMAND} -E rm -rf "${CMAKE_BINARY_DIR}/out/${project_name}AUv3.appex"
      COMMAND ${CMAKE_COMMAND} -E rename
        "$<TARGET_BUNDLE_DIR:${target}>"
        "${CMAKE_BINARY_DIR}/out/${project_name}AUv3.appex"
      COMMENT "Building ${project_name}AUv3.appex"
    )
  endif()
endfunction()
