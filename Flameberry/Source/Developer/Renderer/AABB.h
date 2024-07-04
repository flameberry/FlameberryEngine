#pragma once

#include <glm/glm.hpp>

namespace Flameberry {

	struct AABB
	{
		glm::vec3 Min, Max;

		AABB() = default;

		AABB(const glm::vec3& min, const glm::vec3& max)
			: Min(min), Max(max) {}
	};

} // namespace Flameberry