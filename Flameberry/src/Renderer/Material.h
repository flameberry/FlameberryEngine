#pragma once

#include <glm/glm.hpp>
#include "OpenGL/OpenGLTexture.h"

namespace Flameberry {
    struct Material
    {
        glm::vec3 Albedo;
        float Roughness;
        bool Metallic;

        bool TextureMapEnabled = false;
        std::shared_ptr<OpenGLTexture> TextureMap;

        Material(
            const glm::vec3& albedo = glm::vec3(1.0f),
            float roughness = 0.2f,
            bool isMetal = false,
            bool textureMapEnabled = false,
            const std::shared_ptr<OpenGLTexture>& textureMap = nullptr
        )
            : Albedo(albedo), Roughness(roughness), Metallic(isMetal), TextureMapEnabled(textureMapEnabled), TextureMap(textureMap)
        {}
    };
}
