#pragma once

#include "Flameberry.h"

namespace Flameberry {
    class MaterialEditorPanel
    {
    public:
        void SetEditingContext(const Ref<MaterialAsset>& editingContext) { m_EditingContext = editingContext; }
        void OnUIRender();
        bool DrawMapControls(const char* label, bool& mapEnabledVar, Ref<Texture2D>& map);
    private:
        static constexpr ImGuiTableFlags s_TableFlags = ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_NoKeepColumnsVisible;
        static constexpr float s_LabelWidth = 100.0f;

        Ref<MaterialAsset> m_EditingContext;
        bool m_IsMaterialEdited = false, m_ShouldRename = false;
        char m_RenameBuffer[256] = { '\0' };
    };
}
