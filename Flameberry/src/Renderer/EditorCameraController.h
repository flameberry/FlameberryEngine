#pragma once

#include <memory>
#include "PerspectiveCamera.h"

namespace Flameberry {
    class EditorCameraController
    {
    public:
        EditorCameraController(const PerspectiveCameraSpecification& specification);
        ~EditorCameraController();

        bool OnUpdate(float delta);

        std::shared_ptr<PerspectiveCamera> GetPerspectiveCamera() const { return m_Camera; }
    private:
        std::shared_ptr<PerspectiveCamera> m_Camera;
        glm::vec3 m_RightDirection, m_UpDirection{ 0, 1, 0 };
        glm::vec2 m_LastMousePosition;
    };
}