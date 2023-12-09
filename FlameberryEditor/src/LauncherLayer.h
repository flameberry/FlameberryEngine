#pragma once

#include "Flameberry.h"

namespace Flameberry {

    class LauncherLayer : public Layer
    {
    public:
        LauncherLayer(const std::function<void(const std::shared_ptr<Project>&)>& callback);
        virtual ~LauncherLayer() = default;

        void OnCreate() override;
        void OnUpdate(float delta) override;
        void OnUIRender() override;
        void OnEvent(Event& e) override;
        void OnDestroy() override;

        void UI_NewProjectPopup();
    private:
        ProjectRegistry m_ProjectRegistry;

        std::shared_ptr<Project> m_Project;
        std::function<void(const std::shared_ptr<Project>&)> m_OpenProjectCallback;
        bool m_ShouldClose = false;

        char m_ProjectNameBuffer[128] = "";
        char m_ProjectPathBuffer[512] = "";
    };

}
