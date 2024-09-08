#pragma once

#include <memory>
#include <set>

#include "Scene.h"

namespace YAML {
	class Emitter;
}

namespace Flameberry {

	class SceneSerializer
	{
	public:
		static Ref<Scene> DeserializeIntoNewScene(const std::filesystem::path& path);
		static bool DeserializeIntoExistingScene(const std::filesystem::path& path, const Ref<Scene>& destScene);
		static void SerializeSceneToFile(const std::filesystem::path& path, const Ref<Scene>& srcScene);

	private:
		static void SerializeEntity(YAML::Emitter& out, const FEntity& entity, const Ref<Scene>& scene, std::set<UUID>& assetUUIDs);
	};

} // namespace Flameberry
