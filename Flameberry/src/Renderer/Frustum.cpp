#include "Frustum.h"

namespace Flameberry {

    void Frustum::ExtractFrustumPlanes(const glm::mat4& viewProjectionMatrix)
    {
        // Left plane
        this->Planes[0] = glm::vec4(
            viewProjectionMatrix[0][3] + viewProjectionMatrix[0][0],
            viewProjectionMatrix[1][3] + viewProjectionMatrix[1][0],
            viewProjectionMatrix[2][3] + viewProjectionMatrix[2][0],
            viewProjectionMatrix[3][3] + viewProjectionMatrix[3][0]
        );

        // Right plane
        this->Planes[1] = glm::vec4(
            viewProjectionMatrix[0][3] - viewProjectionMatrix[0][0],
            viewProjectionMatrix[1][3] - viewProjectionMatrix[1][0],
            viewProjectionMatrix[2][3] - viewProjectionMatrix[2][0],
            viewProjectionMatrix[3][3] - viewProjectionMatrix[3][0]
        );

        // Bottom plane
        this->Planes[2] = glm::vec4(
            viewProjectionMatrix[0][3] + viewProjectionMatrix[0][1],
            viewProjectionMatrix[1][3] + viewProjectionMatrix[1][1],
            viewProjectionMatrix[2][3] + viewProjectionMatrix[2][1],
            viewProjectionMatrix[3][3] + viewProjectionMatrix[3][1]
        );

        // Top plane
        this->Planes[3] = glm::vec4(
            viewProjectionMatrix[0][3] - viewProjectionMatrix[0][1],
            viewProjectionMatrix[1][3] - viewProjectionMatrix[1][1],
            viewProjectionMatrix[2][3] - viewProjectionMatrix[2][1],
            viewProjectionMatrix[3][3] - viewProjectionMatrix[3][1]
        );

        // Near plane
        this->Planes[4] = glm::vec4(
            viewProjectionMatrix[0][2],
            viewProjectionMatrix[1][2],
            viewProjectionMatrix[2][2],
            viewProjectionMatrix[3][2]
        );

        // Far plane
        this->Planes[5] = glm::vec4(
            viewProjectionMatrix[0][3] - viewProjectionMatrix[0][2],
            viewProjectionMatrix[1][3] - viewProjectionMatrix[1][2],
            viewProjectionMatrix[2][3] - viewProjectionMatrix[2][2],
            viewProjectionMatrix[3][3] - viewProjectionMatrix[3][2]
        );
    }

    bool IsAABBInsideFrustum(const AABB& aabb, const glm::mat4& transform, const Frustum& frustum)
    {
        std::array<glm::vec4, 8> vertices = {
            transform * glm::vec4(aabb.Min.x, aabb.Min.y, aabb.Min.z, 1.0f),
            transform * glm::vec4(aabb.Max.x, aabb.Min.y, aabb.Min.z, 1.0f),
            transform * glm::vec4(aabb.Max.x, aabb.Max.y, aabb.Min.z, 1.0f),
            transform * glm::vec4(aabb.Min.x, aabb.Max.y, aabb.Min.z, 1.0f),
            transform * glm::vec4(aabb.Min.x, aabb.Min.y, aabb.Max.z, 1.0f),
            transform * glm::vec4(aabb.Max.x, aabb.Min.y, aabb.Max.z, 1.0f),
            transform * glm::vec4(aabb.Max.x, aabb.Max.y, aabb.Max.z, 1.0f),
            transform * glm::vec4(aabb.Min.x, aabb.Max.y, aabb.Max.z, 1.0f)
        };

        for (const auto& plane : frustum.Planes)
        {
            bool allOutside = true;

            for (const auto& vertex : vertices)
            {
                if (glm::dot(glm::vec3(plane), glm::vec3(vertex)) + plane.w > 0)
                {
                    allOutside = false;
                    break;
                }
            }

            if (allOutside)
                return false;
        }

        return true;
    }

}