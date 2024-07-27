#pragma once

#include <memory>

#include "Core/Event.h"
#include "Core/Core.h"

#include "GenericCamera.h"

namespace Flameberry {

	class EditorCameraController
	{
	public:
		EditorCameraController(const glm::vec3& position, const glm::vec3& direction, const GenericCameraSettings& settings);
		~EditorCameraController();

		bool OnUpdate(float delta);
		void OnResize(float aspectRatio);
		void OnEvent(const Event& e);

		inline GenericCamera& GetCamera() { return m_Camera; }
		inline const glm::vec3& GetPosition() const { return m_Position; }
		inline const glm::vec3& GetDirection() const { return m_Direction; }

		inline void SetPosition(const glm::vec3& position) { m_Position = position; }
		inline void SetDirection(const glm::vec3& direction) { m_Direction = direction; }

		void SetView(const glm::vec3& position, const glm::vec3& direction);

	private:
		bool OnUpdatePerspective(float delta);
		bool OnUpdateOrthographic(float delta);

	private:
		glm::vec3 m_Position, m_Direction;
		GenericCamera m_Camera;

		glm::vec3 m_RightDirection, m_UpDirection{ 0, 1, 0 };
		glm::vec2 m_LastMousePosition;
		glm::vec3 m_FocalPoint{ 0.0f };
	};

} // namespace Flameberry
