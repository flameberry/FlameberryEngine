#pragma once

#include <unordered_map>

#include "Registry.h"
#include "Renderer/OpenGL/OpenGLRenderer2D.h"
#include "Renderer/OpenGL/OpenGLRenderer3D.h"
#include "Renderer/Light.h"
#include "Renderer/Material.h"
#include "Renderer/Skybox.h"

namespace Flameberry {
    struct SceneData
    {
        std::string Name = "Untitled";
        std::vector<Mesh> Meshes;
        std::unordered_map<std::string, Material> Materials;
        DirectionalLight DirLight;
        Skybox ActiveSkybox;
        SceneData(): ActiveSkybox(FL_PROJECT_DIR"SandboxApp/assets/skybox") {}
    };

    class Scene
    {
    public:
        Scene(Registry* registry = nullptr);
        ~Scene() = default;

        Registry* GetRegistry() { return m_Registry; }
        void RenderScene(OpenGLRenderer2D* renderer, const OrthographicCamera& camera);
        void RenderScene(OpenGLRenderer3D* renderer, const PerspectiveCamera& camera, const std::vector<PointLight>& lights);
        void SetSelectedEntity(entity_handle* entity) { m_SelectedEntity = entity; }
        void LoadMesh(const Mesh& mesh);
        void SetDirectionalLight(const DirectionalLight& light) { m_SceneData.DirLight = light; }
        void AddMaterial(const std::string& materialName, const Material& material) { m_SceneData.Materials[materialName] = material; }
    private:
        entity_handle* m_SelectedEntity = nullptr;
        Registry* m_Registry;
        SceneData m_SceneData;

        friend class SceneHierarchyPanel;
        friend class SceneSerializer;
    };
}
