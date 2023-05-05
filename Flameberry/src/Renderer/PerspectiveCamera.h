#pragma once

#include <glm/glm.hpp>

namespace Flameberry {
    struct PerspectiveCameraInfo
    {
        glm::vec3 cameraPostion, cameraDirection;
        float aspectRatio, FOV, zNear, zFar;

        PerspectiveCameraInfo()
            : cameraPostion(0, 0, 0), cameraDirection(0, 0, -1), aspectRatio(1280.0f / 720.0f), FOV(45.0f), zNear(0.1f), zFar(1000.0f)
        {}
    };

    class PerspectiveCamera
    {
    public:
        PerspectiveCamera() = default;
        PerspectiveCamera(const PerspectiveCameraInfo& cameraInfo);
        ~PerspectiveCamera();
        bool OnUpdate(float delta);
        void OnResize(float aspectRatio);
        const glm::mat4& GetViewProjectionMatrix() const { return m_ViewProjectionMatrix; }
        const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
        const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
        void SetAspectRatio(float aspectRatio) { m_AspectRatio = aspectRatio; }
        glm::vec3 GetPosition() const { return m_CameraPosition; }
    private:
        void Invalidate();
    private:
        glm::mat4 m_ProjectionMatrix, m_ViewMatrix, m_ViewProjectionMatrix;
        glm::vec3 m_CameraPosition, m_CameraDirection, m_RightDirection, m_UpDirection;
        float m_AspectRatio, m_FOV, m_ZNear, m_ZFar;
        glm::vec2 m_LastMousePosition;
    };
}
