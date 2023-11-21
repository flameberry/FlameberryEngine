#pragma once

#include "Flameberry.h"

namespace Flameberry {
    class MaterialSelectorPanel
    {
    public:
        using CallBackType = std::function<void(const std::shared_ptr<Material>&)>;
    public:
        void OpenPanel(const CallBackType& selectCallBack);
        void OnUIRender();
    private:
        bool m_Open = false;
        std::shared_ptr<Material> m_SelectedMaterial;
        CallBackType m_SelectCallBack;
    };
}
