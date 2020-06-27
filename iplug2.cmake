#
# This file should be included in your main CMakeLists.txt file. #
#

# We need this so we can find call FindFaust.cmake
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
# This is used in iplug2_configure_target
set(IPLUG2_DIR ${CMAKE_CURRENT_LIST_DIR})
