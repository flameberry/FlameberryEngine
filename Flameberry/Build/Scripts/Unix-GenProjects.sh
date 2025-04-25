#!/bin/zsh
pushd "$(dirname "$0")/../../.."
set -e

openFlag=OFF

while getopts "o" opt; do
	case $opt in
		o)
			openFlag=ON
			;;
		\?)
			echo "Invalid option: -$OPTARG" >&2
			exit 1
			;;
	esac
done

metaFilePath="./Build/Scripts/Setup.meta"
cmakeCommand="cmake"

# Path to where the info about the cmake path is stored
metaFilePath="./Build/Scripts/Setup.meta"
cmakeCommand="cmake"

# Check if cmake is available in the system
if ! command -v $cmakeCommand &>/dev/null; then
    # Read the path from Setup.meta
    while IFS="=" read -r tool path; do
        if [ "$tool" == "cmake" ]; then
			cmakeCommand="$path"
            break
        fi
    done < "$metaFilePath"
fi

# Run cmake with the extracted path
if [[ "$OSTYPE" == "darwin"* ]]; then
	"$cmakeCommand" -Wno-dev -S. -BFlameberry/Intermediate/Build/Xcode -G"Xcode"
else
	"$cmakeCommand" -Wno-dev -S. -BFlameberry/Intermediate/Build
fi

if openFlag=ON; then
    open Flameberry/Intermediate/Build/Xcode/FlameberryEngine.xcodeproj
fi

popd
