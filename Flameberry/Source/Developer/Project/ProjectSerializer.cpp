#include "ProjectSerializer.h"

#include <fstream>

#include "Core/YamlUtils.h"

namespace Flameberry {

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

		YAML::Node config = data["Configuration"];
		if (!config)
		{
			FBY_ERROR("Failed to load project [{}]: 'Configuration' attribute not present in file!", filePath);
			return false;
		}

		dest->m_Config.AssetDirectory = config["AssetDirectory"].as<std::string>();
		dest->m_Config.StartScene = config["StartScene"].as<AssetHandle>();
		dest->m_Config.AssetRegistryPath = config["AssetRegistryPath"].as<std::string>();
		dest->m_Config.ThumbnailCacheDirectory = config["ThumbnailCacheDirectory"].as<std::string>();
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
			out << YAML::Key << "AssetRegistryPath" << YAML::Value << project->m_Config.AssetRegistryPath;
			out << YAML::Key << "ThumbnailCacheDirectory" << YAML::Value << project->m_Config.ThumbnailCacheDirectory;
			out << YAML::EndMap; // Configuration
		}
		out << YAML::EndMap;

		std::ofstream fout(filePath);
		fout << out.c_str();
	}
} // namespace Flameberry
