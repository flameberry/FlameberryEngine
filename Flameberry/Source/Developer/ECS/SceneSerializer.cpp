#include "SceneSerializer.h"

#include <fstream>

#include "Core/YamlUtils.h"
#include "Core/Timer.h"
#include "Components.h"

#include "Asset/AssetManager.h"
#include "Asset/EditorAssetManager.h"
#include "Asset/Importers/MeshImporter.h"
#include "Renderer/MaterialAsset.h"
#include "Renderer/GenericCamera.h"
#include "Renderer/Skymap.h"

namespace Flameberry {

	static std::string ProjectionTypeEnumToString(ProjectionType type)
	{
		switch (type)
		{
			case ProjectionType::Orthographic:
				return "Orthographic";
			case ProjectionType::Perspective:
				return "Perspective";
		}
	}

	static ProjectionType ProjectionTypeStringToEnum(const std::string& type)
	{
		if (type == "Orthographic")
			return ProjectionType::Orthographic;
		else if (type == "Perspective")
			return ProjectionType::Perspective;
	}

	static std::string RigidBodyTypeEnumToString(RigidBodyComponent::RigidBodyType type)
	{
		switch (type)
		{
			case RigidBodyComponent::RigidBodyType::Static:
				return "Static";
			case RigidBodyComponent::RigidBodyType::Dynamic:
				return "Dynamic";
		}
	}

	static RigidBodyComponent::RigidBodyType RigidBodyTypeStringToEnum(const std::string& type)
	{
		if (type == "Static")
			return RigidBodyComponent::RigidBodyType::Static;
		else if (type == "Dynamic")
			return RigidBodyComponent::RigidBodyType::Dynamic;
	}

	static std::string AxisTypeEnumToString(AxisType type)
	{
		switch (type)
		{
			case AxisType::X:
				return "X";
			case AxisType::Y:
				return "Y";
			case AxisType::Z:
				return "Z";
		}
	}

	static AxisType AxisTypeStringToEnum(const std::string& type)
	{
		if (type == "X")
			return AxisType::X;
		else if (type == "Y")
			return AxisType::Y;
		else if (type == "Z")
			return AxisType::Z;
	}

	Ref<Scene> SceneSerializer::DeserializeIntoNewScene(const char* path)
	{
		Ref<Scene> newScene = CreateRef<Scene>();
		if (DeserializeIntoExistingScene(path, newScene))
			return newScene;
		return nullptr;
	}

