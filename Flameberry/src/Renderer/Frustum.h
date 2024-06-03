#pragma once

#include <array>

#include "AABB.h"

namespace Flameberry {

    struct Frustum
    {
        std::array<glm::vec4, 6> Planes;

        void ExtractFrustumPlanes(const glm::mat4& viewProjectionMatrix);
    };

    bool IsAABBInsideFrustum(const AABB& aabb, const Frustum& frustum);

}