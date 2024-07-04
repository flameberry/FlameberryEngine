#pragma once

#include <glm/glm.hpp>

namespace Flameberry {

	struct DirectionalLight
	{
		alignas(16) glm::vec3 Direction{ 0.0f };
		alignas(16) glm::vec3 Color{ 0.0f };
		alignas(4) float Intensity = 0.0f, LightSize = 0.0f;
	};

	struct PointLight
	{
		alignas(16) glm::vec3 Position;
		alignas(16) glm::vec3 Color;
		alignas(4) float Intensity;
	};

} // namespace Flameberry
