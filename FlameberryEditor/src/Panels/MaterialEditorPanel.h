#pragma once

#include "Flameberry.h"

namespace Flameberry {
    class MaterialEditorPanel
    {
    public:
        void SetEditingContext(const std::shared_ptr<Material>& editingContext) { m_EditingContext = editingContext; }
        void OnUIRender();
        void DrawMapControls(const char* label, bool& mapEnabledVar, std::shared_ptr<Texture2D>& map);
    private:
        static constexpr ImGuiTableFlags s_TableFlags = ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_NoKeepColumnsVisible;
        static constexpr float s_LabelWidth = 100.0f;

        std::shared_ptr<Material> m_EditingContext;
        bool m_IsMaterialEdited = false, m_ShouldRename = false;
        char m_RenameBuffer[256] = { '\0' };
    };
}
