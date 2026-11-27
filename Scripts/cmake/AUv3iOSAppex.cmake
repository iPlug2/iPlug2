#  ==============================================================================
#
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

# AUv3 Appex target configuration for iPlug2 iOS
# The appex is an app extension (executable) that hosts the framework

include(${CMAKE_CURRENT_LIST_DIR}/AUv3iOS.cmake)

function(iplug_configure_auv3iosappex target project_name)
  set(framework_target ${project_name}AU-ios-framework)

  # Appex is an app bundle (executable) with .appex extension
  # Uses NSExtensionMain entry point from Foundation
  # The bundle is ${project_name}AppExtension.appex, matching OUTPUT_NAME, CFBundleExecutable in the project's
  # -iOS-AUv3-Info.plist and the iOS Xcode projects. It must not be renamed after the build: Xcode's later build
  # steps (e.g. dsymutil) expect it where it was built, and the bundle name has to match CFBundleExecutable.
  # For Ninja: Use separate output dir to avoid collision with APP target
  if(XCODE)
    set(APPEX_OUTPUT_DIR "${CMAKE_BINARY_DIR}/out")
  else()
    set(APPEX_OUTPUT_DIR "${CMAKE_BINARY_DIR}/out/appex-build")
  endif()

  set(APPEX_BUNDLE_ID "com.AcmeInc.${project_name}.AUv3")

  set_target_properties(${target} PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST ${PLUG_RESOURCES_DIR}/${project_name}-iOS-AUv3-Info.plist
    RUNTIME_OUTPUT_DIRECTORY "${APPEX_OUTPUT_DIR}"
    OUTPUT_NAME "${project_name}AppExtension"
    # iOS-specific Xcode attributes
    XCODE_ATTRIBUTE_WRAPPER_EXTENSION "appex"
    XCODE_PRODUCT_TYPE "com.apple.product-type.app-extension"
    XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO"
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${APPEX_BUNDLE_ID}"
    XCODE_ATTRIBUTE_SKIP_INSTALL "YES"
    XCODE_ATTRIBUTE_APPLICATION_EXTENSION_API_ONLY "YES"
  )

  # Use NSExtensionMain as entry point (provided by Foundation)
  # Add rpath to find framework at App/Frameworks/ (one level up from appex in PlugIns/)
  # iOS bundle structure: App/PlugIns/appex -> App/Frameworks/
  target_link_options(${target} PRIVATE
    "-e" "_NSExtensionMain"
    "-Wl,-rpath,@executable_path/../../Frameworks"
  )

  # Appex links to the framework binary at build time
  # At runtime, it finds the framework via rpath in the host app
  add_dependencies(${target} ${framework_target})

  # Link to the framework
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

  # Post-build: Create PkgInfo (and for Ninja, move the bundle to out/).
  # PkgInfo via staged source + `cmake -E copy` (see APP.cmake for rationale).
  set(AUV3_IOS_APPEX_PKGINFO_SRC "${CMAKE_CURRENT_BINARY_DIR}/PkgInfo_${target}")
  file(WRITE "${AUV3_IOS_APPEX_PKGINFO_SRC}" "XPC!????")
  set(APPEX_BUNDLE_NAME "${project_name}AppExtension.appex")

  if(XCODE)
    # For Xcode: XCODE_ATTRIBUTE_WRAPPER_EXTENSION creates ${APPEX_BUNDLE_NAME} directly, and Xcode processes
    # its Info.plist (expanding $(PRODUCT_BUNDLE_IDENTIFIER) etc.), so it must not be replaced by the source plist
    add_custom_command(TARGET ${target} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy "${AUV3_IOS_APPEX_PKGINFO_SRC}" "${CMAKE_BINARY_DIR}/out/$<CONFIG>/${APPEX_BUNDLE_NAME}/PkgInfo"
      COMMENT "Building ${APPEX_BUNDLE_NAME} (iOS)"
    )
  else()
    # For Ninja/other generators: nothing expands Xcode build settings in the Info.plist, so substitute the bundle ID
    file(READ "${PLUG_RESOURCES_DIR}/${project_name}-iOS-AUv3-Info.plist" APPEX_PLIST)
    string(REPLACE "$(PRODUCT_BUNDLE_IDENTIFIER)" "${APPEX_BUNDLE_ID}" APPEX_PLIST "${APPEX_PLIST}")
    set(APPEX_PLIST_FILE "${CMAKE_CURRENT_BINARY_DIR}/${project_name}-iOS-AUv3-Info.plist")
    file(WRITE "${APPEX_PLIST_FILE}" "${APPEX_PLIST}")

    # Convert the .app to .appex and move it to its final location
    add_custom_command(TARGET ${target} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy "${AUV3_IOS_APPEX_PKGINFO_SRC}" "$<TARGET_BUNDLE_DIR:${target}>/PkgInfo"
      COMMAND ${CMAKE_COMMAND} -E copy "${APPEX_PLIST_FILE}" "$<TARGET_BUNDLE_DIR:${target}>/Info.plist"
      COMMAND ${CMAKE_COMMAND} -E rm -rf "${CMAKE_BINARY_DIR}/out/${APPEX_BUNDLE_NAME}"
      COMMAND ${CMAKE_COMMAND} -E rename
        "$<TARGET_BUNDLE_DIR:${target}>"
        "${CMAKE_BINARY_DIR}/out/${APPEX_BUNDLE_NAME}"
      COMMENT "Building ${APPEX_BUNDLE_NAME} (iOS)"
    )
  endif()
endfunction()
