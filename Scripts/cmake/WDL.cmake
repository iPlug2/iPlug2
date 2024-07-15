#  ==============================================================================
#  
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

# Set the WDL directory
set(WDL_DIR ${IPLUG2_DIR}/WDL)

# Create an interface library for WDL
add_library(_wdl INTERFACE)

# Set the source directory for WDL
set(WDL_SRC "${WDL_DIR}/")

# List of WDL source files
set(_wdl_src
  fft.c
  fft.h
  resample.cpp
  resample.h
  convoengine.h
  convoengine.cpp
)

# Prepend the WDL source directory to each file
list(TRANSFORM _wdl_src PREPEND "${WDL_SRC}")

# Configure the WDL interface library
iplug_target_add(_wdl INTERFACE
  INCLUDE ${WDL_DIR}
  SOURCE ${_wdl_src}
)