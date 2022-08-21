#include "PerspectiveCamera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Flameberry {
    PerspectiveCamera::PerspectiveCamera(const PerspectiveCameraInfo& cameraInfo)
        : m_AspectRatio(cameraInfo.aspectRatio), m_FOV(cameraInfo.FOV), m_ZNear(cameraInfo.zNear), m_ZFar(cameraInfo.zFar), m_CameraPosition(cameraInfo.cameraPostion), m_CameraDirection(cameraInfo.cameraDirection)
    {
        m_ViewMatrix = glm::lookAt(m_CameraPosition, m_CameraPosition + m_CameraDirection, glm::vec3(0, 1, 0));
        m_ProjectionMatrix = glm::perspective(glm::radians(m_FOV), m_AspectRatio, m_ZNear, m_ZFar);
        m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
    }

    PerspectiveCamera::~PerspectiveCamera()
    {
    }

    void PerspectiveCamera::OnUpdate()
    {
        m_ViewMatrix = glm::lookAt(m_CameraPosition, m_CameraPosition + m_CameraDirection, glm::vec3(0, 1, 0));
        m_ProjectionMatrix = glm::perspective(glm::radians(m_FOV), m_AspectRatio, 0.1f, 100.0f);
        m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
    }
}