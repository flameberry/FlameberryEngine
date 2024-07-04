#pragma once

#include <unordered_map>

#include "Core/Core.h"
#include "Shader.h"

namespace Flameberry {

	// Stores the references to all shaders loaded into memory
	// Advised to only use during the initialization phase and should be avoided to be used per frame
	// This is because to access any shader from here std::string must be hashed
	class ShaderLibrary
	{
	public:
		static void Init();
		static void Shutdown();

		static Ref<Shader> Get(const std::string& shaderName);

	private:
		// Is this a sign of laziness to not make this a const char* ?
		static const std::filesystem::path					s_ShaderBaseDirectory;
		static std::unordered_map<std::string, Ref<Shader>> s_ShaderStorage;
	};

} // namespace Flameberry