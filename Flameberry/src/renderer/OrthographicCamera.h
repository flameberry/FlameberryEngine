#pragma once
#include <glm/glm.hpp>

namespace Flameberry {
    class OrthographicCamera
    {
    public:
        OrthographicCamera(float aspectRatio, float zoom);
        ~OrthographicCamera();

        void SetAspectRatio(float aspectRatio) { M_AspectRatio = aspectRatio; }
        void SetPosition(const glm::vec3& position) { M_CameraPosition = position; }
        void SetRotation(float rotationAngle) { M_CameraRotation = rotationAngle; }

        void AddControls();

        void SetSensitivity(int value) { M_CameraSensitivity = value; }

        void OnUpdate();
        const glm::mat4& GetViewProjectionMatrix() const { return M_ViewProjectionMatrix; }
    private:
        float M_AspectRatio, M_Zoom = 1.0f;
        glm::mat4 M_ProjectionMatrix, M_ViewMatrix, M_ViewProjectionMatrix;
        glm::vec3 M_CameraPosition{ 0.0f, 0.0f, 0.0f };
        float M_CameraRotation = 0.0f;
        int M_CameraSensitivity = 25;
    };
}