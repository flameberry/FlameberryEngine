#pragma once

#include "Flameberry.h"

namespace Flameberry {
    class EnvironmentSettingsPanel
    {
    public:
        EnvironmentSettingsPanel(const std::shared_ptr<Scene>& context);
        ~EnvironmentSettingsPanel() = default;

        void SetContext(const std::shared_ptr<Scene>& context) { m_Context = context; }

        void OnUIRender();

        template<typename... Args>
        static std::shared_ptr<EnvironmentSettingsPanel> Create(Args... args) { return std::make_shared<EnvironmentSettingsPanel>(std::forward<Args>(args)...); }
    private:
        std::shared_ptr<Scene> m_Context;

        const ImGuiTableFlags m_TableFlags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoKeepColumnsVisible | ImGuiTableFlags_PadOuterX;
        const float m_LabelWidth = 100.0f;
    };
}