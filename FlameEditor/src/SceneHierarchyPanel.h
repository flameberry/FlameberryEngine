#pragma once

#include "Flameberry.h"

class SceneHierarchyPanel
{
public:
    SceneHierarchyPanel(Flameberry::Scene* scene = NULL);
    ~SceneHierarchyPanel() = default;
    void OnUIRender();
private:
    Flameberry::entity_handle m_SelectedEntity;
    Flameberry::Scene* m_Scene;
};
