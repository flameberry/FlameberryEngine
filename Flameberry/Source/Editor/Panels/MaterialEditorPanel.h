#pragma once

#include "Flameberry.h"

namespace Flameberry {

	class MaterialEditorPanel
	{
	public:
		MaterialEditorPanel();

		void OnUIRender();
		bool DrawMapControls(const char* label, bool& mapEnabledVar, AssetHandle& mapHandle);

		inline void DisplayMaterial(const Ref<MaterialAsset>& material)
		{
			m_Open = true;
			m_EditingContext = material;
		}

	private:
		Ref<MaterialAsset> m_EditingContext;
		bool m_IsMaterialEdited = false, m_ShouldRename = false, m_Open = false;
		char m_RenameBuffer[256] = { '\0' };

		Ref<Texture2D> m_CheckerboardTexture;
	};

} // namespace Flameberry