	bool SceneSerializer::DeserializeIntoExistingScene(const char* path, const Ref<Scene>& destScene)
	{
		FBY_SCOPED_TIMER("Scene Deserialization");

		std::ifstream in(path);
		std::stringstream ss;
		ss << in.rdbuf();

		YAML::Node data = YAML::Load(ss.str());
		if (!data["Scene"])
		{
			FBY_ERROR("Failed to load scene [{}]: 'Scene' attribute not present in file!", path);
			return false;
		};

		destScene->m_Name = data["Scene"].as<std::string>();

		destScene->m_Registry->clear();

		auto entities = data["Entities"];
		if (entities)
		{
			std::unordered_map<UUID, fbentt::entity> UUIDToEntityMap;
			UUIDToEntityMap[UUID(0)] = fbentt::null;
			for (const auto entity : entities)
			{
				const auto deserializedEntity = destScene->m_Registry->create();
				const UUID ID = entity["Entity"].as<UUID>();
				UUIDToEntityMap[ID] = deserializedEntity;
			}

			// Deserialize entities
			for (const auto entity : entities)
			{
				const UUID ID = entity["Entity"].as<UUID>();
				const fbentt::entity deserializedEntity = UUIDToEntityMap[ID];

				auto& IDComp = destScene->m_Registry->emplace<IDComponent>(deserializedEntity);
				IDComp.ID = ID;

				if (auto tag = entity["TagComponent"])
				{
					auto& tagComp = destScene->m_Registry->emplace<TagComponent>(deserializedEntity);
					tagComp.Tag = tag.as<std::string>();
				}

				if (auto transform = entity["TransformComponent"])
				{
					auto& transformComp = destScene->m_Registry->emplace<TransformComponent>(deserializedEntity);
					transformComp.Translation = transform["Translation"].as<glm::vec3>();
					transformComp.Rotation = transform["Rotation"].as<glm::vec3>();
					transformComp.Scale = transform["Scale"].as<glm::vec3>();
				}

				if (auto relation = entity["RelationshipComponent"])
				{
					auto& relationComp = destScene->m_Registry->emplace<RelationshipComponent>(deserializedEntity);
					relationComp.Parent = UUIDToEntityMap[relation["Parent"].as<UUID>()];
					relationComp.PrevSibling = UUIDToEntityMap[relation["PrevSibling"].as<UUID>()];
					relationComp.NextSibling = UUIDToEntityMap[relation["NextSibling"].as<UUID>()];
					relationComp.FirstChild = UUIDToEntityMap[relation["FirstChild"].as<UUID>()];
				}

				if (auto text = entity["TextComponent"])
				{
					auto& textComp = destScene->m_Registry->emplace<TextComponent>(deserializedEntity);
					textComp.TextString = text["TextString"].as<std::string>();
					textComp.Font = text["Font"].as<AssetHandle>();
					textComp.Color = text["Color"].as<glm::vec3>();
					textComp.Kerning = text["Kerning"].as<float>();
					textComp.LineSpacing = text["LineSpacing"].as<float>();
				}

				if (auto camera = entity["CameraComponent"])
				{
					auto& cameraComp = destScene->m_Registry->emplace<CameraComponent>(deserializedEntity);
					cameraComp.IsPrimary = camera["IsPrimary"].as<bool>();

					ProjectionType type = ProjectionTypeStringToEnum(camera["ProjectionType"].as<std::string>());
					float aspectRatio = camera["AspectRatio"].as<float>();
					float FOV_or_Zoom = camera["FOV/Zoom"].as<float>();
					float near = camera["Near"].as<float>();
					float far = camera["Far"].as<float>();

					switch (type)
					{
						case ProjectionType::Orthographic:
							cameraComp.Camera.SetOrthographic(aspectRatio, FOV_or_Zoom, near, far);
							break;
						case ProjectionType::Perspective:
							cameraComp.Camera.SetPerspective(aspectRatio, FOV_or_Zoom, near, far);
							break;
					}
				}

				if (auto mesh = entity["MeshComponent"])
				{
					auto& meshComp = destScene->m_Registry->emplace<MeshComponent>(deserializedEntity, 0);
					meshComp.MeshHandle = mesh["MeshHandle"].as<AssetHandle>();

					for (auto entry : mesh["OverridenMaterialTable"])
						meshComp.OverridenMaterialTable[entry["SubmeshIndex"].as<uint32_t>()] = entry["Material"].as<AssetHandle>();
				}

				if (auto skyLight = entity["SkyLightComponent"])
				{
					auto& skyLightComp = destScene->m_Registry->emplace<SkyLightComponent>(deserializedEntity);
					skyLightComp.Color = skyLight["Color"].as<glm::vec3>();
					skyLightComp.Intensity = skyLight["Intensity"].as<float>();
					skyLightComp.EnableSkymap = skyLight["EnableSkymap"].as<bool>();
					skyLightComp.Skymap = skyLight["Skymap"].as<AssetHandle>();
				}

				if (auto light = entity["DirectionalLightComponent"])
				{
					auto& lightComp = destScene->m_Registry->emplace<DirectionalLightComponent>(deserializedEntity);
					lightComp.Color = light["Color"].as<glm::vec3>();
					lightComp.Intensity = light["Intensity"].as<float>();
					lightComp.LightSize = light["LightSize"].as<float>();
				}

				if (auto light = entity["PointLightComponent"])
				{
					auto& lightComp = destScene->m_Registry->emplace<PointLightComponent>(deserializedEntity);
					lightComp.Color = light["Color"].as<glm::vec3>();
					lightComp.Intensity = light["Intensity"].as<float>();
				}

				if (auto light = entity["SpotLightComponent"])
				{
					auto& lightComp = destScene->m_Registry->emplace<SpotLightComponent>(deserializedEntity);
					lightComp.Color = light["Color"].as<glm::vec3>();
					lightComp.Intensity = light["Intensity"].as<float>();
					lightComp.InnerConeAngle = light["InnerConeAngle"].as<float>();
					lightComp.OuterConeAngle = light["OuterConeAngle"].as<float>();
				}

				if (auto rigidBody = entity["RigidBodyComponent"])
				{
					auto& rbComp = destScene->m_Registry->emplace<RigidBodyComponent>(deserializedEntity);
					rbComp.Type = RigidBodyTypeStringToEnum(rigidBody["Type"].as<std::string>());
					rbComp.Density = rigidBody["Density"].as<float>();
					rbComp.StaticFriction = rigidBody["StaticFriction"].as<float>();
					rbComp.DynamicFriction = rigidBody["DynamicFriction"].as<float>();
					rbComp.Restitution = rigidBody["Restitution"].as<float>();
				}

				if (auto boxCollider = entity["BoxColliderComponent"])
				{
					auto& bcComp = destScene->m_Registry->emplace<BoxColliderComponent>(deserializedEntity);
					bcComp.Size = boxCollider["Size"].as<glm::vec3>();
				}

				if (auto sphereCollider = entity["SphereColliderComponent"])
				{
					auto& scComp = destScene->m_Registry->emplace<SphereColliderComponent>(deserializedEntity);
					scComp.Radius = sphereCollider["Radius"].as<float>();
				}

				if (auto capsuleCollider = entity["CapsuleColliderComponent"])
				{
					auto& ccComp = destScene->m_Registry->emplace<CapsuleColliderComponent>(deserializedEntity);
					ccComp.Axis = AxisTypeStringToEnum(capsuleCollider["Axis"].as<std::string>());
					ccComp.Radius = capsuleCollider["Radius"].as<float>();
					ccComp.Height = capsuleCollider["Height"].as<float>();
				}
			}
		}
		return true;
	}

