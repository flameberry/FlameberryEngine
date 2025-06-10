#pragma once

#include <memory>

#include "Core/Event.h"
#include "Core/Core.h"

#include "AABB.h"
#include "GenericCamera.h"

namespace Flameberry {

	struct TransformComponent;

	class EditorCameraController
	{
	public:
		EditorCameraController(const glm::vec3& position, const glm::vec3& direction, const GenericCameraSettings& settings);
		~EditorCameraController();

		bool OnUpdate(float delta);
		void OnResize(float aspectRatio);
		void OnEvent(const Event& e);

		inline GenericCamera& GetCamera() { return m_Camera; }
		[[nodiscard]] inline const glm::vec3& GetPosition() const { return m_Position; }
		[[nodiscard]] inline const glm::vec3& GetDirection() const { return m_Direction; }

		inline void SetPosition(const glm::vec3& position) { m_Position = position; }
		inline void SetDirection(const glm::vec3& direction) { m_Direction = direction; }

		void SetView(const glm::vec3& position, const glm::vec3& direction);

		// Movement Utilities
		void FrameEntity(const TransformComponent& transform);
		void FrameEntity(const TransformComponent& transform, const AABB& aabb);

	private:
		bool OnUpdatePerspective(float delta);
		bool OnUpdateOrthographic(float delta);

	private:
		glm::vec3 m_Position, m_Direction, m_Velocity;
		GenericCamera m_Camera;

		// Variables for framing
		bool m_ShouldFrame = false;
		glm::vec3 m_TargetPosition, m_TargetFocusPoint;

		glm::vec3 m_RightDirection = { 0, 0, 0 }, m_UpDirection{ 0, 1, 0 };
		glm::vec2 m_LastMousePosition = { 0, 0 };
		glm::vec3 m_FocalPoint{ 0.0f };
	};

} // namespace Flameberry
