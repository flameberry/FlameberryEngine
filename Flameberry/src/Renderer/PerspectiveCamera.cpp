#include "PerspectiveCamera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <GLFW/glfw3.h> 

#include "Core/Input.h"
#include "Core/Core.h"

namespace Flameberry {
    PerspectiveCamera::PerspectiveCamera(const PerspectiveCameraInfo& cameraInfo)
        : m_AspectRatio(cameraInfo.aspectRatio),
        m_FOV(cameraInfo.FOV),
        m_ZNear(cameraInfo.zNear),
        m_ZFar(cameraInfo.zFar),
        m_CameraPosition(cameraInfo.cameraPostion),
        m_CameraDirection(cameraInfo.cameraDirection),
        m_UpDirection(glm::vec3(0, 1, 0))
    {
        Invalidate();
    }

    PerspectiveCamera::~PerspectiveCamera()
    {
    }

    void PerspectiveCamera::OnResize(float aspectRatio)
    {
        if (m_AspectRatio != aspectRatio)
        {
            m_AspectRatio = aspectRatio;
            Invalidate();
        }
    }

    bool PerspectiveCamera::OnUpdate(float delta)
    {
        glm::vec2 mousePos = Input::GetCursorPosition();
        glm::vec2 rotationDelta = (mousePos - m_LastMousePosition) * 0.002f;
        m_LastMousePosition = mousePos;

        if (!Input::IsMouseButton(GLFW_MOUSE_BUTTON_RIGHT, GLFW_PRESS))
        {
            Input::SetCursorMode(GLFW_CURSOR_NORMAL);
            return false;
        }

        Input::SetCursorMode(GLFW_CURSOR_DISABLED);

        bool moved = false;
        float speed = 10.5f, rotationSpeed = 0.6f;
        if (Input::IsMouseButton(GLFW_MOUSE_BUTTON_RIGHT, GLFW_PRESS))
        {
            if (Input::IsKey(GLFW_KEY_W, GLFW_PRESS))
            {
                m_CameraPosition += m_CameraDirection * speed * delta;
                moved = true;
            }
            if (Input::IsKey(GLFW_KEY_S, GLFW_PRESS))
            {
                m_CameraPosition -= m_CameraDirection * speed * delta;
                moved = true;
            }
            if (Input::IsKey(GLFW_KEY_A, GLFW_PRESS))
            {
                m_CameraPosition -= m_RightDirection * speed * delta;
                moved = true;
            }
            if (Input::IsKey(GLFW_KEY_D, GLFW_PRESS))
            {
                m_CameraPosition += m_RightDirection * speed * delta;
                moved = true;
            }
            if (Input::IsKey(GLFW_KEY_Q, GLFW_PRESS))
            {
                m_CameraPosition -= m_UpDirection * speed * delta;
                moved = true;
            }
            if (Input::IsKey(GLFW_KEY_E, GLFW_PRESS))
            {
                m_CameraPosition += m_UpDirection * speed * delta;
                moved = true;
            }
        }

        if (rotationDelta.x != 0.0f || rotationDelta.y != 0.0f)
        {
            float pitchDelta = rotationDelta.y * rotationSpeed;
            float yawDelta = rotationDelta.x * rotationSpeed;

            m_RightDirection = glm::cross(m_UpDirection, -m_CameraDirection);
            glm::quat q = glm::normalize(glm::cross(glm::angleAxis(-pitchDelta, m_RightDirection), glm::angleAxis(-yawDelta, glm::vec3(0.0f, 1.0f, 0.0f))));
            m_CameraDirection = glm::rotate(q, m_CameraDirection);

            moved = true;
        }

        if (moved)
            Invalidate();
        return true;
    }

    void PerspectiveCamera::Invalidate()
    {
        m_RightDirection = glm::cross(m_UpDirection, -m_CameraDirection);
        m_ViewMatrix = glm::lookAt(m_CameraPosition, m_CameraPosition + m_CameraDirection, m_UpDirection);
        m_ProjectionMatrix = glm::perspective(glm::radians(m_FOV), m_AspectRatio, m_ZNear, m_ZFar);
        m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
    }
}