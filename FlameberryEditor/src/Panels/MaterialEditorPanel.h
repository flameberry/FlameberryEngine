#pragma once

#include "Flameberry.h"

namespace Flameberry {
    class MaterialEditorPanel
    {
    public:
        void SetEditingContext(const std::shared_ptr<Material>& editingContext) { m_EditingContext = editingContext; }
        void OnUIRender();
    private:
        ImGuiTableFlags m_TableFlags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoKeepColumnsVisible | ImGuiTableFlags_PadOuterX;
        std::shared_ptr<Material> m_EditingContext;
        bool m_ShouldRename = false;
        char m_RenameBuffer[256] = { '\0' };
    };
}
