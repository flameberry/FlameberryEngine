#pragma once

#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

namespace Flameberry::Math {

	void DecomposeViewMatrix(const glm::mat4& viewMatrix, glm::vec3& eye, glm::vec3& lookDirection, glm::vec3& up);
	bool DecomposeTransform(const glm::mat4& transform, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale);
}