#pragma once

#include "Flameberry.h"

namespace Flameberry {
    class MaterialEditorPanel
    {
    public:
        void SetEditingContext(const std::shared_ptr<Material>& editingContext) { m_EditingContext = editingContext; }
        void OnUIRender();
    private:
        std::shared_ptr<Material> m_EditingContext;
    };
}
