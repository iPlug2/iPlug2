set(WDL_DIR ${IPLUG2_DIR}/WDL)

#
add_library(_wdl INTERFACE)
set(WDL_SRC "${WDL_DIR}/")
set(_wdl_src
  besselfilter.cpp
  besselfilter.h
  fft.c
  fft.h
  resample.cpp
  resample.h
  convoengine.h
  convoengine.cpp
  )
list(TRANSFORM _wdl_src PREPEND "${WDL_SRC}")
iplug_target_add(_wdl INTERFACE
  INCLUDE ${WDL_DIR}
  SOURCE ${_wdl_src}
)
