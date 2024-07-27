# Flameberry Engine

![Logo](Flameberry/Documentation/Branding/Flameberry-Crazy-Logo-Upscaled.jpg)

Flameberry Engine is a C++ Game Engine based upon the Vulkan Graphics API and is currently under development. My goal is to build a capable fast and efficient 3D game engine which can ship games on platforms like macOS, Windows, and Linux, and maybe Consoles, PlayStations, Android and iOS in the future!

## Preview

<img src="Flameberry/Documentation/Screenshots/Flameberry_SS_UI.png">

***

## Build Requirements
1. C++17 Compiler
2. Vulkan/MoltenVK support
3. Python
4. CMake (Preferred)
5. For Windows, Visual Studio is required to build PhysX SDK

_Note: If you have Visual Studio version less than 17 installed then in the `Scripts/Win-GenProjects.bat` change the CMake Generator from `Visual Studio 17 2022` to the appropriate version that you have_

## Build Steps

_Note: For detailed build steps: <a href="https://flameberry.github.io/docs/installationguide/">Flameberry Website - Docs</a>_

1. Clone the repository using: <br> `git clone --recursive https://github.com/flameberry/FlameberryEngine`
2. Run the `Flameberry/Build/Scripts/Setup.py` script to setup all the dependencies and generate project files.
3. Open the project file using an IDE and build it and run it.
4. After running the Engine for the first time, if you want to add new files or configurations, just add them and run `Win-GenProjects.bat` or `Unix-GenProjects.sh` for generating Visual Studio or Xcode projects respectively. Optionally you can run `Win-AutoGenAndBuild.bat` or `Unix-AutoGenAndBuild.sh` if you don't use those IDEs.

## Third party libraries
1. GLFW
2. VulkanSDK
3. glm
4. Dear ImGui
5. ImGuizmo
6. stb
7. fmtlib
8. Tiny OBJ Loader
9. Yaml-CPP
10. Assimp
11. Nvidia PhysX
12. Freetype
13. MSDF-Gen-Atlas
14. MurmurHash
15. IconFontCppHeaders