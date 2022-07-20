cmake_minimum_required(VERSION 3.11)

set(IPLUG2_AAX_ICON
  "${IPLUG2_DIR}/Dependencies/IPlug/AAX_SDK/Utilities/PlugIn.ico"
  CACHE FILEPATH "Path to AAX plugin icon"
)

if (WIN32)
  set(AAX_32_PATH "C:/Program Files (x86)/Common Files/Avid/Audio/Plug-Ins"
    CACHE PATH "Path to install 32-bit AAX plugins")
  set(AAX_64_PATH "C:/Program Files/Common Files/Avid/Audio/Plug-Ins"
    CACHE PATH "Path to install 64-bit AAX plugins")
endif()

function(iplug_configure_aax target)
  message("AAX not yet implemented" FATAL_ERROR)
endfunction()
