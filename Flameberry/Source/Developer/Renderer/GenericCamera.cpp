#include "GenericCamera.h"

#include "Core/Core.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Flameberry {

	GenericCamera::GenericCamera()
		: m_ProjectionMatrix(1.0f), m_ViewMatrix(1.0f) {}

	void GenericCamera::SetOrthographic(float aspectRatio, float zoom, float near,
		float far)
	{
		m_CameraSettings.ProjectionType = ProjectionType::Orthographic;
		m_CameraSettings.AspectRatio = aspectRatio;
		m_CameraSettings.Zoom = zoom;
		m_CameraSettings.Near = near;
		m_CameraSettings.Far = far;

		RecalculateProjectionMatrix();
	}

	void GenericCamera::SetPerspective(float aspectRatio, float FOV, float near,
		float far)
	{
		m_CameraSettings.ProjectionType = ProjectionType::Perspective;
		m_CameraSettings.AspectRatio = aspectRatio;
		m_CameraSettings.FOV = FOV;
		m_CameraSettings.Near = near;
		m_CameraSettings.Far = far;

		RecalculateProjectionMatrix();
	}

	void GenericCamera::SetView(const glm::vec3& position,
		const glm::vec3& rotation)
	{
		glm::vec3 cameraDirection =
			glm::rotate(glm::quat(rotation), glm::vec3(0, 0, -1));
		m_ViewMatrix =
			glm::lookAt(position + cameraDirection, position, glm::vec3(0, 1, 0));
	}

	void GenericCamera::UpdateWithAspectRatio(float aspectRatio)
	{
		m_CameraSettings.AspectRatio = aspectRatio;

		RecalculateProjectionMatrix();
	}

	void GenericCamera::UpdateWithFOVorZoom(float FOV)
	{
		m_CameraSettings.FOV = FOV;

		RecalculateProjectionMatrix();
	}

	void GenericCamera::UpdateWithNear(float near)
	{
		m_CameraSettings.Near = near;

		RecalculateProjectionMatrix();
	}

	void GenericCamera::UpdateWithFar(float far)
	{
		m_CameraSettings.Far = far;

		RecalculateProjectionMatrix();
	}

	void GenericCamera::RecalculateProjectionMatrix()
	{
		switch (m_CameraSettings.ProjectionType)
		{
			case ProjectionType::Orthographic:
				m_ProjectionMatrix =
					glm::ortho(-m_CameraSettings.AspectRatio * m_CameraSettings.Zoom,
						m_CameraSettings.AspectRatio * m_CameraSettings.Zoom,
						-m_CameraSettings.Zoom, m_CameraSettings.Zoom,
						m_CameraSettings.Near, m_CameraSettings.Far);
				m_ProjectionMatrix[1][1] *= -1; // For Vulkan
				break;
			case ProjectionType::Perspective:
				m_ProjectionMatrix = glm::perspective(
					glm::radians(m_CameraSettings.FOV), m_CameraSettings.AspectRatio,
					m_CameraSettings.Near, m_CameraSettings.Far);
				m_ProjectionMatrix[1][1] *= -1; // For Vulkan
				break;
		}
	}

} // namespace Flameberry
