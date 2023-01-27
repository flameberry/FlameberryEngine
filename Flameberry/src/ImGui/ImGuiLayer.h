#pragma once

#include "Core/Event.h"

namespace Flameberry {
    class ImGuiLayer
    {
    public:
        void OnCreate();
        void OnDestroy();
        void OnEvent(Event& e);
        void Begin();
        void End();
    private:
        void SetupImGuiStyle();
    };
}
