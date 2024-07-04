#include "OrthographicCamera.h"

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include "Core/Input.h"

namespace Flameberry {
	OrthographicCamera::OrthographicCamera(const glm::vec2& viewportSize,
		float zoom)
		: m_ViewportSize(viewportSize), m_Zoom(zoom), m_ViewMatrix(1.0f)
	{
		m_AspectRatio = m_ViewportSize.x / m_ViewportSize.y;
		m_ProjectionMatrix = glm::ortho(-m_AspectRatio * zoom, m_AspectRatio * zoom,
			-zoom, zoom, -1.0f, 1.0f);
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}

	OrthographicCamera::~OrthographicCamera() {}

	void OrthographicCamera::OnUpdate(float delta)
	{
		if (Input::IsKeyPressed(KeyCode::LeftAlt))
		{
			// Camera Movement
			if (Input::IsKeyPressed(KeyCode::Up))
				m_CameraPosition.y += m_CameraSpeed * delta;
			else if (Input::IsKeyPressed(KeyCode::Down))
				m_CameraPosition.y -= m_CameraSpeed * delta;
			else if (Input::IsKeyPressed(KeyCode::Right))
				m_CameraPosition.x += m_CameraSpeed * delta;
			else if (Input::IsKeyPressed(KeyCode::Left))
				m_CameraPosition.x -= m_CameraSpeed * delta;

			// Zoom Controls
			if (Input::IsKeyPressed(KeyCode::I))
				m_Zoom -= 0.025f;
			if (Input::IsKeyPressed(KeyCode::O))
				m_Zoom += 0.025f;
		}

		m_Zoom = glm::max(m_Zoom, 0.1f);
		m_CameraSpeed = m_Zoom * 2.5f;

		m_AspectRatio = m_ViewportSize.x / m_ViewportSize.y;
		m_ProjectionMatrix =
			glm::ortho(-m_AspectRatio * m_Zoom, m_AspectRatio * m_Zoom, -m_Zoom,
				m_Zoom, -1.0f, 1.0f);

		glm::mat4 transform =
			glm::translate(glm::mat4(1.0f), m_CameraPosition) * glm::rotate(glm::mat4(1.0f), glm::radians(m_CameraRotation), glm::vec3(0, 0, 1));
		m_ViewMatrix = glm::inverse(transform);

		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}
} // namespace Flameberry
