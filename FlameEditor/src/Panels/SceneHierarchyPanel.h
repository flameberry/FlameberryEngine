#pragma once

#include "Panel.h"
#include "Flameberry.h"

class SceneHierarchyPanel : public Panel
{
public:
    SceneHierarchyPanel(Flameberry::Scene* scene = NULL);
    virtual ~SceneHierarchyPanel() = default;
    void OnUIRender() override;
private:
    static void DrawComponent(Flameberry::TransformComponent& transform);
    static void DrawVec3Control(const std::string& label, glm::vec3& value, float defaultValue = 0.0f, float dragSpeed = 0.01f);
private:
    Flameberry::entity_handle m_SelectedEntity;
    Flameberry::Scene* m_Scene;
};
