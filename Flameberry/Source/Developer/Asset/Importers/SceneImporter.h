#pragma once

#include "Asset/AssetMetadata.h"
#include "ECS/Scene.h"

namespace Flameberry {

	class SceneImporter
	{
	public:
		static Ref<Scene> ImportScene(AssetHandle handle, const AssetMetadata& metadata);
		static Ref<Scene> LoadScene(const std::filesystem::path& path);
	};

} // namespace Flameberry
