#pragma once

#include "Flameberry.h"
#include "MaterialEditorPanel.h"
#include "MaterialSelectorPanel.h"

namespace Flameberry {
    class InspectorPanel
    {
    public:
        InspectorPanel();
        InspectorPanel(const std::shared_ptr<Scene>& context);
        ~InspectorPanel() = default;

        void SetContext(const std::shared_ptr<Scene>& context) { m_Context = context; }
        void SetSelectionContext(const ecs::entity_handle& selectionContext) { m_SelectionContext = selectionContext; }
        void OnUIRender();

        template<typename... Args>
        static std::shared_ptr<InspectorPanel> Create(Args... args) { return std::make_shared<InspectorPanel>(std::forward<Args>(args)...); }
    private:
        void DrawComponent(TransformComponent& transform);
        void DrawComponent(MeshComponent& mesh);
        void DrawComponent(LightComponent& light);
    private:
        ImGuiTableFlags m_TableFlags = ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoKeepColumnsVisible;

        std::shared_ptr<MaterialEditorPanel> m_MaterialEditorPanel;
        std::shared_ptr<MaterialSelectorPanel> m_MaterialSelectorPanel;

        ecs::entity_handle m_SelectionContext = {};
        std::shared_ptr<Scene> m_Context;
    };
}