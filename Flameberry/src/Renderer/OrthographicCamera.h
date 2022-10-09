#pragma once
#include <glm/glm.hpp>

namespace Flameberry {
    class OrthographicCamera
    {
    public:
        OrthographicCamera() = default;
        OrthographicCamera(const glm::vec2& viewportSize, float zoom);
        ~OrthographicCamera();

        void SetViewportSize(const glm::vec2& viewportSize) { m_ViewportSize = viewportSize; m_AspectRatio = m_ViewportSize.x / m_ViewportSize.y; }
        void SetPosition(const glm::vec3& position) { m_CameraPosition = position; }
        void SetRotation(float rotationAngle) { m_CameraRotation = rotationAngle; }

        void OnUpdate(float delta);
        void SetSpeed(int value) { m_CameraSpeed = value; }
        const glm::mat4& GetViewProjectionMatrix() const { return m_ViewProjectionMatrix; }
    private:
        float m_AspectRatio, m_Zoom = 1.0f;
        glm::mat4 m_ProjectionMatrix, m_ViewMatrix, m_ViewProjectionMatrix;
        glm::vec2 m_ViewportSize;
        glm::vec3 m_CameraPosition{ 0.0f, 0.0f, 0.0f };
        float m_CameraRotation = 0.0f;
        float m_CameraSpeed = 2.5f;
    };
}
