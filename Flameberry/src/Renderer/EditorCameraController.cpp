#include "EditorCameraController.h"

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Core/Input.h"

namespace Flameberry {
    EditorCameraController::EditorCameraController(const PerspectiveCameraSpecification& specification)
        : m_Camera(std::make_shared<PerspectiveCamera>(specification))
    {
    }

    EditorCameraController::~EditorCameraController()
    {
    }

    bool EditorCameraController::OnUpdate(float delta)
    {
        glm::vec2 mousePos = Input::GetCursorPosition();
        glm::vec2 rotationDelta = (mousePos - m_LastMousePosition) * 0.002f;
        m_LastMousePosition = mousePos;

        if (!Input::IsMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT))
        {
            Input::SetCursorMode(GLFW_CURSOR_NORMAL);
            return false;
        }

        Input::SetCursorMode(GLFW_CURSOR_DISABLED);

        float speed = 10.5f, rotationSpeed = 0.6f;
        if (Input::IsKeyPressed(GLFW_KEY_LEFT_SHIFT) || Input::IsKeyPressed(GLFW_KEY_RIGHT_SHIFT))
            speed *= 2.0f;

        bool moved = false;
        auto& cameraSpec = m_Camera->m_CameraSpec;
        if (Input::IsMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT))
        {
            if (Input::IsKeyPressed(GLFW_KEY_W))
            {
                cameraSpec.Position += cameraSpec.Direction * speed * delta;
                moved = true;
            }
            if (Input::IsKeyPressed(GLFW_KEY_S))
            {
                cameraSpec.Position -= cameraSpec.Direction * speed * delta;
                moved = true;
            }
            if (Input::IsKeyPressed(GLFW_KEY_A))
            {
                cameraSpec.Position -= m_RightDirection * speed * delta;
                moved = true;
            }
            if (Input::IsKeyPressed(GLFW_KEY_D))
            {
                cameraSpec.Position += m_RightDirection * speed * delta;
                moved = true;
            }
            if (Input::IsKeyPressed(GLFW_KEY_Q))
            {
                cameraSpec.Position -= m_UpDirection * speed * delta;
                moved = true;
            }
            if (Input::IsKeyPressed(GLFW_KEY_E))
            {
                cameraSpec.Position += m_UpDirection * speed * delta;
                moved = true;
            }
        }

        if (rotationDelta.x != 0.0f || rotationDelta.y != 0.0f)
        {
            float pitchDelta = rotationDelta.y * rotationSpeed;
            float yawDelta = rotationDelta.x * rotationSpeed;

            glm::quat q = glm::normalize(
                glm::cross(
                    glm::angleAxis(-pitchDelta, m_RightDirection),
                    glm::angleAxis(-yawDelta, m_UpDirection)
                )
            );
            cameraSpec.Direction = glm::rotate(q, cameraSpec.Direction);

            moved = true;
        }

        if (moved)
        {
            m_RightDirection = glm::cross(m_UpDirection, -cameraSpec.Direction);
            m_Camera->Invalidate();
        }

        return true;
    }

    void EditorCameraController::OnEvent(const Event& e)
    {
        switch (e.GetType()) {
            case EventType::MouseScrolled: {
                // auto event = *(MouseScrollEvent*)(&e);

                // auto& cameraSpec = m_Camera->m_CameraSpec;
                // glm::vec2 delta(event.OffsetX, event.OffsetY);
                break;
            }
        }
    }
}