	void SceneSerializer::SerializeSceneToFile(const char* path, const Ref<Scene>& srcScene)
	{
		FBY_SCOPED_TIMER("Serialization");

		std::string scenePath(path);
		uint32_t lastSlashPosition = scenePath.find_last_of('/') + 1;
		uint32_t lastDotPosition = scenePath.find_last_of('.');
		std::string sceneName = scenePath.substr(lastSlashPosition, lastDotPosition - lastSlashPosition);

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Scene" << YAML::Value << sceneName;

		out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

		std::set<UUID> meshHandles;

		srcScene->m_Registry->for_each([&](fbentt::entity entity) {
			SerializeEntity(out, entity, srcScene, meshHandles);
		});
		out << YAML::EndSeq; // Entities

		out << YAML::EndMap; // Scene

		std::ofstream fout(scenePath);
		fout << out.c_str();
	}

	void SceneSerializer::SerializeEntity(YAML::Emitter& out, const fbentt::entity& entity, const Ref<Scene>& scene, std::set<UUID>& meshUUIDs)
	{
		out << YAML::BeginMap;
		out << YAML::Key << "Entity" << YAML::Value << scene->m_Registry->get<IDComponent>(entity).ID;

		out << YAML::Key << "TagComponent" << YAML::Value << scene->m_Registry->get<TagComponent>(entity).Tag;

		if (scene->m_Registry->has<TransformComponent>(entity))
		{
			auto& transform = scene->m_Registry->get<TransformComponent>(entity);
			out << YAML::Key << "TransformComponent" << YAML::BeginMap;
			out << YAML::Key << "Translation" << YAML::Value << transform.Translation;
			out << YAML::Key << "Rotation" << YAML::Value << transform.Rotation;
			out << YAML::Key << "Scale" << YAML::Value << transform.Scale;
			out << YAML::EndMap; // Transform Component
		}

		if (scene->m_Registry->has<RelationshipComponent>(entity))
		{
			auto& relation = scene->m_Registry->get<RelationshipComponent>(entity);
			out << YAML::Key << "RelationshipComponent" << YAML::BeginMap;

			const UUID parentID = relation.Parent != fbentt::null ? scene->m_Registry->get<IDComponent>(relation.Parent).ID : UUID(0);
			const UUID prevSiblingID = relation.PrevSibling != fbentt::null ? scene->m_Registry->get<IDComponent>(relation.PrevSibling).ID : UUID(0);
			const UUID nextSiblingID = relation.NextSibling != fbentt::null ? scene->m_Registry->get<IDComponent>(relation.NextSibling).ID : UUID(0);
			const UUID firstChildID = relation.FirstChild != fbentt::null ? scene->m_Registry->get<IDComponent>(relation.FirstChild).ID : UUID(0);

			out << YAML::Key << "Parent" << YAML::Value << parentID;
			out << YAML::Key << "PrevSibling" << YAML::Value << prevSiblingID;
			out << YAML::Key << "NextSibling" << YAML::Value << nextSiblingID;
			out << YAML::Key << "FirstChild" << YAML::Value << firstChildID;
			out << YAML::EndMap; // Relationship Component
		}

		if (scene->m_Registry->has<TextComponent>(entity))
		{
			auto& text = scene->m_Registry->get<TextComponent>(entity);
			out << YAML::Key << "TextComponent" << YAML::BeginMap;
			out << YAML::Key << "TextString" << YAML::Value << text.TextString;
			out << YAML::Key << "Font" << YAML::Value << text.Font;
			out << YAML::Key << "Color" << YAML::Value << text.Color;
			out << YAML::Key << "Kerning" << YAML::Value << text.Kerning;
			out << YAML::Key << "LineSpacing" << YAML::Value << text.LineSpacing;
			out << YAML::EndMap; // Text Component
		}

		if (scene->m_Registry->has<CameraComponent>(entity))
		{
			auto& cameraComp = scene->m_Registry->get<CameraComponent>(entity);
			const auto& settings = cameraComp.Camera.GetSettings();
			out << YAML::Key << "CameraComponent" << YAML::BeginMap;
			out << YAML::Key << "IsPrimary" << YAML::Value << cameraComp.IsPrimary;
			out << YAML::Key << "ProjectionType" << YAML::Value << ProjectionTypeEnumToString(settings.ProjectionType);
			out << YAML::Key << "AspectRatio" << YAML::Value << settings.AspectRatio;
			out << YAML::Key << "FOV/Zoom" << YAML::Value << settings.FOV;
			out << YAML::Key << "Near" << YAML::Value << settings.Near;
			out << YAML::Key << "Far" << YAML::Value << settings.Far;
			out << YAML::EndMap; // Camera Component
		}

		if (scene->m_Registry->has<MeshComponent>(entity))
		{
			auto& mesh = scene->m_Registry->get<MeshComponent>(entity);
			out << YAML::Key << "MeshComponent" << YAML::BeginMap;
			out << YAML::Key << "MeshHandle" << YAML::Value << mesh.MeshHandle;

			out << YAML::Key << "OverridenMaterialTable" << YAML::BeginSeq;

			for (const auto& [submeshIndex, materialHandle] : mesh.OverridenMaterialTable)
			{
				out << YAML::BeginMap;
				out << YAML::Key << "SubmeshIndex" << YAML::Value << submeshIndex;
				out << YAML::Key << "Material" << YAML::Value << materialHandle;
				out << YAML::EndMap;
			}

			out << YAML::EndSeq;
			out << YAML::EndMap; // Mesh Component

			meshUUIDs.insert(mesh.MeshHandle);
		}

		if (scene->m_Registry->has<SkyLightComponent>(entity))
		{
			auto& skyLight = scene->m_Registry->get<SkyLightComponent>(entity);
			out << YAML::Key << "SkyLightComponent" << YAML::BeginMap;
			out << YAML::Key << "Color" << YAML::Value << skyLight.Color;
			out << YAML::Key << "Intensity" << YAML::Value << skyLight.Intensity;
			out << YAML::Key << "EnableSkymap" << YAML::Value << skyLight.EnableSkymap;
			out << YAML::Key << "Skymap" << YAML::Value << skyLight.Skymap;
			out << YAML::EndMap; // Sky Light Component
		}

		if (scene->m_Registry->has<DirectionalLightComponent>(entity))
		{
			auto& light = scene->m_Registry->get<DirectionalLightComponent>(entity);
			out << YAML::Key << "DirectionalLightComponent" << YAML::BeginMap;
			out << YAML::Key << "Color" << YAML::Value << light.Color;
			out << YAML::Key << "Intensity" << YAML::Value << light.Intensity;
			out << YAML::Key << "LightSize" << YAML::Value << light.LightSize;
			out << YAML::EndMap; // Directional Light Component
		}

		if (scene->m_Registry->has<PointLightComponent>(entity))
		{
			auto& light = scene->m_Registry->get<PointLightComponent>(entity);
			out << YAML::Key << "PointLightComponent" << YAML::BeginMap;
			out << YAML::Key << "Color" << YAML::Value << light.Color;
			out << YAML::Key << "Intensity" << YAML::Value << light.Intensity;
			out << YAML::EndMap; // Point Light Component
		}

		if (scene->m_Registry->has<SpotLightComponent>(entity))
		{
			auto& light = scene->m_Registry->get<SpotLightComponent>(entity);
			out << YAML::Key << "SpotLightComponent" << YAML::BeginMap;
			out << YAML::Key << "Color" << YAML::Value << light.Color;
			out << YAML::Key << "Intensity" << YAML::Value << light.Intensity;
			out << YAML::Key << "InnerConeAngle" << YAML::Value << light.InnerConeAngle;
			out << YAML::Key << "OuterConeAngle" << YAML::Value << light.OuterConeAngle;
			out << YAML::EndMap; // Spot Light Component
		}

		if (scene->m_Registry->has<RigidBodyComponent>(entity))
		{
			auto& rigidBody = scene->m_Registry->get<RigidBodyComponent>(entity);
			out << YAML::Key << "RigidBodyComponent" << YAML::BeginMap;
			out << YAML::Key << "Type" << YAML::Value << RigidBodyTypeEnumToString(rigidBody.Type);
			out << YAML::Key << "Density" << YAML::Value << rigidBody.Density;
			out << YAML::Key << "StaticFriction" << YAML::Value << rigidBody.StaticFriction;
			out << YAML::Key << "DynamicFriction" << YAML::Value << rigidBody.DynamicFriction;
			out << YAML::Key << "Restitution" << YAML::Value << rigidBody.Restitution;
			out << YAML::Key << YAML::EndMap; // Rigid Body Component
		}

		if (scene->m_Registry->has<BoxColliderComponent>(entity))
		{
			auto& boxCollider = scene->m_Registry->get<BoxColliderComponent>(entity);
			out << YAML::Key << "BoxColliderComponent" << YAML::BeginMap;
			out << YAML::Key << "Size" << YAML::Value << boxCollider.Size;
			out << YAML::Key << YAML::EndMap; // Box Collider Component
		}

		if (scene->m_Registry->has<SphereColliderComponent>(entity))
		{
			auto& sphereCollider = scene->m_Registry->get<SphereColliderComponent>(entity);
			out << YAML::Key << "SphereColliderComponent" << YAML::BeginMap;
			out << YAML::Key << "Radius" << YAML::Value << sphereCollider.Radius;
			out << YAML::Key << YAML::EndMap; // Sphere Collider Component
		}

		if (scene->m_Registry->has<CapsuleColliderComponent>(entity))
		{
			auto& capsuleCollider = scene->m_Registry->get<CapsuleColliderComponent>(entity);
			out << YAML::Key << "CapsuleColliderComponent" << YAML::BeginMap;
			out << YAML::Key << "Axis" << YAML::Value << AxisTypeEnumToString(capsuleCollider.Axis);
			out << YAML::Key << "Radius" << YAML::Value << capsuleCollider.Radius;
			out << YAML::Key << "Height" << YAML::Value << capsuleCollider.Height;
			out << YAML::Key << YAML::EndMap; // Capsule Collider Component
		}

		out << YAML::EndMap; // Entity
	}

} // namespace Flameberry
