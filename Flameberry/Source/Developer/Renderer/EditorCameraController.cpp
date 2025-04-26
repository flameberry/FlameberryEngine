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
		: m_Position(position), m_Direction(direction), m_Velocity(0.0f)
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
		glm::vec2 mouseDelta = (mousePos - m_LastMousePosition);
		m_LastMousePosition = mousePos;

		if (!Input::IsMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT))
		{
			Input::SetCursorMode(GLFW_CURSOR_NORMAL);
			return false;
		}

		Input::SetCursorMode(GLFW_CURSOR_DISABLED);

		const float baseMoveSpeed = 10.5f;
		const float acceleration = 15.0f;		   // How fast you reach full speed
		const float damping = 10.0f;			   // How quickly you slow down
		const float rotationSpeed = 0.6f * 0.002f; // You had 0.002f, so move it here

		float currentMoveSpeed = baseMoveSpeed;
		if (Input::IsKeyPressed(KeyCode::LeftShift) || Input::IsKeyPressed(KeyCode::RightShift))
			currentMoveSpeed *= 2.0f;

		// Movement Input
		glm::vec3 moveInput(0.0f);
		if (Input::IsKeyPressed(KeyCode::W))
			moveInput += m_Direction;
		if (Input::IsKeyPressed(KeyCode::S))
			moveInput -= m_Direction;
		if (Input::IsKeyPressed(KeyCode::D))
			moveInput += m_RightDirection;
		if (Input::IsKeyPressed(KeyCode::A))
			moveInput -= m_RightDirection;
		if (Input::IsKeyPressed(KeyCode::E))
			moveInput += m_UpDirection;
		if (Input::IsKeyPressed(KeyCode::Q))
			moveInput -= m_UpDirection;

		bool moved = false;
		if (glm::length(moveInput) > 0.0f)
		{
			moveInput = glm::normalize(moveInput);
			m_Velocity += moveInput * currentMoveSpeed * acceleration * delta;
			moved = true;
		}
		else
		{
			// Apply damping when no input
			m_Velocity -= m_Velocity * glm::min(damping * delta, 1.0f);
		}

		// Actually move
		m_Position += m_Velocity * delta;

		// Rotation Input
		if (mouseDelta.x != 0.0f || mouseDelta.y != 0.0f)
		{
			const float pitchDelta = -mouseDelta.y * rotationSpeed;
			const float yawDelta = -mouseDelta.x * rotationSpeed;

			glm::quat q = glm::normalize(glm::cross(glm::angleAxis(pitchDelta, m_RightDirection),
				glm::angleAxis(yawDelta, m_UpDirection)));
			m_Direction = glm::rotate(q, m_Direction);
			moved = true;
		}

		if (moved || glm::length(m_Velocity) > 0.001f)
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
