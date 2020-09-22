cmake_minimum_required(VERSION 3.17)
include_guard(GLOBAL)


# Include the IPlug2.cmake module. Path can be absolute or relative.
include("../../cmake/IPlug2.cmake")


# Declare our project
project(IPlugControls)

# Project configuration
iplug_configure_project(
    PLUG_NAME                   "IPlugControls"             #
    PLUG_MFR                    "AcmeInc"                   #
    PLUG_VERSION_HEX            0x00010000                  #
    PLUG_VERSION_STR            "1.0.0"                     #
    PLUG_UNIQUE_ID              'Ipct'                      #
    PLUG_MFR_ID                 'Acme'                      #
    PLUG_URL_STR                "https://iplug2.github.io"  #
    PLUG_EMAIL_STR              "spam@me.com"               #
    PLUG_COPYRIGHT_STR          "Copyright 2020 Acme Inc"   #
    PLUG_TYPE                   Instrument                  # [Effect|Instrument|MIDIEffect]
    PLUG_CHANNEL_IO             "0-2"                       #
    PLUG_LATENCY                0                           #
    PLUG_DOES_MIDI_IN           1                           #
    PLUG_DOES_MIDI_OUT          0                           #
    PLUG_DOES_MPE               0                           #
    PLUG_DOES_STATE_CHUNKS      0                           #
    PLUG_HAS_UI                 1                           #
    PLUG_WIDTH                  1024                        #
    PLUG_HEIGHT                 768                         #
    PLUG_HOST_RESIZE            0                           #
    PLUG_FPS                    60                          #
    PLUG_SHARED_RESOURCES       0                           #

    BUNDLE_NAME                 "IPlugControls"             #
    BUNDLE_MFR                  "AcmeInc"                   #
    BUNDLE_DOMAIN               "com"                       #

    ICON                        ""                          #

    SHARED_RESOURCES_SUBPATH    "IPlugControls"             #
    VST3_SUBCATEGORY            "Instrument|Synth"          # TODO: move to vst3 config

    APP_NUM_CHANNELS            2                           # TODO: move to app config
    APP_N_VECTOR_WAIT           0                           #
    APP_MULT                    1                           #
    APP_COPY_AUV3               0                           #
    APP_SIGNAL_VECTOR_SIZE      64                          #

    AAX_TYPE_IDS                'IPCT'                      # TODO: move to aax config
    AAX_TYPE_IDS_AUDIOSUITE     "'EFA1', 'EFA2'"            #
    AAX_PLUG_MFR_STR            "Acme"                      #
    AAX_PLUG_CATEGORY_STR       "Synth"                     #
    AAX_DOES_AUDIOSUITE         0                           #

    AUV2_ENTRY                  IPlugControls_Entry         # TODO: move to auv2 config
    AUV2_VIEW_CLASS             IPlugControls_View          #
    AUV2_FACTORY                IPlugControls_Factory       #


    RESOURCE_DEFINITIONS
        ROBOTO_FN               "Roboto-Regular.ttf"
        FORK_AWESOME_FN         "forkawesome-webfont.ttf"
        FONTAUDIO_FN            "fontaudio.ttf"
        PNGKNOB_FN              "knob.png"
        PNGKNOB_2X_FN           "knob@2x.png"
        PNGKNOBROTATE_FN        "knob-rotate.png"
        PNGKNOBROTATE_2X_FN     "knob-rotate@2x.png"
        PNGSWITCH_FN            "switch.png"
        PNGBUTTON_FN            "button.png"
        PNGSLIDERHANDLE_FN      "slider-handle.png"
        PNGSLIDERTRACK_FN       "slider-track.png"
        PNGTEXT_FN              "font.png"
        PNGTEXT_2X_FN           "font@2x.png"
        SVGSLIDERHANDLE_FN      "slider-handle.svg"
        SVGSLIDERTRACK_FN       "slider-track.svg"
        SVGKNOBROTATE_FN        "vector-knob.svg"
        SVGHSLIDERHANDLE_FN     "hslider-handle.svg"
        SVGHSLIDERTRACK_FN      "hslider-track.svg"
)


set(SOURCE_LIST
    PRIVATE
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

# Add targets
iplug_add_application(IPlugControls-app)
iplug_add_vst3(IPlugControls-vst3)

target_sources(IPlugControls-app ${SOURCE_LIST})
target_sources(IPlugControls-vst3 ${SOURCE_LIST})

target_include_directories(IPlugControls-app PRIVATE ${INCLUDE_DIRECTORIES})
target_include_directories(IPlugControls-vst3 PRIVATE ${INCLUDE_DIRECTORIES})

# extra libraries to link if needed
target_link_libraries(IPlugControls-app PRIVATE )
target_link_libraries(IPlugControls-vst3 PRIVATE )

# Generate source list for IDE
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCE_LIST})


