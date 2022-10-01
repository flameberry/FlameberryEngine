#pragma once

#include "Panel.h"
#include "Flameberry.h"

class SceneHierarchyPanel : public Panel
{
public:
    SceneHierarchyPanel(Flameberry::Scene* scene = nullptr);
    virtual ~SceneHierarchyPanel() = default;
    void OnUIRender() override;
private:
    void DrawComponent(Flameberry::TransformComponent& transform);
    void DrawComponent(Flameberry::SpriteRendererComponent& sprite);
    void DrawVec3Control(const std::string& label, glm::vec3& value, float defaultValue = 0.0f, float dragSpeed = 0.01f);
private:
    Flameberry::entity_handle m_SelectedEntity;
    Flameberry::Scene* m_Scene;
private:
    uint32_t m_DefaultTextureId;
};
