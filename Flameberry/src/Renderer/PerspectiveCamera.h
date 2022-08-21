#pragma once

#include <glm/glm.hpp>

namespace Flameberry {
    struct PerspectiveCameraInfo
    {
        glm::vec3 cameraPostion, cameraDirection;
        float aspectRatio, FOV, zNear, zFar;

        PerspectiveCameraInfo()
            : cameraPostion(0, 0, 0), cameraDirection(0, 0, -1), aspectRatio(1280.0f / 720.0f), FOV(45.0f), zNear(0.1f), zFar(100.0f)
        {
        }
    };

    class PerspectiveCamera
    {
    public:
        PerspectiveCamera() = default;
        PerspectiveCamera(const PerspectiveCameraInfo& cameraInfo);
        ~PerspectiveCamera();
        void OnUpdate();
        glm::mat4 GetViewProjectionMatrix() const { return m_ViewProjectionMatrix; }
        void SetAspectRatio(float aspectRatio) { m_AspectRatio = aspectRatio; }
    private:
        glm::mat4 m_ProjectionMatrix, m_ViewMatrix, m_ViewProjectionMatrix;
        glm::vec3 m_CameraPosition, m_CameraDirection;
        float m_AspectRatio, m_FOV, m_ZNear, m_ZFar;
    };
}