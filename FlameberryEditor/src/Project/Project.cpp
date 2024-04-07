#include "Project.h"

#include <fstream>
#include <filesystem>

#include "Core/Core.h"
#include "Core/YamlUtils.h"

namespace Flameberry {

    Project::Project(const std::filesystem::path& projectDirectory, const ProjectConfig& config)
        : m_ProjectDirectory(projectDirectory), m_Config(config)
    {
    }

    Project::~Project() {}

    void Project::Save()
    {
        std::string fileName = m_Config.Name + ".fbproj";
        ProjectSerializer::SerializeProject(m_ProjectDirectory / fileName, this);
    }

    Ref<Project> ProjectSerializer::DeserializeIntoNewProject(const std::filesystem::path& filePath)
    {
        Ref<Project> newProject = CreateRef<Project>();
        if (DeserializeIntoExistingProject(filePath, newProject))
            return newProject;
        return nullptr;
    }

    bool ProjectSerializer::DeserializeIntoExistingProject(const std::filesystem::path& filePath, const Ref<Project>& dest)
    {
        std::ifstream in(filePath);
        std::stringstream ss;
        ss << in.rdbuf();

        YAML::Node data = YAML::Load(ss.str());
        if (!data["Project"])
        {
            FBY_ERROR("Failed to load project [{}]: 'Project' attribute not present in file!", filePath);
            return false;
        };

        dest->m_ProjectDirectory = std::filesystem::absolute(filePath.parent_path());
        dest->m_Config.Name = data["Project"].as<std::string>();

        auto config = data["Configuration"];
        if (!config)
        {
            FBY_ERROR("Failed to load project [{}]: 'Configuration' attribute not present in file!", filePath);
            return false;
        };
        dest->m_Config.AssetDirectory = config["AssetDirectory"].as<std::string>();
        dest->m_Config.StartScene = config["StartScene"].as<std::string>();
        return true;
    }

    void ProjectSerializer::SerializeProject(const std::filesystem::path& filePath, const Project* project)
    {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Project" << YAML::Value << project->m_Config.Name;
        out << YAML::Key << "Configuration" << YAML::Value;
        {
            out << YAML::BeginMap; // Configuration
            out << YAML::Key << "AssetDirectory" << YAML::Value << project->m_Config.AssetDirectory;
            out << YAML::Key << "StartScene" << YAML::Value << project->m_Config.StartScene;
            out << YAML::EndMap; // Configuration
        }
        out << YAML::EndMap;

        std::ofstream fout(filePath);
        fout << out.c_str();
    }

    void ProjectRegistryManager::AppendEntryToGlobalRegistry(Project* project)
    {
        YAML::Emitter out;
        const auto& filepath = fmt::format("{}/{}.fbproj", project->GetProjectDirectory(), project->GetConfig().Name);
        out << YAML::BeginMap;
        {
            out << YAML::Key << filepath << YAML::Value << YAML::BeginMap;
            out << YAML::Key << "ProjectName" << YAML::Value << project->GetConfig().Name;
            out << YAML::EndMap;
        }
        out << YAML::EndMap;

        std::ofstream fout(c_GlobalProjectRegistryPath, std::ios_base::app);
        FBY_ASSERT(fout.is_open(), "Failed to find/open project registry: {}", c_GlobalProjectRegistryPath);
        fout << out.c_str() << '\n';
    }

    void ProjectRegistryManager::RemoveEntryFromGlobalRegistry(const std::filesystem::path& pathOfProjectEntry)
    {
    }

    ProjectRegistry ProjectRegistryManager::LoadEntireProjectRegistry()
    {
        std::ifstream in(c_GlobalProjectRegistryPath);
        std::stringstream ss;
        ss << in.rdbuf();

        YAML::Node data = YAML::Load(ss.str());

        ProjectRegistry registry;
        for (const auto& entry : data)
        {
            ProjectRegistryEntry projectEntry;
            projectEntry.ProjectFilePath = entry.first.as<std::string>();
            projectEntry.ProjectName = entry.second["ProjectName"].as<std::string>();
            registry.emplace_back(projectEntry);
        }
        return registry;
    }

    void ProjectRegistryManager::ClearAllNonExistingProjectRegistryEntries()
    {
        std::stringstream ss;
        {
            std::ifstream fin(c_GlobalProjectRegistryPath);
            ss << fin.rdbuf();
        }

        YAML::Node data = YAML::Load(ss.str());

        for (const auto& entry : data)
        {
            const auto& path = entry.first.as<std::string>();
            if (!std::filesystem::exists(path))
                data.remove(entry.first.as<std::string>());
        }

        {
            std::ofstream fout(c_GlobalProjectRegistryPath, std::ios::trunc);
            fout << data << '\n';
        }
    }

}
