#include "SceneImporter.h"

#include "ECS/SceneSerializer.h"

namespace Flameberry {

	Ref<Scene> SceneImporter::ImportScene(AssetHandle handle, const AssetMetadata& metadata)
	{
		return LoadScene(metadata.FilePath);
	}

	Ref<Scene> SceneImporter::LoadScene(const std::filesystem::path& path)
	{
		return SceneSerializer::DeserializeIntoNewScene(path.c_str());
	}

} // namespace Flameberry