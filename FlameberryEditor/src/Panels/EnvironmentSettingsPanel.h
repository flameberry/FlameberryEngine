#pragma once

#include "Flameberry.h"

namespace Flameberry {
    class EnvironmentSettingsPanel
    {
    public:
        EnvironmentSettingsPanel(const std::shared_ptr<Scene>& context);
        ~EnvironmentSettingsPanel() = default;

        void OnUIRender();

        template<typename... Args>
        static std::shared_ptr<EnvironmentSettingsPanel> Create(Args... args) { return std::make_shared<EnvironmentSettingsPanel>(std::forward<Args>(args)...); }
    private:
        const std::shared_ptr<Scene> m_Context;
        ImGuiTableFlags m_TableFlags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoKeepColumnsVisible | ImGuiTableFlags_PadOuterX;
    };
}