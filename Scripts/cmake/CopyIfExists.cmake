# Copy a directory only if it exists. Called from Deploy.cmake post-build.
# Usage: cmake -Dsrc=<path> -Ddst=<path> -P CopyIfExists.cmake
if(IS_DIRECTORY "${src}")
  file(COPY "${src}/" DESTINATION "${dst}")
endif()
