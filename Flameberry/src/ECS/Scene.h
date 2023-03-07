#pragma once

#include <unordered_map>

#include "ecs.hpp"
#include "Renderer/OpenGL/OpenGLRenderer2D.h"
#include "Renderer/OpenGL/OpenGLRenderer3D.h"
#include "Renderer/OpenGL/OpenGLShader.h"
#include "Renderer/Light.h"
#include "Renderer/Material.h"
#include "Renderer/Skybox.h"

namespace Flameberry {
    struct EnvironmentMap
    {
        glm::vec3 ClearColor;
        std::shared_ptr<Skybox> ActiveSkybox;
        DirectionalLight DirLight;
        bool Reflections, EnableSkybox;

        EnvironmentMap()
            : ActiveSkybox(std::make_shared<Skybox>(FL_PROJECT_DIR"SandboxApp/assets/skybox")),
            Reflections(false),
            EnableSkybox(true)
        {}
    };

    struct SceneData
    {
        std::string Name = "Untitled";
        std::vector<Mesh> Meshes;
        std::unordered_map<std::string, Material> Materials;
        EnvironmentMap ActiveEnvironmentMap;
    };

    class Scene
    {
    public:
        Scene(ecs::registry* registry = nullptr);
        ~Scene() = default;

        void RenderScene(OpenGLRenderer2D* renderer, const OrthographicCamera& camera);

        ecs::registry* GetRegistry() { return m_Registry; }
        void SetSelectedEntity(ecs::entity_handle* entity) { m_SelectedEntity = entity; }
        ecs::entity_handle GetSelectedEntity() const { return *m_SelectedEntity; }
        void LoadMesh(const Mesh& mesh);
        void SetDirectionalLight(const DirectionalLight& light) { m_SceneData.ActiveEnvironmentMap.DirLight = light; }
        void AddMaterial(const std::string& materialName, const Material& material) { m_SceneData.Materials[materialName] = material; }

        glm::vec3 GetClearColor() const { return m_SceneData.ActiveEnvironmentMap.ClearColor; }
        DirectionalLight GetDirectionalLight() const { return m_SceneData.ActiveEnvironmentMap.DirLight; }
    private:
        ecs::entity_handle* m_SelectedEntity = nullptr;
        ecs::registry* m_Registry;
        SceneData m_SceneData;

        friend class SceneHierarchyPanel;
        friend class SceneSerializer;
        friend class SceneRenderer;
    };
}
