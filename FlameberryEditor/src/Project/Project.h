#pragma once

#include <filesystem>

namespace Flameberry {
    struct ProjectConfig
    {
        std::filesystem::path AssetDirectory /* = "Content" */;

        std::string Name = "Flameberry-Project";
    };

    struct ProjectRegistryEntry
    {
        std::string ProjectName;
        std::filesystem::path ProjectFilePath;

        const bool operator==(const ProjectRegistryEntry& other) {
            // This assumes that ProjectName will always be derived from the project file name so no need to compare `ProjectName`
            return ProjectFilePath == other.ProjectFilePath;
        }

        const bool operator!=(const ProjectRegistryEntry& other) {
            // This assumes that ProjectName will always be derived from the project file name so no need to compare `ProjectName`
            return !(*this == other);
        }

        // TODO: Add Timestamps, Project Type etc in the future
    };

    typedef std::vector<ProjectRegistryEntry> ProjectRegistry;

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
        Project() = default;
        ~Project();

        void Save();

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

        friend class ProjectSerializer;
    };

    class ProjectSerializer
    {
    public:
        static std::shared_ptr<Project> DeserializeIntoNewProject(const std::filesystem::path& filePath);
        static bool DeserializeIntoExistingProject(const std::filesystem::path& filePath, const std::shared_ptr<Project>& dest);
        static void SerializeProject(const std::filesystem::path& filePath, const Project* project);
    };

    class ProjectRegistryManager
    {
    public:
        static void AppendEntryToGlobalRegistry(Project* project);
        static void RemoveEntryFromGlobalRegistry(const std::filesystem::path& pathOfProjectEntry);
        static ProjectRegistry LoadEntireProjectRegistry();

    private:
        constexpr static const char* c_GlobalProjectRegistryPath = FBY_PROJECT_DIR"FlameberryEditor/GlobalConfig/ProjectRegistry.fbreg";
    };
}
