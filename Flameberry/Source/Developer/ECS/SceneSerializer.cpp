#include "SceneSerializer.h"

#include <fstream>

#include "Core/YamlUtils.h"
#include "Core/Timer.h"
#include "Components.h"

#include "Asset/AssetManager.h"
#include "Asset/MeshLoader.h"
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

	std::string ScriptFieldTypeEnumToString(ScriptFieldType fieldType)
	{
		switch (fieldType)
		{
			case ScriptFieldType::Invalid:
				return "Invalid";
			case ScriptFieldType::Char:
				return "Char";
			case ScriptFieldType::Byte:
				return "Byte";
			case ScriptFieldType::Short:
				return "Short";
			case ScriptFieldType::Int:
				return "Int";
			case ScriptFieldType::Long:
				return "Long";
			case ScriptFieldType::UByte:
				return "UByte";
			case ScriptFieldType::UShort:
				return "UShort";
			case ScriptFieldType::UInt:
				return "UInt";
			case ScriptFieldType::ULong:
				return "ULong";
			case ScriptFieldType::Float:
				return "Float";
			case ScriptFieldType::Double:
				return "Double";
			case ScriptFieldType::Boolean:
				return "Boolean";
			case ScriptFieldType::Vector2:
				return "Vector2";
			case ScriptFieldType::Vector3:
				return "Vector3";
			case ScriptFieldType::Vector4:
				return "Vector4";
			case ScriptFieldType::Actor:
				return "Actor";
		}
	}

	ScriptFieldType ScriptFieldTypeStringToEnum(const std::string& fieldType)
	{
		if (fieldType == "Char")
			return ScriptFieldType::Char;
		else if (fieldType == "Byte")
			return ScriptFieldType::Byte;
		else if (fieldType == "Short")
			return ScriptFieldType::Short;
		else if (fieldType == "Int")
			return ScriptFieldType::Int;
		else if (fieldType == "Long")
			return ScriptFieldType::Long;
		else if (fieldType == "UByte")
			return ScriptFieldType::UByte;
		else if (fieldType == "UShort")
			return ScriptFieldType::UShort;
		else if (fieldType == "UInt")
			return ScriptFieldType::UInt;
		else if (fieldType == "ULong")
			return ScriptFieldType::ULong;
		else if (fieldType == "Float")
			return ScriptFieldType::Float;
		else if (fieldType == "Double")
			return ScriptFieldType::Double;
		else if (fieldType == "Boolean")
			return ScriptFieldType::Boolean;
		else if (fieldType == "Vector2")
			return ScriptFieldType::Vector2;
		else if (fieldType == "Vector3")
			return ScriptFieldType::Vector3;
		else if (fieldType == "Vector4")
			return ScriptFieldType::Vector4;
		else if (fieldType == "Actor")
			return ScriptFieldType::Actor;
		else
			return ScriptFieldType::Invalid;
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

		std::ifstream	  in(path);
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
				const UUID ID = entity["Entity"].as<UUID::value_type>();
				UUIDToEntityMap[ID] = deserializedEntity;
			}

			// Deserialize entities
			for (const auto entity : entities)
			{
				const UUID			 ID = entity["Entity"].as<UUID::value_type>();
				const fbentt::entity deserializedEntity = UUIDToEntityMap[ID];

				auto& IDComp = destScene->m_Registry->emplace<IDComponent>(deserializedEntity);
				IDComp.ID = ID;

				if (auto tag = entity["TagComponent"]; tag)
				{
					auto& tagComp = destScene->m_Registry->emplace<TagComponent>(deserializedEntity);
					tagComp.Tag = tag.as<std::string>();
				}

				if (auto transform = entity["TransformComponent"]; transform)
				{
					auto& transformComp = destScene->m_Registry->emplace<TransformComponent>(deserializedEntity);
					transformComp.Translation = transform["Translation"].as<glm::vec3>();
					transformComp.Rotation = transform["Rotation"].as<glm::vec3>();
					transformComp.Scale = transform["Scale"].as<glm::vec3>();
				}

				if (auto relation = entity["RelationshipComponent"]; relation)
				{
					auto& relationComp = destScene->m_Registry->emplace<RelationshipComponent>(deserializedEntity);
					relationComp.Parent = UUIDToEntityMap[relation["Parent"].as<UUID::value_type>()];
					relationComp.PrevSibling = UUIDToEntityMap[relation["PrevSibling"].as<UUID::value_type>()];
					relationComp.NextSibling = UUIDToEntityMap[relation["NextSibling"].as<UUID::value_type>()];
					relationComp.FirstChild = UUIDToEntityMap[relation["FirstChild"].as<UUID::value_type>()];
				}

				if (auto camera = entity["CameraComponent"]; camera)
				{
					auto& cameraComp = destScene->m_Registry->emplace<CameraComponent>(deserializedEntity);
					cameraComp.IsPrimary = camera["IsPrimary"].as<bool>();

					ProjectionType type = ProjectionTypeStringToEnum(camera["ProjectionType"].as<std::string>());
					float		   aspectRatio = camera["AspectRatio"].as<float>();
					float		   FOV_or_Zoom = camera["FOV/Zoom"].as<float>();
					float		   near = camera["Near"].as<float>();
					float		   far = camera["Far"].as<float>();

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

				if (auto mesh = entity["MeshComponent"]; mesh)
				{
					auto& meshComp = destScene->m_Registry->emplace<MeshComponent>(deserializedEntity, 0);
					meshComp.MeshHandle = mesh["MeshHandle"].as<UUID::value_type>();

					for (auto entry : mesh["OverridenMaterialTable"]) // TODO: Update with different format
					{
						auto mat = AssetManager::TryGetOrLoadAsset<MaterialAsset>(entry["Material"].as<std::string>());
						meshComp.OverridenMaterialTable[entry["SubmeshIndex"].as<uint32_t>()] = mat->Handle;
					}
				}

				if (auto skyLight = entity["SkyLightComponent"]; skyLight)
				{
					auto& skyLightComp = destScene->m_Registry->emplace<SkyLightComponent>(deserializedEntity);
					skyLightComp.Color = skyLight["Color"].as<glm::vec3>();
					skyLightComp.Intensity = skyLight["Intensity"].as<float>();
					skyLightComp.EnableSkyMap = skyLight["EnableSkyMap"].as<bool>();

					std::string skymapPath = skyLight["SkyMap"].as<std::string>();
					skyLightComp.SkyMap = ((skymapPath != "") ? AssetManager::TryGetOrLoadAsset<Skymap>(skyLight["SkyMap"].as<std::string>())->Handle : AssetHandle(0));
				}

				if (auto light = entity["DirectionalLightComponent"]; light)
				{
					auto& lightComp = destScene->m_Registry->emplace<DirectionalLightComponent>(deserializedEntity);
					lightComp.Color = light["Color"].as<glm::vec3>();
					lightComp.Intensity = light["Intensity"].as<float>();
					lightComp.LightSize = light["LightSize"].as<float>();
				}

				if (auto light = entity["PointLightComponent"]; light)
				{
					auto& lightComp = destScene->m_Registry->emplace<PointLightComponent>(deserializedEntity);
					lightComp.Color = light["Color"].as<glm::vec3>();
					lightComp.Intensity = light["Intensity"].as<float>();
				}

				if (auto script = entity["ScriptComponent"])
				{
					auto& scriptComp =
						destScene->m_Registry->emplace<ScriptComponent>(deserializedEntity);
					scriptComp.AssemblyQualifiedClassName =
						script["AssemblyQualifiedClassName"].as<std::string>();

					if (auto scriptFields = script["ScriptFields"])
					{
						const auto& actorClasses = ScriptEngine::GetActorClassDictionary();
						if (auto it =
								actorClasses.find(scriptComp.AssemblyQualifiedClassName);
							it != actorClasses.end())
						{
							// Filling a temporary script field map for convenience
							struct ScriptFieldIndex
							{
								const ScriptField* Field = nullptr;
								uint32_t Index = 0;
							};

							std::unordered_map<std::string, ScriptFieldIndex> scriptFieldMap;
							uint32_t i = 0;
							for (const auto& scriptField : it->second->GetScriptFields())
							{
								scriptFieldMap[scriptField.Name] = { &scriptField, i };
								i++;
							}

							// Loading the script fields
							auto& localScriptFieldBufferMap =
								ScriptEngine::GetLocalScriptFieldBufferMap();
							auto& bufferMap = localScriptFieldBufferMap[deserializedEntity];
							for (auto scriptField : scriptFields)
							{
								const auto& name = scriptField["Name"].as<std::string>();

								if (auto it = scriptFieldMap.find(name);
									it != scriptFieldMap.end())
								{
									ScriptFieldType fieldType = ScriptFieldTypeStringToEnum(
										scriptField["Type"].as<std::string>());
									if (it->second.Field->Type == fieldType)
									{
										switch (fieldType)
										{
											case ScriptFieldType::Boolean:
												bufferMap[it->second.Index].SetValue(
													scriptField["Data"].as<bool>());
												break;
											case ScriptFieldType::Char:
												bufferMap[it->second.Index].SetValue(
													scriptField["Data"].as<int8_t>());
												break;
											case ScriptFieldType::Short:
												bufferMap[it->second.Index].SetValue(
													scriptField["Data"].as<int16_t>());
												break;
											case ScriptFieldType::Int:
												bufferMap[it->second.Index].SetValue(
													scriptField["Data"].as<int32_t>());
												break;
											case ScriptFieldType::Long:
												bufferMap[it->second.Index].SetValue(
													scriptField["Data"].as<int64_t>());
												break;
											case ScriptFieldType::Byte:
												bufferMap[it->second.Index].SetValue(
													scriptField["Data"].as<uint8_t>());
												break;
											case ScriptFieldType::UShort:
												bufferMap[it->second.Index].SetValue(
													scriptField["Data"].as<uint16_t>());
												break;
											case ScriptFieldType::UInt:
												bufferMap[it->second.Index].SetValue(
													scriptField["Data"].as<uint32_t>());
												break;
											case ScriptFieldType::ULong:
												bufferMap[it->second.Index].SetValue(
													scriptField["Data"].as<uint64_t>());
												break;
											case ScriptFieldType::Float:
												bufferMap[it->second.Index].SetValue(
													scriptField["Data"].as<float>());
												break;
											case ScriptFieldType::Double:
												bufferMap[it->second.Index].SetValue(
													scriptField["Data"].as<double>());
												break;
											case ScriptFieldType::Vector2:
												bufferMap[it->second.Index].SetValue(
													scriptField["Data"].as<glm::vec2>());
												break;
											case ScriptFieldType::Vector3:
												bufferMap[it->second.Index].SetValue(
													scriptField["Data"].as<glm::vec3>());
												break;
											case ScriptFieldType::Vector4:
												bufferMap[it->second.Index].SetValue(
													scriptField["Data"].as<glm::vec4>());
												break;
										}
									}
								}
							}
						}
					}
				}

				if (auto rigidBody = entity["RigidBodyComponent"])
				{
					auto& rbComp = destScene->m_Registry->emplace<RigidBodyComponent>(
						deserializedEntity);
					rbComp.Type =
						RigidBodyTypeStringToEnum(rigidBody["Type"].as<std::string>());
					rbComp.Density = rigidBody["Density"].as<float>();
					rbComp.StaticFriction = rigidBody["StaticFriction"].as<float>();
					rbComp.DynamicFriction = rigidBody["DynamicFriction"].as<float>();
					rbComp.Restitution = rigidBody["Restitution"].as<float>();
				}

				if (auto boxCollider = entity["BoxColliderComponent"]; boxCollider)
				{
					auto& bcComp = destScene->m_Registry->emplace<BoxColliderComponent>(deserializedEntity);
					bcComp.Size = boxCollider["Size"].as<glm::vec3>();
				}

				if (auto sphereCollider = entity["SphereColliderComponent"]; sphereCollider)
				{
					auto& scComp = destScene->m_Registry->emplace<SphereColliderComponent>(deserializedEntity);
					scComp.Radius = sphereCollider["Radius"].as<float>();
				}

				if (auto capsuleCollider = entity["CapsuleColliderComponent"]; capsuleCollider)
				{
					auto& ccComp = destScene->m_Registry->emplace<CapsuleColliderComponent>(deserializedEntity);
					ccComp.Axis = AxisTypeStringToEnum(capsuleCollider["Axis"].as<std::string>());
					ccComp.Radius = capsuleCollider["Radius"].as<float>();
					ccComp.Height = capsuleCollider["Height"].as<float>();
				}
			}
		}

		auto meshes = data["Meshes"];
		if (meshes)
		{
			for (auto mesh : meshes)
			{
				auto meshAsset = MeshLoader::LoadMesh(mesh["FilePath"].as<std::string>().c_str());
				meshAsset->Handle = mesh["Mesh"].as<UUID::value_type>();
				AssetManager::RegisterAsset(meshAsset);
			}
		}
		return true;
	}

	void SceneSerializer::SerializeSceneToFile(const char* path, const Ref<Scene>& srcScene)
	{
		FBY_SCOPED_TIMER("Serialization");

		std::string scenePath(path);
		uint32_t	lastSlashPosition = scenePath.find_last_of('/') + 1;
		uint32_t	lastDotPosition = scenePath.find_last_of('.');
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

		// Serialize Meshes loaded
		out << YAML::Key << "Meshes" << YAML::Value << YAML::BeginSeq;

		for (const auto& uuid : meshHandles)
		{
			auto mesh = AssetManager::GetAsset<StaticMesh>(uuid);

			out << YAML::BeginMap;
			out << YAML::Key << "Mesh" << YAML::Value << mesh->Handle;
			out << YAML::Key << "FilePath" << YAML::Value << mesh->FilePath;
			out << YAML::EndMap; // Mesh
		}
		out << YAML::EndSeq; // Meshes
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

		if (scene->m_Registry->has<CameraComponent>(entity))
		{
			auto&		cameraComp = scene->m_Registry->get<CameraComponent>(entity);
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
			out << YAML::Key << "MeshHandle" << YAML::Value << (UUID::value_type)(mesh.MeshHandle);

			out << YAML::Key << "OverridenMaterialTable" << YAML::BeginSeq;
			for (const auto& [submeshIndex, materialHandle] : mesh.OverridenMaterialTable)
			{
				out << YAML::BeginMap;
				out << YAML::Key << "SubmeshIndex" << YAML::Value << submeshIndex;

				auto mat = AssetManager::GetAsset<MaterialAsset>(materialHandle);
				out << YAML::Key << "Material" << YAML::Value << mat->FilePath;
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
			out << YAML::Key << "EnableSkyMap" << YAML::Value << skyLight.EnableSkyMap;
			out << YAML::Key << "SkyMap" << YAML::Value << (AssetManager::IsAssetHandleValid(skyLight.SkyMap) ? AssetManager::GetAsset<Skymap>(skyLight.SkyMap)->FilePath : "");
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

		if (scene->m_Registry->has<ScriptComponent>(entity))
		{
			auto& script = scene->m_Registry->get<ScriptComponent>(entity);
			out << YAML::Key << "ScriptComponent" << YAML::BeginMap;
			out << YAML::Key << "AssemblyQualifiedClassName" << YAML::Value
				<< script.AssemblyQualifiedClassName;

			// Serialize fields
			const auto& bufferMap = ScriptEngine::GetLocalScriptFieldBufferMap();
			if (auto it = bufferMap.find(entity); it != bufferMap.end())
			{
				out << YAML::Key << "ScriptFields" << YAML::Value;
				out << YAML::BeginSeq;

				Ref<ManagedClass> managedClass =
					ScriptEngine::GetActorClassDictionary().at(
						script.AssemblyQualifiedClassName);
				const auto& scriptFields = managedClass->GetScriptFields();
				for (const auto& [index, scriptFieldBuffer] : it->second)
				{
					out << YAML::BeginMap;
					out << YAML::Key << "Name" << YAML::Value << scriptFields[index].Name;
					out << YAML::Key << "Type" << YAML::Value
						<< ScriptFieldTypeEnumToString(scriptFields[index].Type);
					out << YAML::Key << "Data" << YAML::Value;

					switch (scriptFields[index].Type)
					{
						case ScriptFieldType::Boolean:
							out << scriptFieldBuffer.GetValue<bool>();
							break;
						case ScriptFieldType::Char:
							out << scriptFieldBuffer.GetValue<int8_t>();
							break;
						case ScriptFieldType::Short:
							out << scriptFieldBuffer.GetValue<int16_t>();
							break;
						case ScriptFieldType::Int:
							out << scriptFieldBuffer.GetValue<int32_t>();
							break;
						case ScriptFieldType::Long:
							out << scriptFieldBuffer.GetValue<int64_t>();
							break;
						case ScriptFieldType::Byte:
							// This is stored as uint16_t instead of uint8_t,
							// because YAML conversion stores it as a character and while
							// deserializing it, throws a BadConversion Exception
							out << scriptFieldBuffer.GetValue<uint16_t>();
							break;
						case ScriptFieldType::UShort:
							out << scriptFieldBuffer.GetValue<uint16_t>();
							break;
						case ScriptFieldType::UInt:
							out << scriptFieldBuffer.GetValue<uint32_t>();
							break;
						case ScriptFieldType::ULong:
							out << scriptFieldBuffer.GetValue<uint64_t>();
							break;
						case ScriptFieldType::Float:
							out << scriptFieldBuffer.GetValue<float>();
							break;
						case ScriptFieldType::Double:
							out << scriptFieldBuffer.GetValue<double>();
							break;
						case ScriptFieldType::Vector2:
							out << scriptFieldBuffer.GetValue<glm::vec2>();
							break;
						case ScriptFieldType::Vector3:
							out << scriptFieldBuffer.GetValue<glm::vec3>();
							break;
						case ScriptFieldType::Vector4:
							out << scriptFieldBuffer.GetValue<glm::vec4>();
							break;
						default:
							FBY_DEBUGBREAK();
							break;
					}
					out << YAML::EndMap;
				}

				out << YAML::EndSeq; // Script Field Map
			}

			out << YAML::EndMap; // Script Component
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