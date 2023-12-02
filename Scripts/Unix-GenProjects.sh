#!/bin/zsh
pushd "$(dirname "$0")/.."

metaFilePath="./Scripts/Setup.meta"
cmakeCommand="cmake"

# Check if cmake is available in the system
if command -v $cmakeCommand &>/dev/null; then
    if [[ "$OSTYPE" == "darwin"* ]]; then
        cmake -Wno-dev -S. -Bbuild/Xcode -G"Xcode"
    else
        cmake -Wno-dev -S. -Bbuild/
    fi
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
        if [[ "$OSTYPE" == "darwin"* ]]; then
            "$cmakePath" -Wno-dev -S. -Bbuild/Xcode -G"Xcode"
        else
            "$cmakePath" -Wno-dev -S. -Bbuild/
        fi
    else
        echo "Error: CMake path not found in $metaFilePath."
    fi
fi
popd