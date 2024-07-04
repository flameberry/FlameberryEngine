#include "Project.h"

#include <fstream>
#include <filesystem>

#include "Core/Core.h"
#include "Core/YamlUtils.h"

namespace Flameberry {

	constexpr const char* g_CsprojSource = R"(
<Project Sdk="Microsoft.NET.Sdk">
  <PropertyGroup>
    <TargetFramework>net7.0</TargetFramework>
    <ImplicitUsings>enable</ImplicitUsings>
    <Nullable>enable</Nullable>
    <OutputPath>Binaries</OutputPath>
  </PropertyGroup>
  
  <ItemGroup>
    <Reference Include="Flameberry-ScriptCore">
      <HintPath>{}/Flameberry-ScriptCore/bin/Release/net7.0/Flameberry-ScriptCore.dll</HintPath>
    </Reference>
  </ItemGroup>
</Project>
    )";

	Ref<Project>
	Project::CreateProjectOnDisk(const std::filesystem::path& targetFolder,
		const std::string& projectName)
	{
		ProjectConfig projectConfig;
		projectConfig.Name = projectName;
		projectConfig.AssetDirectory = "Content";
		projectConfig.ScriptAssemblyPath =
			fmt::format("Content/Scripting/Binaries/net7.0/{}.dll", projectName);

		Ref<Project> project =
			CreateRef<Project>(targetFolder / projectConfig.Name, projectConfig);

		// Setup
		// 1. Create the project directory
		if (!std::filesystem::exists(project->GetProjectDirectory()))
			std::filesystem::create_directory(project->GetProjectDirectory());

		if (std::filesystem::is_empty(project->GetProjectDirectory()))
		{
			// 2. Serialize the Project
			project->Save();
			// 3. Create a Content folder
			std::filesystem::create_directory(project->GetAssetDirectory());
			// 4. Create a `Scripting` folder
			std::filesystem::create_directory(project->GetAssetDirectory() / "Scripting");

			// 5. Add a `.csproj` file to it
			std::ofstream csprojFile(project->GetAssetDirectory() / "Scripting" / fmt::format("{}.csproj", projectConfig.Name));
			csprojFile << fmt::format(g_CsprojSource, FBY_PROJECT_DIR);
			csprojFile.close();

			// Build using dotnet?

			// 4. Add project entry to GlobalProjectRegistry
			ProjectRegistryManager::AppendEntryToGlobalRegistry(project.get());
			// 5. Return the project instance
			return project;
		}

		FBY_ERROR("Failed to create project: Project Directory: {} is not empty!",
			project->GetProjectDirectory());
		return nullptr;
	}

	Project::Project(const std::filesystem::path& projectDirectory,
		const ProjectConfig& config)
		: m_ProjectDirectory(projectDirectory), m_Config(config) {}

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
		std::ifstream	  in(filePath);
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
		dest->m_Config.ScriptAssemblyPath =
			config["ScriptAssemblyPath"].as<std::string>();
		return true;
	}

	void ProjectSerializer::SerializeProject(const std::filesystem::path& filePath,
		const Project* project)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Project" << YAML::Value << project->m_Config.Name;
		out << YAML::Key << "Configuration" << YAML::Value;
		{
			out << YAML::BeginMap; // Configuration
			out << YAML::Key << "AssetDirectory" << YAML::Value
				<< project->m_Config.AssetDirectory;
			out << YAML::Key << "ScriptAssemblyPath" << YAML::Value
				<< project->m_Config.ScriptAssemblyPath;
			out << YAML::Key << "StartScene" << YAML::Value
				<< project->m_Config.StartScene;
			out << YAML::EndMap; // Configuration
		}
		out << YAML::EndMap;

		std::ofstream fout(filePath);
		fout << out.c_str();
	}

	void ProjectRegistryManager::AppendEntryToGlobalRegistry(Project* project)
	{
		YAML::Emitter out;
		const auto&	  filepath = fmt::format("{}/{}.fbproj", project->GetProjectDirectory(), project->GetConfig().Name);
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
		std::ifstream	  in(c_GlobalProjectRegistryPath);
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

} // namespace Flameberry
