#include "OrthographicCamera.h"
#include <glm/gtc/matrix_transform.hpp>
#include "../core/Input.h"
#include "Renderer2D.h"

namespace Flameberry {
    OrthographicCamera::OrthographicCamera(float aspectRatio, float zoom)
        : M_ProjectionMatrix(glm::ortho(-aspectRatio * zoom, aspectRatio* zoom, -zoom, zoom, -1.0f, 1.0f)), M_ViewMatrix(1.0f)
    {
        M_ViewProjectionMatrix = M_ProjectionMatrix * M_ViewMatrix;
    }

    void OrthographicCamera::AddControls()
    {
        // Camera Movement
        if (Input::IsKey(GLFW_KEY_W, GLFW_PRESS))
            M_CameraPosition.y += Renderer2D::ConvertYAxisPixelValueToOpenGLValue(M_CameraSensitivity);
        else if (Input::IsKey(GLFW_KEY_S, GLFW_PRESS))
            M_CameraPosition.y -= Renderer2D::ConvertYAxisPixelValueToOpenGLValue(M_CameraSensitivity);
        else if (Input::IsKey(GLFW_KEY_D, GLFW_PRESS))
            M_CameraPosition.x += Renderer2D::ConvertXAxisPixelValueToOpenGLValue(M_CameraSensitivity);
        else if (Input::IsKey(GLFW_KEY_A, GLFW_PRESS))
            M_CameraPosition.x -= Renderer2D::ConvertXAxisPixelValueToOpenGLValue(M_CameraSensitivity);
    }

    OrthographicCamera::~OrthographicCamera()
    {
    }

    void OrthographicCamera::OnUpdate()
    {
        AddControls();

        M_ProjectionMatrix = glm::ortho(-M_AspectRatio * M_Zoom, M_AspectRatio * M_Zoom, -M_Zoom, M_Zoom, -1.0f, 1.0f);

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), M_CameraPosition) * glm::rotate(glm::mat4(1.0f), glm::radians(M_CameraRotation), glm::vec3(0, 0, 1));
        M_ViewMatrix = glm::inverse(transform);

        M_ViewProjectionMatrix = M_ProjectionMatrix * M_ViewMatrix;
    }
}