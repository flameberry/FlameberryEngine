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

    bool IsAABBInsideFrustum(const AABB& aabb, const Frustum& frustum)
    {
        for (const auto& plane : frustum.Planes)
        {
            glm::vec3 normal(plane);
            float distance = plane.w;

            glm::vec3 positiveVertex = aabb.Min;
            if (normal.x >= 0) positiveVertex.x = aabb.Max.x;
            if (normal.y >= 0) positiveVertex.y = aabb.Max.y;
            if (normal.z >= 0) positiveVertex.z = aabb.Max.z;

            if (glm::dot(normal, positiveVertex) + distance < 0) {
                return false;
            }
        }
        return true;
    }

}