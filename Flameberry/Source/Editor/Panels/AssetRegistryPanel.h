#pragma once

#include <string>

#include "Asset/Asset.h"

namespace Flameberry {

	class AssetRegistryPanel {
	public:
		void OnUIRender();

	private:
		void DisplayToolbar(uint32_t visibleAssetCount, uint32_t totalAssetCount);

	private:
		std::string m_SearchInputBuffer, m_TypeFilterSearchBuffer;
		bool m_IsSearchBoxFocused = false;

		// AssetType::None shows the assets of all types
		AssetType m_TypeFilter = AssetType::None;
		AssetHandle m_SelectedHandle = 0;
	};

} // namespace Flameberry
