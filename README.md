# Flameberry Engine

Flameberry Engine is a C++ Game Engine based upon the Vulkan Graphics API and is currently under development. My goal is to build a capable fast and efficient 3D game engine which can ship games on platforms like macOS, Windows, and Linux, and maybe Consoles, PlayStations, Android and iOS in the future!

<img src="screenshots/Flameberry_SS_UI.png">

***

## Build Requirements:
1. Vulkan/MoltenVK support
2. Python
3. CMake (Preferred)
3. C++17 Compiler

## Build Steps:
1. Clone the repository using: <br> `git clone --recursive https://github.com/flameberry/FlameberryEngine`
2. Run the `Scripts/Setup.py` script to setup all the dependencies and generate project files.
3. Open the project file using an IDE and build it and run it.
4. After running the Engine for the first time, if you want to add new files or configurations, just add them and run `Win-GenProjects.bat` or `Unix-GenProjects.sh` for generating Visual Studio or Xcode projects respectively. Optionally you can run `Win-AutoGenAndBuild.bat` or `Unix-AutoGenAndBuild.sh` if you don't use those IDEs.

## Third party libs:
1. GLFW
2. Vulkan SDK
3. glm
4. Dear ImGui
5. ImGuizmo
6. stb
7. Tiny OBJ Loader
8. Yaml-CPP
9. Nvidia PhysX
10. Microsoft Dotnet