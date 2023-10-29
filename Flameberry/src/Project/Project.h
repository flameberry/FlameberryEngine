#pragma once

#include <filesystem>

namespace Flameberry {
    struct ProjectConfig {
        std::filesystem::path AssetDirectory;
        
        std::string Name = "Untitled";
    };
    
    class Project
    {
    public:
        static const std::filesystem::path& GetActiveProjectDirectory() {
            return s_ActiveProject->GetProjectDirectory();
        }
        
        static std::filesystem::path GetActiveAssetDirectory() {
            return s_ActiveProject->GetProjectDirectory() / s_ActiveProject->m_Config.AssetDirectory;
        }
        
        Project(const std::filesystem::path& projectDirectory, const ProjectConfig& config);
        ~Project();
        
        const std::filesystem::path& GetProjectDirectory() const { return m_ProjectDirectory; }
        std::filesystem::path GetAssetDirectory() const { return m_ProjectDirectory / m_Config.AssetDirectory; }
        
        ProjectConfig& GetConfig() { return m_Config; }
        
        static std::shared_ptr<Project> GetActiveProject() { return s_ActiveProject; }
        static void SetActive(const std::shared_ptr<Project>& project) { s_ActiveProject = project; }
        
        template <typename... Args>
        static std::shared_ptr<Project> Create(Args... args) { return std::make_shared<Project>(std::forward<Args>(args)...); }
    private:
        std::filesystem::path m_ProjectDirectory;
        ProjectConfig m_Config;
        
        inline static std::shared_ptr<Project> s_ActiveProject;
    };
}