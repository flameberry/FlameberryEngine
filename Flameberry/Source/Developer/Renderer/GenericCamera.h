#pragma once

#include <glm/glm.hpp>

namespace Flameberry {

    enum class ProjectionType : uint8_t { Orthographic = 0, Perspective };

    struct GenericCameraSettings
    {
        Flameberry::ProjectionType ProjectionType = ProjectionType::Perspective;
        float AspectRatio, Near, Far;
        union { float Zoom, FOV; };
    };

    class GenericCamera
    {
    public:
        GenericCamera();

        void SetProjectionType(ProjectionType projectionType) { m_CameraSettings.ProjectionType = projectionType; }
        void SetOrthographic(float aspectRatio, float zoom, float near, float far);
        void SetPerspective(float aspectRatio, float FOV, float near, float far);
        void SetView(const glm::vec3& position, const glm::vec3& rotation);

        void UpdateWithFOVorZoom(float FOV);
        void UpdateWithAspectRatio(float aspectRatio);
        void UpdateWithNear(float near);
        void UpdateWithFar(float far);

        glm::mat4 CreateViewProjectionMatrix() const { return m_ProjectionMatrix * m_ViewMatrix; }

        const GenericCameraSettings& GetSettings() const { return m_CameraSettings; }
        const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
        const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
    private:
        void RecalculateProjectionMatrix();
        void RecalculateViewMatrix();
    private:
        GenericCameraSettings m_CameraSettings;
        glm::mat4 m_ProjectionMatrix, m_ViewMatrix;
    };

}
