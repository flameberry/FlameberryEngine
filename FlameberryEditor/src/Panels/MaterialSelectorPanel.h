#pragma once

#include "Flameberry.h"

namespace Flameberry {
    class MaterialSelectorPanel
    {
    public:
        using CallBackType = std::function<void(const Ref<Material>&)>;
    public:
        void OpenPanel(const CallBackType& selectCallBack);
        void OnUIRender();
    private:
        bool m_Open = false;
        Ref<Material> m_SelectedMaterial;
        CallBackType m_SelectCallBack;
    };
}
