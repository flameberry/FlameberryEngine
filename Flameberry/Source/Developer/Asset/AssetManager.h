#pragma once

#include <type_traits>

#include "Project/Project.h"

namespace Flameberry {

	/**
	 * Static Wrapper around the IAssetManager stored in the active project
	 */
	class AssetManager
	{
	public:
		template <typename TAssetManager>
		static Ref<TAssetManager> As()
		{
			static_assert(std::is_base_of_v<IAssetManager, TAssetManager>, "The class given must be derived from `IAssetManager`");
			return std::static_pointer_cast<TAssetManager>(Project::GetActiveProject()->GetAssetManager());
		}

		template <typename T>
		static Ref<T> GetAsset(AssetHandle handle)
		{
			static_assert(std::is_base_of_v<Asset, T>, "The class given must be derived from `Asset`");
			return std::static_pointer_cast<T>(Project::GetActiveProject()->GetAssetManager()->GetAsset(handle));
		}

		static bool IsAssetHandleValid(AssetHandle handle)
		{
			return Project::GetActiveProject()->GetAssetManager()->IsAssetHandleValid(handle);
		}

		static bool IsAssetLoaded(AssetHandle handle)
		{
			return Project::GetActiveProject()->GetAssetManager()->IsAssetLoaded(handle);
		}
	};

} // namespace Flameberry
