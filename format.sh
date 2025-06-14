#!/bin/zsh
pushd "$(dirname "$0")"
set -e

# Define the array of directories to format
FOLDERS=(
	"Flameberry/Source/Developer"
	"Flameberry/Source/Editor"
)

# Ensure clang-format is available
if ! command -v clang-format &>/dev/null; then
	echo "Error: clang-format not found in PATH"
	exit 1
fi

# Print working directory (where .clang-format should be)
echo "Using .clang-format from: $(pwd)"

# Format each folder
for folder in "${FOLDERS[@]}"; do
	echo "Formatting files in $folder"

	find "$folder" -type f \( -name "*.cpp" -o -name "*.cc" -o -name "*.cxx" -o -name "*.h" -o -name "*.hpp" \) \
		-exec clang-format -i {} +
done

echo "Formatting complete."

popd
