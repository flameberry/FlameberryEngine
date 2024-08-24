#!/bin/zsh
pushd "$(dirname "$0")/../../.."
set -e

# Command Line Options
compileCommandsFlag=OFF 
buildConfig="Release"

while getopts "cd" opt; do
	case $opt in
		c)
			compileCommandsFlag=ON
			;;
		d)
			buildConfig="Debug"
			;;
		\?)
			echo "Invalid option: -$OPTARG" >&2
			exit 1
			;;
	esac
done

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
"$cmakeCommand" -Wno-dev -DCMAKE_BUILD_TYPE="$buildConfig" -DCMAKE_EXPORT_COMPILE_COMMANDS=$compileCommandsFlag -S. -BFlameberry/Intermediate/Build/Auto
# Copy compile_commands.json to the root project folder to provide intellisense for various code editors
cp Flameberry/Intermediate/Build/Auto/compile_commands.json .
"$cmakeCommand" --build Flameberry/Intermediate/Build/Auto

popd
