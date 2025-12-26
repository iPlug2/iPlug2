#  ==============================================================================
#
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

# AUv3 Framework target configuration for iPlug2
# The framework contains the actual plugin code and is loaded in-process

include(${CMAKE_CURRENT_LIST_DIR}/AUv3.cmake)

function(iplug_configure_auv3framework target project_name)
  target_link_libraries(${target} PUBLIC iPlug2::AUv3)

  # AUv3 requires ARC for its Objective-C++ sources
  set(AUV3_SOURCES
    ${IPLUG_DIR}/AUv3/IPlugAUv3.mm
    ${IPLUG_DIR}/AUv3/IPlugAUAudioUnit.mm
    ${IPLUG_DIR}/AUv3/IPlugAUViewController.mm
  )
  set_source_files_properties(${AUV3_SOURCES}
    PROPERTIES COMPILE_FLAGS "-fobjc-arc"
  )

  # Generate Info.plist with correct CFBundleExecutable
  # The source plist has "AUv3Framework" but the binary is named "${project_name}AU"
  set(FRAMEWORK_PLIST_SOURCE "${PLUG_RESOURCES_DIR}/${project_name}-macOS-AUv3Framework-Info.plist")
  set(FRAMEWORK_PLIST_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${project_name}-AUv3Framework-Info.plist")
  if(EXISTS "${FRAMEWORK_PLIST_SOURCE}")
    file(READ "${FRAMEWORK_PLIST_SOURCE}" PLIST_CONTENT)
    # Replace generic "AUv3Framework" with actual binary name
    string(REPLACE "<string>AUv3Framework</string>" "<string>${project_name}AU</string>" PLIST_CONTENT "${PLIST_CONTENT}")
    file(WRITE "${FRAMEWORK_PLIST_OUTPUT}" "${PLIST_CONTENT}")
  else()
    message(WARNING "AUv3 Framework Info.plist not found: ${FRAMEWORK_PLIST_SOURCE}")
    set(FRAMEWORK_PLIST_OUTPUT "${FRAMEWORK_PLIST_SOURCE}")
  endif()

  set_target_properties(${target} PROPERTIES
    FRAMEWORK TRUE
    FRAMEWORK_VERSION A
    MACOSX_FRAMEWORK_INFO_PLIST "${FRAMEWORK_PLIST_OUTPUT}"
    OUTPUT_NAME "${project_name}AU"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out"
    # Disable Xcode signing - we sign manually in post-build to ensure correct order
    XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO"
  )

  # Create PkgInfo file for framework bundle
  set(PKGINFO_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/write_pkginfo_${target}.cmake")
  file(WRITE ${PKGINFO_SCRIPT} "file(WRITE \"\${PKGINFO_PATH}\" \"FMWK????\")")
  add_custom_command(TARGET ${target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -DPKGINFO_PATH="$<TARGET_BUNDLE_DIR:${target}>/Versions/A/Resources/PkgInfo"
      -P "${PKGINFO_SCRIPT}"
    COMMENT "Creating PkgInfo for ${project_name}AU.framework"
  )

  # Code sign framework with hardened runtime (required for AUv3 to register)
  if(IPLUG2_DEVELOPMENT_TEAM)
    add_custom_command(TARGET ${target} POST_BUILD
      COMMAND codesign --force --sign "Apple Development" -o runtime --timestamp=none
        "$<TARGET_BUNDLE_DIR:${target}>"
      COMMENT "Signing ${project_name}AU.framework"
    )
  endif()

  # Add headers to framework (umbrella + public headers)
  # Check for umbrella header - support both naming conventions:
  # - ${project_name}AU.h (TemplateProject style)
  # - AUv3Framework.h (iPlug2 Examples style)
  if(EXISTS "${PLUG_RESOURCES_DIR}/${project_name}AU.h")
    set(UMBRELLA_HEADER "${PLUG_RESOURCES_DIR}/${project_name}AU.h")
  elseif(EXISTS "${PLUG_RESOURCES_DIR}/AUv3Framework.h")
    set(UMBRELLA_HEADER "${PLUG_RESOURCES_DIR}/AUv3Framework.h")
  else()
    message(WARNING "No AUv3 umbrella header found for ${project_name}")
    set(UMBRELLA_HEADER "")
  endif()

  set(FRAMEWORK_HEADERS
    ${UMBRELLA_HEADER}
    ${IPLUG_DIR}/AUv3/IPlugAUViewController.h
    ${IPLUG_DIR}/AUv3/IPlugAUAudioUnit.h
  )
  target_sources(${target} PRIVATE ${FRAMEWORK_HEADERS})
  set_source_files_properties(${FRAMEWORK_HEADERS}
    PROPERTIES MACOSX_PACKAGE_LOCATION Headers
  )

  # Compile XIB to NIB and add to framework Resources
  set(AU_VIEW_XIB ${PLUG_RESOURCES_DIR}/IPlugAUViewController_v${project_name}.xib)
  if(EXISTS ${AU_VIEW_XIB})
    set(AU_VIEW_NIB ${CMAKE_CURRENT_BINARY_DIR}/IPlugAUViewController_v${project_name}.nib)
    add_custom_command(
      OUTPUT ${AU_VIEW_NIB}
      COMMAND ibtool --compile ${AU_VIEW_NIB} ${AU_VIEW_XIB}
      DEPENDS ${AU_VIEW_XIB}
      COMMENT "Compiling IPlugAUViewController_v${project_name}.xib"
    )
    target_sources(${target} PRIVATE ${AU_VIEW_NIB})
    set_source_files_properties(${AU_VIEW_NIB} PROPERTIES
      MACOSX_PACKAGE_LOCATION Resources
    )
  endif()
endfunction()
