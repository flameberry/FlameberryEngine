#pragma once

#include <glm/glm.hpp>

namespace Flameberry {
    struct PerspectiveCameraSpecification
    {
        glm::vec3 Position, Direction;
        float AspectRatio, FOV, zNear, zFar;

        PerspectiveCameraSpecification()
            : Position(0, 0, 0), Direction(0, 0, -1), AspectRatio(1280.0f / 720.0f), FOV(45.0f), zNear(0.1f), zFar(1000.0f)
        {}

        PerspectiveCameraSpecification(const glm::vec3& position, const glm::vec3& direction, float aspect, float fov, float near, float far)
            : Position(position), Direction(direction), AspectRatio(aspect), FOV(fov), zNear(near), zFar(far)
        {}
    };

    class PerspectiveCamera
    {
    public:
        PerspectiveCamera() = default;
        PerspectiveCamera(const PerspectiveCameraSpecification& specification);
        ~PerspectiveCamera();

        void OnResize(float aspectRatio);
        void Invalidate();

        const glm::mat4& GetViewProjectionMatrix() const { return m_ViewProjectionMatrix; }
        const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
        const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }

        PerspectiveCameraSpecification GetSpecification() const { return m_CameraSpec; }
    private:
        PerspectiveCameraSpecification m_CameraSpec;
        glm::mat4 m_ProjectionMatrix, m_ViewMatrix, m_ViewProjectionMatrix;

        friend class EditorCameraController;
    };
}
