#include "EditorCameraController.h"

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Core/Core.h"
#include "Core/Input.h"

namespace Flameberry {

	EditorCameraController::EditorCameraController(const glm::vec3& position, const glm::vec3& direction, const GenericCameraSettings& settings)
		: m_Position(position), m_Direction(direction)
	{
		switch (settings.ProjectionType)
		{
			case ProjectionType::Orthographic:
				m_Camera.SetOrthographic(settings.AspectRatio, settings.Zoom, settings.Near, settings.Far);
				break;
			case ProjectionType::Perspective:
				m_Camera.SetPerspective(settings.AspectRatio, settings.FOV, settings.Near, settings.Far);
				break;
		}
		m_Camera.SetView_Direction(m_Position, m_Direction);
	}

	EditorCameraController::~EditorCameraController()
	{
	}

	bool EditorCameraController::OnUpdate(float delta)
	{
		switch (m_Camera.GetSettings().ProjectionType)
		{
			case ProjectionType::Perspective:
				return OnUpdatePerspective(delta);
			case ProjectionType::Orthographic:
				return OnUpdateOrthographic(delta);
		}
	}

	void EditorCameraController::SetView(const glm::vec3& position, const glm::vec3& direction)
	{
		m_Position = position;
		m_Direction = direction;
		m_Camera.SetView_Direction(m_Position, m_Direction);
	}

	bool EditorCameraController::OnUpdatePerspective(float delta)
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
		if (Input::IsKeyPressed(KeyCode::LeftShift) || Input::IsKeyPressed(KeyCode::RightShift))
			speed *= 2.0f;

		bool moved = false;
		if (Input::IsKeyPressed(KeyCode::W))
		{
			m_Position += m_Direction * speed * delta;
			moved = true;
		}
		if (Input::IsKeyPressed(KeyCode::S))
		{
			m_Position -= m_Direction * speed * delta;
			moved = true;
		}
		if (Input::IsKeyPressed(KeyCode::A))
		{
			m_Position -= m_RightDirection * speed * delta;
			moved = true;
		}
		if (Input::IsKeyPressed(KeyCode::D))
		{
			m_Position += m_RightDirection * speed * delta;
			moved = true;
		}
		if (Input::IsKeyPressed(KeyCode::Q))
		{
			m_Position -= m_UpDirection * speed * delta;
			moved = true;
		}
		if (Input::IsKeyPressed(KeyCode::E))
		{
			m_Position += m_UpDirection * speed * delta;
			moved = true;
		}

		if (rotationDelta.x != 0.0f || rotationDelta.y != 0.0f)
		{
			const float pitchDelta = rotationDelta.y * rotationSpeed;
			const float yawDelta = rotationDelta.x * rotationSpeed;

			glm::quat q = glm::normalize(glm::cross(glm::angleAxis(-pitchDelta, m_RightDirection), glm::angleAxis(-yawDelta, m_UpDirection)));
			m_Direction = glm::rotate(q, m_Direction);
			moved = true;
		}

		if (moved)
		{
			m_RightDirection = glm::cross(m_UpDirection, -m_Direction);
			m_Camera.SetView_Direction(m_Position, m_Direction);
		}
		return true;
	}

	bool EditorCameraController::OnUpdateOrthographic(float delta)
	{
		bool moved = false;
		float m_CameraSpeed = 20.0f;
		static float m_Zoom = 10.0f;

		// Camera Movement
		if (Input::IsKeyPressed(KeyCode::W))
		{
			m_Position.y += m_CameraSpeed * delta;
			moved = true;
		}
		else if (Input::IsKeyPressed(KeyCode::S))
		{
			m_Position.y -= m_CameraSpeed * delta;
			moved = true;
		}
		else if (Input::IsKeyPressed(KeyCode::D))
		{
			m_Position.x += m_CameraSpeed * delta;
			moved = true;
		}
		else if (Input::IsKeyPressed(KeyCode::A))
		{
			m_Position.x -= m_CameraSpeed * delta;
			moved = true;
		}

		bool zoomed = false;

		// Zoom Controls
		if (Input::IsKeyPressed(KeyCode::I))
		{
			m_Zoom -= 0.025f;
			zoomed = true;
		}
		if (Input::IsKeyPressed(KeyCode::O))
		{
			m_Zoom += 0.025f;
			zoomed = true;
		}

		m_Zoom = glm::max(m_Zoom, 0.1f);
		m_CameraSpeed = m_Zoom * 2.5f;

		if (zoomed)
			m_Camera.UpdateWithFOVorZoom(m_Zoom);
		if (moved)
			m_Camera.SetView_Direction(m_Position, m_Direction);

		return moved;
	}

	void EditorCameraController::OnResize(float aspectRatio)
	{
		m_Camera.UpdateWithAspectRatio(aspectRatio);
	}

	void EditorCameraController::OnEvent(const Event& e)
	{
		switch (e.GetType())
		{
			case EventType::MouseScrolled:
				break;
		}
	}

} // namespace Flameberry
