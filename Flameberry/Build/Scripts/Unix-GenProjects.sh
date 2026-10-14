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

# Path to where the info about the cmake path is stored
metaFilePath="./Flameberry/Build/Scripts/Setup.meta"
cmakeCommand="cmake"

# Check if cmake is available in the system
if ! command -v $cmakeCommand &>/dev/null; then
    # Read the path from Setup.meta
    while IFS="=" read -r tool path; do
        if [ "$tool" = "cmake" ]; then
			cmakeCommand="$path"
            break
        fi
    done < "$metaFilePath"
fi

if [[ "$OSTYPE" == "darwin"* ]]; then
	buildDir="Flameberry/Intermediate/Build/Xcode"
else
	buildDir="Flameberry/Intermediate/Build"
fi

# Xcode updates remove the previous macOS SDK, but CMake keeps the paths it found in its cache (e.g. ZLIB from the SDK)
# If a cached find result (PATH/FILEPATH entry) references an SDK that no longer exists, discard the cache so that everything is found again
cacheFile="$buildDir/CMakeCache.txt"
if [ -f "$cacheFile" ]; then
	for sdkPath in $(grep -E '^[^#]*:(FILE)?PATH=' "$cacheFile" | grep -o '/[^][ ;:"]*/SDKs/[^/]*\.sdk' | sort -u); do
		if [ ! -d "$sdkPath" ]; then
			echo "CMake cache references missing SDK '$sdkPath', regenerating cache..."
			rm -rf "$cacheFile" "$buildDir/CMakeFiles"
			break
		fi
	done
fi

# Run cmake with the extracted path
if [[ "$OSTYPE" == "darwin"* ]]; then
	"$cmakeCommand" -Wno-dev -S. -B"$buildDir" -G"Xcode"
else
	"$cmakeCommand" -Wno-dev -S. -B"$buildDir"
fi

if [ "$openFlag" = "ON" ] && [[ "$OSTYPE" == "darwin"* ]]; then
    open "$buildDir/FlameberryEngine.xcodeproj"
fi

popd
