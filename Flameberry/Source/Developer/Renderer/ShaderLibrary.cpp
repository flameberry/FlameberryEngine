#include "ShaderLibrary.h"

namespace Flameberry {

	std::unordered_map<std::string, Ref<Shader>> ShaderLibrary::s_ShaderStorage;
	const std::filesystem::path ShaderLibrary::s_ShaderBaseDirectory = FBY_PROJECT_DIR "Flameberry/Shaders/Vulkan/Compiled";

	void ShaderLibrary::Init()
	{
		// Currently this is hardcoded as no new shaders are loaded dynamically at runtime
		const char* paths[][2] = {
			{ "PBR.vert.spv", "PBR.frag.spv" },
			{ "MousePicking.vert.spv", "MousePicking.frag.spv" },
			{ "MousePicking2D.vert.spv", "MousePicking2D.frag.spv" },
			{ "Skymap.vert.spv", "Skymap.frag.spv" },
			{ "DirectionalShadowMap.vert.spv", "DirectionalShadowMap.frag.spv" },
			{ "Quad.vert.spv", "Quad.frag.spv" },
			{ "SolidColor.vert.spv", "SolidColor.frag.spv" },
			{ "Text.vert.spv", "Text.frag.spv" },
			{ "InfiniteGrid.vert.spv", "InfiniteGrid.frag.spv" },
		};

		const char* computeShaderPaths[] = {
			"HDRToCubemap.comp.spv",
			"IrradianceMap.comp.spv",
			"PrefilteredMap.comp.spv",
			"BRDFLUT.comp.spv",
		};

		for (const auto& path : paths)
		{
			Ref<Shader> shader = CreateRef<Shader>((s_ShaderBaseDirectory / path[0]).string().c_str(), (s_ShaderBaseDirectory / path[1]).string().c_str());
			s_ShaderStorage[shader->GetName()] = shader;
		}

		for (const auto& path : computeShaderPaths)
		{
			Ref<Shader> shader = CreateRef<Shader>((s_ShaderBaseDirectory / path).string().c_str());
			s_ShaderStorage[shader->GetName()] = shader;
		}
	}

	Ref<Shader> ShaderLibrary::Get(const std::string& shaderName)
	{
		auto it = s_ShaderStorage.find(shaderName);
		FBY_ASSERT(it != s_ShaderStorage.end(), "Shader with name: {} is not loaded into ShaderLibrary yet", shaderName);
		return it->second;
	}

	void ShaderLibrary::Shutdown()
	{
		s_ShaderStorage.clear();
	}

} // namespace Flameberry
