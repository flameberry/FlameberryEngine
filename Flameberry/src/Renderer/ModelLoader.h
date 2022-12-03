#pragma once

#include <string>
#include <tuple>

#include "OpenGL/OpenGLVertex.h"
#include "Mesh.h"

namespace Flameberry {
    class ModelLoader
    {
    public:
        static std::tuple<std::vector<OpenGLVertex>, std::vector<uint32_t>> LoadOBJ(const std::string& modelPath);
        static void LoadOBJ(const std::string& modelPath, std::vector<Mesh>* targetMeshes);
        static float ParseFloat(const char* str, char delimiter = ' ');
        static void Hello();
    };
}
