#!/bin/zsh
pushd "$(dirname "$0")/../../.."

metaFilePath="./Build/Scripts/Setup.meta"
cmakeCommand="cmake"

# Check if cmake is available in the system
if command -v $cmakeCommand &>/dev/null; then
    cmake -Wno-dev -DCMAKE_BUILD_TYPE="Release" -S. -BFlameberry/Intermediate/Build/Auto
    cmake --build Flameberry/Intermediate/Build/Auto
else
    # Read the path from Setup.meta
    while IFS="=" read -r tool path; do
        if [ "$tool" == "cmake" ]; then
            cmakePath="$path"
            break
        fi
    done < "$metaFilePath"

    # Check if cmakePath is set
    if [ -n "$cmakePath" ]; then
        # Run cmake with the extracted path
        "$cmakePath" -Wno-dev -DCMAKE_BUILD_TYPE="Release" -S. -BFlameberry/Intermediate/Build/Auto
        "$cmakePath" --build Flameberry/Intermediate/Build/Auto
    else
        echo "Error: CMake path not found in $metaFilePath."
    fi
fi
popd
