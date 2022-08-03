#include "OrthographicCamera.h"
#include <glm/gtc/matrix_transform.hpp>
#include "../Core/Input.h"
#include "Renderer2D.h"

namespace Flameberry {
    OrthographicCamera::OrthographicCamera(float aspectRatio, float zoom)
        : m_ProjectionMatrix(glm::ortho(-aspectRatio * zoom, aspectRatio* zoom, -zoom, zoom, -1.0f, 1.0f)), m_ViewMatrix(1.0f)
    {
        m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
    }

    void OrthographicCamera::AddControls()
    {
        // Camera Movement
        if (Input::IsKey(GLFW_KEY_W, GLFW_PRESS))
            m_CameraPosition.y += Renderer2D::ConvertYAxisPixelValueToOpenGLValue(m_CameraSensitivity);
        else if (Input::IsKey(GLFW_KEY_S, GLFW_PRESS))
            m_CameraPosition.y -= Renderer2D::ConvertYAxisPixelValueToOpenGLValue(m_CameraSensitivity);
        else if (Input::IsKey(GLFW_KEY_D, GLFW_PRESS))
            m_CameraPosition.x += Renderer2D::ConvertXAxisPixelValueToOpenGLValue(m_CameraSensitivity);
        else if (Input::IsKey(GLFW_KEY_A, GLFW_PRESS))
            m_CameraPosition.x -= Renderer2D::ConvertXAxisPixelValueToOpenGLValue(m_CameraSensitivity);
    }

    OrthographicCamera::~OrthographicCamera()
    {
    }

    void OrthographicCamera::OnUpdate()
    {
        AddControls();

        m_ProjectionMatrix = glm::ortho(-m_AspectRatio * m_Zoom, m_AspectRatio * m_Zoom, -m_Zoom, m_Zoom, -1.0f, 1.0f);

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_CameraPosition) * glm::rotate(glm::mat4(1.0f), glm::radians(m_CameraRotation), glm::vec3(0, 0, 1));
        m_ViewMatrix = glm::inverse(transform);

        m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
    }
}