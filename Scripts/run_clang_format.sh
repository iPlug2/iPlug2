#!/bin/bash

# script to clang-format iPlug2 source codes
# based on https://gist.github.com/leilee/1d0915a583f8f29414cc21cd86e7151b

# Folders to process
DIRS=(
./../IPlug
./../IGraphics
./../Examples
./../Tests
)

# Variable that will hold the name of the clang-format command
FMT=""

for clangfmt in clang-format{,-{4,3}.{9,8,7,6,5,4,3,2,1,0}}; do
  if which "$clangfmt" &>/dev/null; then
    FMT="$clangfmt"
    break
  fi
done

# Check if we found a working clang-format
if [ -z "$FMT" ]; then
  echo "failed to find clang-format"
  exit 1
fi

function format() {
  for f in $(find $@ -name '*.h' -or -name '*.m' -or -name '*.mm' -or -name '*.c' -or -name '*.cpp'); do 
    echo "formating: ${f}"
    ${FMT} -i style=file ${f}
  done

  echo "~~~ $@ Done ~~~";
}

# Check all of the arguments first to make sure they're all directories
for dir in "${DIRS[@]}"; do
  if [ ! -d "${dir}" ]; then
    echo "${dir} is not a directory"
  else
    format ${dir}
  fi
done