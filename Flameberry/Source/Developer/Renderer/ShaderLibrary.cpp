#include "ShaderLibrary.h"

namespace Flameberry {

	std::unordered_map<std::string, Ref<Shader>> ShaderLibrary::s_ShaderStorage;
	const std::filesystem::path ShaderLibrary::s_ShaderBaseDirectory =
		FBY_PROJECT_DIR "Flameberry/shaders/vulkan/bin";

	void ShaderLibrary::Init()
	{
		// Currently this is hardcoded as no new shaders are loaded dynamically at
		// runtime
		const char* paths[][2] = {
			{ "Flameberry_PBR.vert.spv", "Flameberry_PBR.frag.spv" },
			{ "Flameberry_MousePicking.vert.spv", "Flameberry_MousePicking.frag.spv" },
			{ "Flameberry_MousePicking2D.vert.spv",
				"Flameberry_MousePicking2D.frag.spv" },
			{ "Flameberry_SkyMap.vert.spv", "Flameberry_SkyMap.frag.spv" },
			{ "Flameberry_DirectionalShadowMap.vert.spv",
				"Flameberry_DirectionalShadowMap.frag.spv" },
			{ "Flameberry_Quad.vert.spv", "Flameberry_Quad.frag.spv" },
			{ "Flameberry_SolidColor.vert.spv", "Flameberry_SolidColor.frag.spv" }
		};

		const char* computeShaderPaths[] = {};

		for (const auto& path : paths)
		{
			Ref<Shader> shader =
				CreateRef<Shader>((s_ShaderBaseDirectory / path[0]).c_str(),
					(s_ShaderBaseDirectory / path[1]).c_str());
			s_ShaderStorage[shader->GetName()] = shader;
		}

		for (const auto& path : computeShaderPaths)
		{
			Ref<Shader> shader =
				CreateRef<Shader>((s_ShaderBaseDirectory / path).c_str());
			s_ShaderStorage[shader->GetName()] = shader;
		}
	}

	Ref<Shader> ShaderLibrary::Get(const std::string& shaderName)
	{
		auto it = s_ShaderStorage.find(shaderName);
		FBY_ASSERT(it != s_ShaderStorage.end(),
			"Shader with name: {} is not loaded into ShaderLibrary yet",
			shaderName);
		return it->second;
	}

	void ShaderLibrary::Shutdown()
	{
		s_ShaderStorage.clear();
	}

} // namespace Flameberry
