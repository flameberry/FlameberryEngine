#include "PerspectiveCamera.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Flameberry {
	PerspectiveCamera::PerspectiveCamera(
		const PerspectiveCameraSpecification& specification)
		: m_CameraSpec(specification)
	{
		Invalidate();
	}

	PerspectiveCamera::~PerspectiveCamera() {}

	void PerspectiveCamera::OnResize(float aspectRatio)
	{
		m_CameraSpec.AspectRatio = aspectRatio;
		Invalidate();
	}

	void PerspectiveCamera::Invalidate()
	{
		m_ViewMatrix = glm::lookAt(m_CameraSpec.Position,
			m_CameraSpec.Position + m_CameraSpec.Direction,
			glm::vec3(0, 1, 0));
		m_ProjectionMatrix =
			glm::perspective(glm::radians(m_CameraSpec.FOV), m_CameraSpec.AspectRatio,
				m_CameraSpec.zNear, m_CameraSpec.zFar);
		m_ProjectionMatrix[1][1] *= -1;
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}
} // namespace Flameberry
