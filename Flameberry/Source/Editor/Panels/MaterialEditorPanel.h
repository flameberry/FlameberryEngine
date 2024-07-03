#pragma once

#include "Flameberry.h"

namespace Flameberry {

	class MaterialEditorPanel
	{
	public:
		inline void DisplayMaterial(const Ref<MaterialAsset>& material)
		{
			m_Open = true;
			m_EditingContext = material;
		}
		void OnUIRender();
		bool DrawMapControls(const char* label, bool& mapEnabledVar, Ref<Texture2D>& map);

	private:
		static constexpr ImGuiTableFlags s_TableFlags = ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_NoKeepColumnsVisible;
		static constexpr float s_LabelWidth = 100.0f;

		Ref<MaterialAsset> m_EditingContext;
		bool m_IsMaterialEdited = false, m_ShouldRename = false, m_Open = false;
		char m_RenameBuffer[256] = { '\0' };
	};

} // namespace Flameberry
