#pragma once

#include <unordered_map>

#include "ecs.hpp"
#include "Renderer/Light.h"
#include "Renderer/Material.h"
#include "Renderer/Vulkan/VulkanMesh.h"
// #include "Renderer/Skybox.h"

namespace Flameberry {
    struct EnvironmentMap
    {
        glm::vec3 ClearColor;
        // std::shared_ptr<Skybox> ActiveSkybox;
        bool EnableSkybox, Reflections;
        DirectionalLight DirLight;

        EnvironmentMap()
            : ClearColor(0.05f, 0.05f, 0.05f),
            EnableSkybox(false),
            Reflections(false)
        {}
    };

    struct SceneData
    {
        std::string Name = "Untitled";
        std::vector<std::shared_ptr<VulkanMesh>> Meshes;
        std::unordered_map<std::string, Material> Materials;
        EnvironmentMap ActiveEnvironmentMap;
    };

    class Scene
    {
    public:
        Scene(ecs::registry* registry = nullptr);
        ~Scene() = default;

        ecs::registry* GetRegistry() { return m_Registry; }
        void SetSelectedEntity(ecs::entity_handle* entity) { m_SelectedEntity = entity; }
        ecs::entity_handle GetSelectedEntity() const { return *m_SelectedEntity; }
        void LoadMesh(const char* filePath);
        void SetDirectionalLight(const DirectionalLight& light) { m_SceneData.ActiveEnvironmentMap.DirLight = light; }
        void AddMaterial(const std::string& materialName, const Material& material) { m_SceneData.Materials[materialName] = material; }

        inline glm::vec3 GetClearColor() const { return m_SceneData.ActiveEnvironmentMap.ClearColor; }
        inline std::string GetName() const { return m_SceneData.Name; }
        inline DirectionalLight GetDirectionalLight() const { return m_SceneData.ActiveEnvironmentMap.DirLight; }
    private:
        ecs::entity_handle* m_SelectedEntity = nullptr;
        ecs::registry* m_Registry;
        SceneData m_SceneData;

        friend class SceneHierarchyPanel;
        friend class SceneSerializer;
        friend class SceneRenderer;
    };
}
