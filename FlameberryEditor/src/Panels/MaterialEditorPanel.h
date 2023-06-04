#pragma once

#include "Flameberry.h"

namespace Flameberry {
    class MaterialEditorPanel
    {
    public:
        void SetCurrentMaterial(const std::shared_ptr<Material>& mat) { m_CurrentMaterial = mat; }
        void OnUIRender();
    private:
        std::shared_ptr<Material> m_CurrentMaterial;
    };
}
