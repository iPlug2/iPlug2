#!/bin/bash

echo "Formatting..."

git ls-files "IPlug/*.h" "IPlug/*.cpp" "IPlug/*.mm" | xargs clang-format -i .
git ls-files "IGraphics/*.h" "IGraphics/*.cpp" "IGraphics/*.mm" | xargs clang-format -i .
git ls-files "Examples/*.h" "Examples/*.cpp" "Tests/*.h" "Tests/*.cpp" | grep -v 'resource.h' | xargs clang-format -i .