#include "Project.h"

#include <fstream>
#include <filesystem>

#include "Core/Application.h"
#include "Asset/AssetManager.h"
#include "Asset/EditorAssetManager.h"
#include "Asset/RuntimeAssetManager.h"

#include "Project/ProjectSerializer.h"

namespace Flameberry {

	Ref<Project> Project::s_ActiveProject;

	constexpr const char* g_CsprojSource = R"(
<Project Sdk="Microsoft.NET.Sdk">
  <PropertyGroup>
    <TargetFramework>net7.0</TargetFramework>
    <ImplicitUsings>enable</ImplicitUsings>
    <Nullable>enable</Nullable>
    <OutputPath>Binaries</OutputPath>
  </PropertyGroup>
  
  <ItemGroup>
    <Reference Include="FlameberryScriptCore">
      <HintPath>{}/Flameberry/Source/Developer/ScriptingAPI/bin/Debug/net7.0/FlameberryScriptCore.dll</HintPath>
    </Reference>
  </ItemGroup>
</Project>
    )";

	Ref<Project> Project::CreateProjectOnDisk(const std::filesystem::path& targetFolder,
		const std::string& projectName)
	{
		ProjectConfig projectConfig;
		projectConfig.Name = projectName;
		projectConfig.AssetDirectory = "Content";
		projectConfig.AssetRegistryPath = "Intermediate/AssetRegistry.yaml";
		projectConfig.ThumbnailCacheDirectory = "Intermediate/Thumbnail.cache";
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

			// 6. Return the project instance
			return project;
		}

		FBY_ERROR("Failed to create project: Project Directory: {} is not empty!", project->GetProjectDirectory());
		return nullptr;
	}

	Project::Project(const std::filesystem::path& projectDirectory, const ProjectConfig& config)
		: m_ProjectDirectory(projectDirectory), m_Config(config)
	{
		Init();
	}

	Project::Project()
	{
		Init();
	}

	void Project::Init()
	{
		m_ThumbnailCache = CreateRef<ThumbnailCache>(GetThumbnailCacheDirectory());

		switch (Application::Get().GetSpecification().Type)
		{
			case ApplicationType::Editor:
				m_AssetManager = CreateRef<EditorAssetManager>();
				break;
			case ApplicationType::Runtime:
				m_AssetManager = CreateRef<RuntimeAssetManager>();
				break;
			default:
				break;
		}
	}

	Project::~Project() {}

	void Project::Save()
	{
		std::string fileName = m_Config.Name + ".fbproj";
		ProjectSerializer::SerializeProject(m_ProjectDirectory / fileName, this);
	}

	Ref<Project> Project::Load(const std::filesystem::path& path)
	{
		if (Ref<Project> project = ProjectSerializer::DeserializeIntoNewProject(path))
		{
			Ref<EditorAssetManager> editorAssetManager = std::static_pointer_cast<EditorAssetManager>(project->m_AssetManager);
			editorAssetManager->DeserializeAssetRegistry(project->GetAssetRegistryPath());
			return project;
		}
		FBY_ERROR("Failed to load project: {}", path);
	}

} // namespace Flameberry
