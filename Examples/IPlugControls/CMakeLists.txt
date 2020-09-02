cmake_minimum_required(VERSION 3.17)
include_guard(GLOBAL)


# Include the IPlug2.cmake module. Path can be absolute or relative.
include("../../cmake/IPlug2.cmake")


# Declare our project
project(IPlugControls)

iplug_configure_project( IPlugControls )

set(SOURCE_LIST
    PRIVATE
        "config.h"
        "IconsFontaudio.h"
        "IconsForkAwesome.h"
        "IPlugControls.cpp"
        "IPlugControls.h"

        "resources/main.rc"
        "resources/IPlugControls.ico"
)

set(INCLUDE_DIRECTORIES
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        "resources"
        "resources/fonts"
        "resources/img"
)


iplug_add_application(IPlugControls-app)
iplug_add_vst3(IPlugControls-vst3)

target_sources(IPlugControls-app ${SOURCE_LIST})
target_sources(IPlugControls-vst3 ${SOURCE_LIST})

target_include_directories(IPlugControls-app PRIVATE ${INCLUDE_DIRECTORIES})
target_include_directories(IPlugControls-vst3 PRIVATE ${INCLUDE_DIRECTORIES})

#target_link_libraries(IPlugControls-app PRIVATE)
#target_link_libraries(IPlugControls-vst3 PRIVATE)

# Generate source list for IDE
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCE_LIST})


