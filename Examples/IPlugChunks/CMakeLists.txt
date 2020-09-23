cmake_minimum_required(VERSION 3.17)
include_guard(GLOBAL)


# Include the IPlug2.cmake module. Path can be absolute or relative.
include("../../cmake/IPlug2.cmake")


# Declare our project
project(IPlugChunks)

# Project configuration
iplug_configure_project(
    PLUG_NAME                   "IPlugChunks"
    PLUG_MFR                    "AcmeInc"
    PLUG_VERSION_HEX            0x00010000
    PLUG_VERSION_STR            "1.0.0"
    PLUG_UNIQUE_ID              'W1Q1'
    PLUG_MFR_ID                 'Acme'
    PLUG_URL_STR                "https://iplug2.github.io"
    PLUG_EMAIL_STR              "spam@me.com"
    PLUG_COPYRIGHT_STR          "Copyright 2020 Acme Inc"

    BUNDLE_NAME                 "IPlugChunks"
    BUNDLE_DOMAIN               "com"

    SHARED_RESOURCES_SUBPATH    "IPlugChunks"

    PLUG_CHANNEL_IO             "1-1 2-2"

    PLUG_LATENCY                0
    PLUG_TYPE                   Effect
    PLUG_DOES_MIDI_IN           0
    PLUG_DOES_MIDI_OUT          0
    PLUG_DOES_MPE               0
    PLUG_DOES_STATE_CHUNKS      1
    PLUG_HAS_UI                 1
    PLUG_WIDTH                  600
    PLUG_HEIGHT                 600
    PLUG_FPS                    60
    PLUG_SHARED_RESOURCES       0
    PLUG_HOST_RESIZE            0

    AUV2_ENTRY                  IPlugChunks_Entry
    AUV2_FACTORY                IPlugChunks_Factory
    AUV2_VIEW_CLASS             IPlugChunks_View

    AAX_TYPE_IDS                "'IEF1', 'IEF2'"
    AAX_TYPE_IDS_AUDIOSUITE     "'IEA1', 'IEA2'"
    AAX_PLUG_CATEGORY_STR       "Effect"
    AAX_DOES_AUDIOSUITE         1

    VST3_SUBCATEGORY            "Fx"

    APP_NUM_CHANNELS            2
    APP_N_VECTOR_WAIT           0
    APP_MULT                    1
    APP_COPY_AUV3               0
    APP_SIGNAL_VECTOR_SIZE      64

    RESOURCE_DEFINITIONS
        ROBOTO_FN               "Roboto-Regular.ttf"
)


set(SOURCE_LIST
    PRIVATE
        "IPlugChunks.cpp"
        "IPlugChunks.h"

        "resources/main.rc"
        "resources/IPlugChunks.ico"
)

set(INCLUDE_DIRECTORIES
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        "resources"
        "resources/fonts"
        "resources/img"
)

# Add targets
iplug_add_application(IPlugChunks-app)
iplug_add_vst3(IPlugChunks-vst3)

target_sources(IPlugChunks-app ${SOURCE_LIST})
target_sources(IPlugChunks-vst3 ${SOURCE_LIST})

target_include_directories(IPlugChunks-app PRIVATE ${INCLUDE_DIRECTORIES})
target_include_directories(IPlugChunks-vst3 PRIVATE ${INCLUDE_DIRECTORIES})

# extra libraries to link if needed
target_link_libraries(IPlugChunks-app PRIVATE )
target_link_libraries(IPlugChunks-vst3 PRIVATE )

# Generate source list for IDE
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCE_LIST})
