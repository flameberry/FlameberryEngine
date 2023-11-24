#include "Project.h"

#include <fstream>

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

    std::shared_ptr<Project> ProjectSerializer::DeserializeIntoNewProject(const std::filesystem::path& filePath)
    {
        std::shared_ptr<Project> newProject = std::make_shared<Project>();
        if (DeserializeIntoExistingProject(filePath, newProject))
            return newProject;
        return nullptr;
    }
    
    bool ProjectSerializer::DeserializeIntoExistingProject(const std::filesystem::path& filePath, const std::shared_ptr<Project>& dest)
    {
        std::ifstream in(filePath);
        std::stringstream ss;
        ss << in.rdbuf();
        
        YAML::Node data = YAML::Load(ss.str());
        if (!data["Project"])
        {
            FBY_ERROR("Failed to load project [{0}]: 'Project' attribute not present in file!", filePath);
            return false;
        };
        
        dest->m_ProjectDirectory = std::filesystem::absolute(filePath.parent_path());
        dest->m_Config.Name = data["Project"].as<std::string>();
        
        auto config = data["Configuration"];
        if (!config)
        {
            FBY_ERROR("Failed to load project [{0}]: 'Configuration' attribute not present in file!", filePath);
            return false;
        };
        dest->m_Config.AssetDirectory = config["AssetDirectory"].as<std::string>();
        dest->m_Config.ScriptAssemblyPath = config["ScriptAssemblyPath"].as<std::string>();
        return true;
    }
    
    void ProjectSerializer::SerializeProject(const std::filesystem::path &filePath, const Project* project)
    {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Project" << YAML::Value << project->m_Config.Name;
        out << YAML::Key << "Configuration" << YAML::Value;
        {
            out << YAML::BeginMap; // Configuration
            out << YAML::Key << "AssetDirectory" << YAML::Value << project->m_Config.AssetDirectory;
            out << YAML::Key << "ScriptAssemblyPath" << YAML::Value << project->m_Config.ScriptAssemblyPath;
            out << YAML::EndMap; // Configuration
        }
        out << YAML::EndMap;
        
        std::ofstream fout(filePath);
        fout << out.c_str();
    }
}
