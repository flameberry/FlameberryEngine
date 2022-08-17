#pragma once
#include <glm/glm.hpp>

namespace Flameberry {
    class OrthographicCamera
    {
    public:
        OrthographicCamera() = default;
        OrthographicCamera(float aspectRatio, float zoom);
        ~OrthographicCamera();

        void SetAspectRatio(float aspectRatio) { m_AspectRatio = aspectRatio; }
        void SetPosition(const glm::vec3& position) { m_CameraPosition = position; }
        void SetRotation(float rotationAngle) { m_CameraRotation = rotationAngle; }

        void AddControls();
        void OnUpdate();
        void SetSensitivity(int value) { m_CameraSensitivity = value; }
        const glm::mat4& GetViewProjectionMatrix() const { return m_ViewProjectionMatrix; }
    private:
        float m_AspectRatio, m_Zoom = 1.0f;
        glm::mat4 m_ProjectionMatrix, m_ViewMatrix, m_ViewProjectionMatrix;
        glm::vec3 m_CameraPosition{ 0.0f, 0.0f, 0.0f };
        float m_CameraRotation = 0.0f;
        int m_CameraSensitivity = 25;
    };
}