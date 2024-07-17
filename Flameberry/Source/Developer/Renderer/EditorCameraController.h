#pragma once

#include <memory>

#include "PerspectiveCamera.h"
#include "Core/Event.h"
#include "Core/Core.h"

namespace Flameberry {

	class EditorCameraController
	{
	public:
		EditorCameraController(const PerspectiveCameraSpecification& specification);
		~EditorCameraController();

		bool OnUpdate(float delta);
		void OnEvent(const Event& e);

		Ref<PerspectiveCamera> GetPerspectiveCamera() const { return m_Camera; }

	private:
		Ref<PerspectiveCamera> m_Camera;
		glm::vec3 m_RightDirection, m_UpDirection{ 0, 1, 0 };
		glm::vec2 m_LastMousePosition;
		glm::vec3 m_FocalPoint{ 0.0f };
	};

} // namespace Flameberry
