#include "ScriptEngine.h"

#include <fstream>

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/mono-config.h>
#include <mono/metadata/attrdefs.h>

#include <PxPhysicsAPI.h>

#include "Core/Core.h"
#include "Core/Input.h"

#include "ECS/Components.h"

#define FBY_REGISTER_COMPONENT(Type) ::Flameberry::RegisterComponent<Type>(#Type)
#define FBY_ADD_INTERNAL_CALL(InternalCall) mono_add_internal_call("Flameberry.Managed.InternalCalls::"#InternalCall, (const void*)InternalCalls::InternalCall);

namespace Flameberry {

    static const std::unordered_map<std::string, ScriptFieldType> s_ScriptFieldTypeMap =
    {
        { "System.Single",            ScriptFieldType::Float },
        { "System.Double",            ScriptFieldType::Double },
        { "System.Boolean",           ScriptFieldType::Boolean },
        { "System.Char",              ScriptFieldType::Char },
        { "System.Int16",             ScriptFieldType::Short },
        { "System.Int32",             ScriptFieldType::Int },
        { "System.Int64",             ScriptFieldType::Long },
        { "System.Byte",              ScriptFieldType::Byte },
        { "System.UInt16",            ScriptFieldType::UShort },
        { "System.UInt32",            ScriptFieldType::UInt },
        { "System.UInt64",            ScriptFieldType::ULong },
        { "System.Numerics.Vector2",  ScriptFieldType::Vector2 },
        { "System.Numerics.Vector3",  ScriptFieldType::Vector3 },
        { "System.Numerics.Vector4",  ScriptFieldType::Vector4 },
        { "Flameberry.Actor",         ScriptFieldType::Actor },
    };

    // ------------------------------------ Mono Utils ------------------------------------
    namespace MonoUtils {

        char* ReadBytes(const std::string& filepath, uint32_t* outSize)
        {
            std::ifstream stream(filepath, std::ios::binary | std::ios::ate);

            if (!stream)
            {
                FBY_ERROR("Failed to open the CSharp Assembly file: {}", filepath);
                return nullptr;
            }

            std::streampos end = stream.tellg();
            stream.seekg(0, std::ios::beg);
            uint32_t size = end - stream.tellg();

            if (size == 0)
            {
                FBY_ERROR("CSharp Assembly file is empty: {}", filepath);
                return nullptr;
            }

            char* buffer = new char[size];
            stream.read((char*)buffer, size);
            stream.close();

            *outSize = size;
            return buffer;
        }

        MonoAssembly* LoadMonoAssembly(const std::string& assemblyPath)
        {
            uint32_t fileSize = 0;
            char* fileData = ReadBytes(assemblyPath, &fileSize);

            // NOTE: We can't use this image for anything other than loading the assembly because this image doesn't have a reference to the assembly
            MonoImageOpenStatus status;
            MonoImage* image = mono_image_open_from_data_full(fileData, fileSize, 1, &status, 0);

            if (status != MONO_IMAGE_OK)
            {
                const char* errorMessage = mono_image_strerror(status);
                FBY_ERROR("mono_image_open_from_data_full failed with: {}", errorMessage);
                return nullptr;
            }

            MonoAssembly* assembly = mono_assembly_load_from_full(image, assemblyPath.c_str(), &status, 0);
            mono_image_close(image);

            // IMP: Free the file data
            delete[] fileData;

            return assembly;
        }

        void PrintAssemblyTypes(MonoAssembly* assembly)
        {
            MonoImage* image = mono_assembly_get_image(assembly);
            const MonoTableInfo* typeDefinitionsTable = mono_image_get_table_info(image, MONO_TABLE_TYPEDEF);
            int32_t numTypes = mono_table_info_get_rows(typeDefinitionsTable);

            for (int32_t i = 0; i < numTypes; i++)
            {
                uint32_t cols[MONO_TYPEDEF_SIZE];
                mono_metadata_decode_row(typeDefinitionsTable, i, cols, MONO_TYPEDEF_SIZE);

                const char* nameSpace = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAMESPACE]);
                const char* name = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAME]);

                printf("%s.%s\n", nameSpace, name);
            }
        }

        MonoClass* GetClassInAssembly(MonoAssembly* assembly, const char* namespaceName, const char* className)
        {
            MonoImage* image = mono_assembly_get_image(assembly);
            MonoClass* klass = mono_class_from_name(image, namespaceName, className);

            if (klass == nullptr)
            {
                FBY_ERROR("Failed to get class '{}.{}' in assembly", namespaceName, className);
                return nullptr;
            }

            return klass;
        }

        ScriptFieldType MonoTypeToScriptFieldType(MonoType* monoType)
        {
            std::string typeName = mono_type_get_name(monoType);

            auto it = s_ScriptFieldTypeMap.find(typeName);
            if (it == s_ScriptFieldTypeMap.end())
            {
                FBY_ERROR("Unknown type: {}", typeName);
                return ScriptFieldType::Invalid;
            }

            return it->second;
        }
    }

    // ------------------------------------ Script Engine ------------------------------------
    struct ScriptEngineData
    {
        MonoDomain* RootDomain;
        MonoDomain* AppDomain;

        MonoAssembly* CoreAssembly;
        MonoImage* CoreAssemblyImage;

        MonoAssembly* AppAssembly;
        MonoImage* AppAssemblyImage;

        std::filesystem::path CoreAssemblyPath, AppAssemblyPath;

        const Scene* ActiveScene;

        // Stores the Flameberry.Actor class present in Flameberry-ScriptCore
        // Used to retrieve the .ctor constructor for initilizing user subclasses of `Actor`
        Ref<ManagedClass> ActorClass;

        // Used to get the specific mono class of the class name
        // Also used to keep track of all loaded user classes
        std::unordered_map<std::string, Ref<ManagedClass>> ClassFullNameToManagedClass;
        // Used to manage runtime actors in the scene
        std::unordered_map<fbentt::entity::handle_type, Ref<ManagedActor>> ManagedActors;

        using HasComponentFunction = bool(*)(fbentt::entity entity);
        std::unordered_map<MonoType*, HasComponentFunction> ComponentTypeHashToHasComponentFunction;

        // Editor Specific Field Buffers
        std::unordered_map<fbentt::entity::handle_type, ScriptFieldBufferMap> LocalScriptFieldBufferMap;
    };

    static ScriptEngineData* s_Data;

    namespace InternalCalls {

        bool Input_IsKeyDown(KeyCode key) { return Input::IsKeyPressed(key); }

        bool Entity_HasComponent(uint64_t entity, MonoReflectionType* componentType)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");

            MonoType* managedType = mono_reflection_type_get_type(componentType);
            FBY_ASSERT(s_Data->ComponentTypeHashToHasComponentFunction.find(managedType) != s_Data->ComponentTypeHashToHasComponentFunction.end(), "HasComponent: Unknown component type");
            return s_Data->ComponentTypeHashToHasComponentFunction.at(managedType)(entity);
        }

        void TransformComponent_GetTranslation(uint64_t entity, glm::vec3& translation)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            translation = s_Data->ActiveScene->GetRegistry()->get<TransformComponent>(entity).Translation;
        }

        void TransformComponent_SetTranslation(uint64_t entity, const glm::vec3& translation)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            s_Data->ActiveScene->GetRegistry()->get<TransformComponent>(entity).Translation = translation;
        }

        void TransformComponent_GetRotation(uint64_t entity, glm::vec3& rotation)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            rotation = s_Data->ActiveScene->GetRegistry()->get<TransformComponent>(entity).Rotation;
        }

        void TransformComponent_SetRotation(uint64_t entity, const glm::vec3& rotation)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            s_Data->ActiveScene->GetRegistry()->get<TransformComponent>(entity).Rotation = rotation;
        }

        void TransformComponent_GetScale(uint64_t entity, glm::vec3& scale)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            scale = s_Data->ActiveScene->GetRegistry()->get<TransformComponent>(entity).Scale;
        }

        void TransformComponent_SetScale(uint64_t entity, const glm::vec3& scale)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            s_Data->ActiveScene->GetRegistry()->get<TransformComponent>(entity).Scale = scale;
        }

        void CameraComponent_GetIsPrimary(uint64_t entity, bool& isPrimary)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            isPrimary = s_Data->ActiveScene->GetRegistry()->get<CameraComponent>(entity).IsPrimary;
        }

        // Need to take in uint8_t instead of bool for some reason to make it work
        void CameraComponent_SetIsPrimary(uint64_t entity, uint8_t isPrimary)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            s_Data->ActiveScene->GetRegistry()->get<CameraComponent>(entity).IsPrimary = (bool)isPrimary;
        }

        void CameraComponent_GetProjectionType(uint64_t entity, ProjectionType& projectionType)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            projectionType = s_Data->ActiveScene->GetRegistry()->get<CameraComponent>(entity).Camera.GetSettings().ProjectionType;
        }

        void CameraComponent_SetProjectionType(uint64_t entity, const ProjectionType& projectionType)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            s_Data->ActiveScene->GetRegistry()->get<CameraComponent>(entity).Camera.SetProjectionType(projectionType);
        }

        void CameraComponent_GetFOVOrZoom(uint64_t entity, float& fovOrZoom)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            fovOrZoom = s_Data->ActiveScene->GetRegistry()->get<CameraComponent>(entity).Camera.GetSettings().FOV;
        }

        void CameraComponent_SetFOVOrZoom(uint64_t entity, float fovOrZoom)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            s_Data->ActiveScene->GetRegistry()->get<CameraComponent>(entity).Camera.UpdateWithFOVorZoom(fovOrZoom);
        }

        void CameraComponent_GetNear(uint64_t entity, float& near)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            near = s_Data->ActiveScene->GetRegistry()->get<CameraComponent>(entity).Camera.GetSettings().Near;
        }

        void CameraComponent_SetNear(uint64_t entity, float near)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            s_Data->ActiveScene->GetRegistry()->get<CameraComponent>(entity).Camera.UpdateWithNear(near);
        }

        void CameraComponent_GetFar(uint64_t entity, float& far)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            far = s_Data->ActiveScene->GetRegistry()->get<CameraComponent>(entity).Camera.GetSettings().Far;
        }

        void CameraComponent_SetFar(uint64_t entity, float far)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            s_Data->ActiveScene->GetRegistry()->get<CameraComponent>(entity).Camera.UpdateWithFar(far);
        }

        void SkyLightComponent_GetColor(uint64_t entity, glm::vec3& color)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            color = s_Data->ActiveScene->GetRegistry()->get<SkyLightComponent>(entity).Color;
        }

        void SkyLightComponent_SetColor(uint64_t entity, const glm::vec3& color)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            s_Data->ActiveScene->GetRegistry()->get<SkyLightComponent>(entity).Color = color;
        }

        void SkyLightComponent_GetIntensity(uint64_t entity, float& intensity)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            intensity = s_Data->ActiveScene->GetRegistry()->get<SkyLightComponent>(entity).Intensity;
        }

        void SkyLightComponent_SetIntensity(uint64_t entity, float intensity)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            s_Data->ActiveScene->GetRegistry()->get<SkyLightComponent>(entity).Intensity = intensity;
        }

        void SkyLightComponent_GetEnableSkyMap(uint64_t entity, bool& enableSkyMap)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            enableSkyMap = s_Data->ActiveScene->GetRegistry()->get<SkyLightComponent>(entity).EnableSkyMap;
        }

        void SkyLightComponent_SetEnableSkyMap(uint64_t entity, uint8_t enableSkyMap)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            s_Data->ActiveScene->GetRegistry()->get<SkyLightComponent>(entity).EnableSkyMap = (bool)enableSkyMap;
        }

        void SkyLightComponent_GetEnableReflections(uint64_t entity, bool& enableReflections)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            enableReflections = s_Data->ActiveScene->GetRegistry()->get<SkyLightComponent>(entity).EnableReflections;
        }

        void SkyLightComponent_SetEnableReflections(uint64_t entity, uint8_t enableReflections)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            s_Data->ActiveScene->GetRegistry()->get<SkyLightComponent>(entity).EnableReflections = (bool)enableReflections;
        }

        void DirectionalLightComponent_GetColor(uint64_t entity, glm::vec3& color)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            color = s_Data->ActiveScene->GetRegistry()->get<DirectionalLightComponent>(entity).Color;
        }

        void DirectionalLightComponent_SetColor(uint64_t entity, const glm::vec3& color)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            s_Data->ActiveScene->GetRegistry()->get<DirectionalLightComponent>(entity).Color = color;
        }

        void DirectionalLightComponent_GetIntensity(uint64_t entity, float& intensity)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            intensity = s_Data->ActiveScene->GetRegistry()->get<DirectionalLightComponent>(entity).Intensity;
        }

        void DirectionalLightComponent_SetIntensity(uint64_t entity, const float& intensity)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            s_Data->ActiveScene->GetRegistry()->get<DirectionalLightComponent>(entity).Intensity = intensity;
        }

        void DirectionalLightComponent_GetLightSize(uint64_t entity, float& lightSize)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            lightSize = s_Data->ActiveScene->GetRegistry()->get<DirectionalLightComponent>(entity).LightSize;
        }

        void DirectionalLightComponent_SetLightSize(uint64_t entity, const float& lightSize)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            s_Data->ActiveScene->GetRegistry()->get<DirectionalLightComponent>(entity).LightSize = lightSize;
        }

        void PointLightComponent_GetColor(uint64_t entity, glm::vec3& color)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            color = s_Data->ActiveScene->GetRegistry()->get<PointLightComponent>(entity).Color;
        }

        void PointLightComponent_SetColor(uint64_t entity, const glm::vec3& color)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            s_Data->ActiveScene->GetRegistry()->get<PointLightComponent>(entity).Color = color;
        }

        void PointLightComponent_GetIntensity(uint64_t entity, float& intensity)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            intensity = s_Data->ActiveScene->GetRegistry()->get<PointLightComponent>(entity).Intensity;
        }

        void PointLightComponent_SetIntensity(uint64_t entity, const float& intensity)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            s_Data->ActiveScene->GetRegistry()->get<PointLightComponent>(entity).Intensity = intensity;
        }

        void RigidBodyComponent_GetRigidBodyType(uint64_t entity, RigidBodyComponent::RigidBodyType& rigidBodyType)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            rigidBodyType = s_Data->ActiveScene->GetRegistry()->get<RigidBodyComponent>(entity).Type;
        }

        void RigidBodyComponent_SetRigidBodyType(uint64_t entity, const RigidBodyComponent::RigidBodyType& rigidBodyType)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            s_Data->ActiveScene->GetRegistry()->get<RigidBodyComponent>(entity).Type = rigidBodyType;
        }

        void RigidBodyComponent_GetDensity(uint64_t entity, float& density)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            density = s_Data->ActiveScene->GetRegistry()->get<RigidBodyComponent>(entity).Density;
        }

        void RigidBodyComponent_SetDensity(uint64_t entity, float density)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            s_Data->ActiveScene->GetRegistry()->get<RigidBodyComponent>(entity).Density = density;
        }

        void RigidBodyComponent_GetStaticFriction(uint64_t entity, float& staticFriction)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            staticFriction = s_Data->ActiveScene->GetRegistry()->get<RigidBodyComponent>(entity).StaticFriction;
        }

        void RigidBodyComponent_SetStaticFriction(uint64_t entity, float staticFriction)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            s_Data->ActiveScene->GetRegistry()->get<RigidBodyComponent>(entity).StaticFriction = staticFriction;
        }

        void RigidBodyComponent_GetDynamicFriction(uint64_t entity, float& dynamicFriction)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            dynamicFriction = s_Data->ActiveScene->GetRegistry()->get<RigidBodyComponent>(entity).DynamicFriction;
        }

        void RigidBodyComponent_SetDynamicFriction(uint64_t entity, float dynamicFriction)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            s_Data->ActiveScene->GetRegistry()->get<RigidBodyComponent>(entity).DynamicFriction = dynamicFriction;
        }

        void RigidBodyComponent_GetRestitution(uint64_t entity, float& restitution)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            restitution = s_Data->ActiveScene->GetRegistry()->get<RigidBodyComponent>(entity).Restitution;
        }

        void RigidBodyComponent_SetRestitution(uint64_t entity, float restitution)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            s_Data->ActiveScene->GetRegistry()->get<RigidBodyComponent>(entity).Restitution = restitution;
        }

        enum class ForceMode : uint8_t { Force, Impulse, VelocityChange, Acceleration };

        void RigidBodyComponent_ApplyForce(uint64_t entity, const glm::vec3& force, const ForceMode& mode, bool autowake)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            physx::PxRigidBody* rigidBodyRuntimePtr = (physx::PxRigidBody*)s_Data->ActiveScene->GetRegistry()->get<RigidBodyComponent>(entity).RuntimeRigidBody;
            physx::PxForceMode::Enum pxMode;
            switch (mode)
            {
                case ForceMode::Force: pxMode = physx::PxForceMode::eFORCE; break;
                case ForceMode::Impulse: pxMode = physx::PxForceMode::eIMPULSE; break;
                case ForceMode::VelocityChange: pxMode = physx::PxForceMode::eVELOCITY_CHANGE; break;
                case ForceMode::Acceleration: pxMode = physx::PxForceMode::eACCELERATION; break;
            }
            rigidBodyRuntimePtr->addForce(physx::PxVec3(force.x, force.y, force.z), pxMode, autowake);
        }

        void BoxColliderComponent_GetSize(uint64_t entity, glm::vec3& size)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            size = s_Data->ActiveScene->GetRegistry()->get<BoxColliderComponent>(entity).Size;
        }

        void BoxColliderComponent_SetSize(uint64_t entity, const glm::vec3& size)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            s_Data->ActiveScene->GetRegistry()->get<BoxColliderComponent>(entity).Size = size;
        }

        void SphereColliderComponent_GetRadius(uint64_t entity, float& radius)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            radius = s_Data->ActiveScene->GetRegistry()->get<SphereColliderComponent>(entity).Radius;
        }

        void CapsuleColliderComponent_GetAxisType(uint64_t entity, AxisType& axisType)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            axisType = s_Data->ActiveScene->GetRegistry()->get<CapsuleColliderComponent>(entity).Axis;
        }

        void CapsuleColliderComponent_SetAxisType(uint64_t entity, const AxisType& axisType)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            s_Data->ActiveScene->GetRegistry()->get<CapsuleColliderComponent>(entity).Axis = axisType;
        }

        void CapsuleColliderComponent_GetRadius(uint64_t entity, float& radius)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            radius = s_Data->ActiveScene->GetRegistry()->get<CapsuleColliderComponent>(entity).Radius;
        }

        void CapsuleColliderComponent_SetRadius(uint64_t entity, float radius)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            s_Data->ActiveScene->GetRegistry()->get<CapsuleColliderComponent>(entity).Radius = radius;
        }

        void CapsuleColliderComponent_GetHeight(uint64_t entity, float& height)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            height = s_Data->ActiveScene->GetRegistry()->get<CapsuleColliderComponent>(entity).Height;
        }

        void CapsuleColliderComponent_SetHeight(uint64_t entity, float height)
        {
            FBY_ASSERT(s_Data->ActiveScene, "InternalCall: Active scene must not be null");
            s_Data->ActiveScene->GetRegistry()->get<CapsuleColliderComponent>(entity).Height = height;
        }
    }

    void ScriptEngine::Init(const std::filesystem::path& appAssemblyPath)
    {
        s_Data = new ScriptEngineData();
        InitMono();

        s_Data->AppAssemblyPath = appAssemblyPath;
        if (std::filesystem::exists(s_Data->AppAssemblyPath))
            LoadAssembliesAndSetup();
    }

    void ScriptEngine::InitMono()
    {
        // TODO: Remove the absolute path usage
        mono_set_dirs("/Users/flameberry/Installations/mono/install/lib", "/Users/flameberry/Installations/mono/install/etc");
        mono_config_parse("/Users/flameberry/Installations/mono/install/etc/mono/config");

        // Initialize Mono JIT Runtime
        MonoDomain* rootDomain = mono_jit_init("Flameberry-ScriptCore");
        if (!rootDomain)
        {
            FBY_ERROR("Failed to initialize mono jit runtime");
            return;
        }

        // Store Root Domain
        s_Data->RootDomain = rootDomain;
        FBY_INFO("Initialized Mono JIT runtime");
    }

    void ScriptEngine::LoadAssembliesAndSetup()
    {
        LoadCoreAssembly();
        LoadAppAssembly(s_Data->AppAssemblyPath);

        // Load the core Flameberry.Actor class
        s_Data->ActorClass = CreateRef<ManagedClass>(s_Data->CoreAssemblyImage, "Flameberry", "Actor");

        // Set Internal Calls
        using namespace InternalCalls;

        FBY_ADD_INTERNAL_CALL(Input_IsKeyDown);

        FBY_ADD_INTERNAL_CALL(Entity_HasComponent);

        FBY_ADD_INTERNAL_CALL(TransformComponent_GetTranslation);
        FBY_ADD_INTERNAL_CALL(TransformComponent_SetTranslation);
        FBY_ADD_INTERNAL_CALL(TransformComponent_GetRotation);
        FBY_ADD_INTERNAL_CALL(TransformComponent_SetRotation);
        FBY_ADD_INTERNAL_CALL(TransformComponent_GetScale);
        FBY_ADD_INTERNAL_CALL(TransformComponent_SetScale);

        FBY_ADD_INTERNAL_CALL(CameraComponent_GetIsPrimary);
        FBY_ADD_INTERNAL_CALL(CameraComponent_SetIsPrimary);
        FBY_ADD_INTERNAL_CALL(CameraComponent_GetProjectionType);
        FBY_ADD_INTERNAL_CALL(CameraComponent_SetProjectionType);
        FBY_ADD_INTERNAL_CALL(CameraComponent_GetFOVOrZoom);
        FBY_ADD_INTERNAL_CALL(CameraComponent_SetFOVOrZoom);
        FBY_ADD_INTERNAL_CALL(CameraComponent_GetNear);
        FBY_ADD_INTERNAL_CALL(CameraComponent_SetNear);
        FBY_ADD_INTERNAL_CALL(CameraComponent_GetFar);
        FBY_ADD_INTERNAL_CALL(CameraComponent_SetFar);

        FBY_ADD_INTERNAL_CALL(SkyLightComponent_GetColor);
        FBY_ADD_INTERNAL_CALL(SkyLightComponent_SetColor);
        FBY_ADD_INTERNAL_CALL(SkyLightComponent_GetIntensity);
        FBY_ADD_INTERNAL_CALL(SkyLightComponent_SetIntensity);
        FBY_ADD_INTERNAL_CALL(SkyLightComponent_GetEnableSkyMap);
        FBY_ADD_INTERNAL_CALL(SkyLightComponent_SetEnableSkyMap);
        FBY_ADD_INTERNAL_CALL(SkyLightComponent_GetEnableReflections);
        FBY_ADD_INTERNAL_CALL(SkyLightComponent_SetEnableReflections);

        FBY_ADD_INTERNAL_CALL(DirectionalLightComponent_GetColor);
        FBY_ADD_INTERNAL_CALL(DirectionalLightComponent_SetColor);
        FBY_ADD_INTERNAL_CALL(DirectionalLightComponent_GetIntensity);
        FBY_ADD_INTERNAL_CALL(DirectionalLightComponent_SetIntensity);
        FBY_ADD_INTERNAL_CALL(DirectionalLightComponent_GetLightSize);
        FBY_ADD_INTERNAL_CALL(DirectionalLightComponent_SetLightSize);

        FBY_ADD_INTERNAL_CALL(PointLightComponent_GetColor);
        FBY_ADD_INTERNAL_CALL(PointLightComponent_SetColor);
        FBY_ADD_INTERNAL_CALL(PointLightComponent_GetIntensity);
        FBY_ADD_INTERNAL_CALL(PointLightComponent_SetIntensity);

        FBY_ADD_INTERNAL_CALL(RigidBodyComponent_GetRigidBodyType);
        FBY_ADD_INTERNAL_CALL(RigidBodyComponent_SetRigidBodyType);
        FBY_ADD_INTERNAL_CALL(RigidBodyComponent_GetDensity);
        FBY_ADD_INTERNAL_CALL(RigidBodyComponent_SetDensity);
        FBY_ADD_INTERNAL_CALL(RigidBodyComponent_GetStaticFriction);
        FBY_ADD_INTERNAL_CALL(RigidBodyComponent_SetStaticFriction);
        FBY_ADD_INTERNAL_CALL(RigidBodyComponent_GetDynamicFriction);
        FBY_ADD_INTERNAL_CALL(RigidBodyComponent_SetDynamicFriction);
        FBY_ADD_INTERNAL_CALL(RigidBodyComponent_GetRestitution);
        FBY_ADD_INTERNAL_CALL(RigidBodyComponent_SetRestitution);
        FBY_ADD_INTERNAL_CALL(RigidBodyComponent_ApplyForce);

        FBY_ADD_INTERNAL_CALL(BoxColliderComponent_GetSize);
        // FBY_ADD_INTERNAL_CALL(BoxColliderComponent_SetSize);

        FBY_ADD_INTERNAL_CALL(SphereColliderComponent_GetRadius);
        // FBY_ADD_INTERNAL_CALL(SphereColliderComponent_SetRadius);

        FBY_ADD_INTERNAL_CALL(CapsuleColliderComponent_GetAxisType);
        FBY_ADD_INTERNAL_CALL(CapsuleColliderComponent_SetAxisType);
        FBY_ADD_INTERNAL_CALL(CapsuleColliderComponent_GetRadius);
        FBY_ADD_INTERNAL_CALL(CapsuleColliderComponent_SetRadius);
        FBY_ADD_INTERNAL_CALL(CapsuleColliderComponent_GetHeight);
        FBY_ADD_INTERNAL_CALL(CapsuleColliderComponent_SetHeight);

        RegisterAllComponents();
    }

    void ScriptEngine::LoadCoreAssembly()
    {
        // Create App Domain
        s_Data->AppDomain = mono_domain_create_appdomain("Flameberry-ScriptCoreRuntime", nullptr);
        mono_domain_set(s_Data->AppDomain, true);

        // Loading Core Mono Assembly
        s_Data->CoreAssemblyPath = FBY_PROJECT_DIR"Flameberry-ScriptCore/bin/Release/net7.0/Flameberry-ScriptCore.dll";
        s_Data->CoreAssembly = MonoUtils::LoadMonoAssembly(s_Data->CoreAssemblyPath);
        s_Data->CoreAssemblyImage = mono_assembly_get_image(s_Data->CoreAssembly);
    }

    void ScriptEngine::LoadAppAssembly(const std::string& assemblyPath)
    {
        s_Data->AppAssemblyPath = assemblyPath;
        s_Data->AppAssembly = MonoUtils::LoadMonoAssembly(s_Data->AppAssemblyPath);
        s_Data->AppAssemblyImage = mono_assembly_get_image(s_Data->AppAssembly);

        // Load also the user defined actor classes
        const MonoTableInfo* typeDefinitionsTable = mono_image_get_table_info(s_Data->AppAssemblyImage, MONO_TABLE_TYPEDEF);
        int32_t numTypes = mono_table_info_get_rows(typeDefinitionsTable);
        MonoClass* actorClass = mono_class_from_name(s_Data->CoreAssemblyImage, "Flameberry", "Actor");

        for (int32_t i = 0; i < numTypes; i++)
        {
            uint32_t cols[MONO_TYPEDEF_SIZE];
            mono_metadata_decode_row(typeDefinitionsTable, i, cols, MONO_TYPEDEF_SIZE);

            const char* nameSpace = mono_metadata_string_heap(s_Data->AppAssemblyImage, cols[MONO_TYPEDEF_NAMESPACE]);
            const char* className = mono_metadata_string_heap(s_Data->AppAssemblyImage, cols[MONO_TYPEDEF_NAME]);

            MonoClass* monoClass = mono_class_from_name(s_Data->AppAssemblyImage, nameSpace, className);

            // Only register those classes which are a subclass of `Actor`
            if (!mono_class_is_subclass_of(monoClass, actorClass, false))
                continue;

            const std::string& fullName = fmt::format("{}.{}", nameSpace, className);

            // Create and store the managed class
            Ref<ManagedClass> userClass = CreateRef<ManagedClass>(s_Data->AppAssemblyImage, nameSpace, className);
            s_Data->ClassFullNameToManagedClass[fullName] = userClass;

            FBY_LOG("Registered Class from App Assembly: {}", fullName);
        }
    }

    template<typename Component>
    static void RegisterComponent(const char* name)
    {
        MonoType* managedType = mono_reflection_type_from_name((char*)fmt::format("Flameberry.{}", name).c_str(), s_Data->CoreAssemblyImage);
        FBY_ASSERT(managedType != nullptr, "Internal Error: Component not available in Script-Core");

        s_Data->ComponentTypeHashToHasComponentFunction[managedType] = [](fbentt::entity entity) -> bool {
            FBY_ASSERT(s_Data->ActiveScene, "Internal Error: Scene should not be nullptr");
            return s_Data->ActiveScene->GetRegistry()->has<Component>(entity);
            };
    }

    void ScriptEngine::RegisterAllComponents()
    {
        FBY_REGISTER_COMPONENT(TransformComponent);
        FBY_REGISTER_COMPONENT(CameraComponent);
        FBY_REGISTER_COMPONENT(SkyLightComponent);
        FBY_REGISTER_COMPONENT(MeshComponent);
        FBY_REGISTER_COMPONENT(DirectionalLightComponent);
        FBY_REGISTER_COMPONENT(PointLightComponent);
        FBY_REGISTER_COMPONENT(RigidBodyComponent);
        FBY_REGISTER_COMPONENT(BoxColliderComponent);
        FBY_REGISTER_COMPONENT(SphereColliderComponent);
        FBY_REGISTER_COMPONENT(CapsuleColliderComponent);
    }

    void ScriptEngine::ReloadAppAssembly()
    {
        FBY_INFO("Reloading Script Assemblies...");
        s_Data->ComponentTypeHashToHasComponentFunction.clear();
        s_Data->ClassFullNameToManagedClass.clear();
        s_Data->LocalScriptFieldBufferMap.clear();

        // This will crash if we try to unload a non-existent app domain
        if (s_Data->AppDomain)
        {
            mono_domain_set(mono_get_root_domain(), false);
            mono_domain_unload(s_Data->AppDomain);
        }
        LoadAssembliesAndSetup();
    }

    void ScriptEngine::OnRuntimeStart(const Scene* scene)
    {
        s_Data->ActiveScene = scene;

        // No need to create any actors if no assembly is loaded
        if (!s_Data->AppAssemblyImage || !s_Data->CoreAssemblyImage)
            return;

        for (const auto entity : s_Data->ActiveScene->GetRegistry()->view<ScriptComponent>())
        {
            auto& sc = s_Data->ActiveScene->GetRegistry()->get<ScriptComponent>(entity);

            // This should prevent any empty named entity from participating in the scripting process
            if (!sc.AssemblyQualifiedClassName.empty())
            {
                Ref<ManagedClass> managedClass = s_Data->ClassFullNameToManagedClass[sc.AssemblyQualifiedClassName];
                Ref<ManagedActor> managedActor = CreateRef<ManagedActor>(managedClass, entity);

                // Copy all local script field buffer values to mono runtime objects
                if (auto it = s_Data->LocalScriptFieldBufferMap.find(entity); it != s_Data->LocalScriptFieldBufferMap.end())
                {
                    for (const auto& [index, scriptFieldBuffer] : it->second)
                    {
                        const auto& field = managedClass->GetScriptFields()[index];
                        managedActor->SetFieldBuffer(field, (void*)scriptFieldBuffer.GetBuffer());
                    }
                }

                managedActor->CallOnCreateMethod();

                s_Data->ManagedActors[entity] = managedActor;
            }
        }
    }

    void ScriptEngine::OnRuntimeUpdate(float delta)
    {
        for (const auto& [entity, managedActor] : s_Data->ManagedActors)
            managedActor->CallOnUpdateMethod(delta);
    }

    void ScriptEngine::OnRuntimeStop()
    {
        s_Data->ActiveScene = nullptr;

        for (const auto& [entity, managedActor] : s_Data->ManagedActors)
            managedActor->CallOnDestroyMethod();

        s_Data->ManagedActors.clear();
    }

    const std::unordered_map<std::string, Ref<ManagedClass>>& Flameberry::ScriptEngine::GetActorClassDictionary()
    {
        return s_Data->ClassFullNameToManagedClass;
    }

    Ref<ManagedActor> ScriptEngine::GetManagedActor(fbentt::entity entity)
    {
        auto it = s_Data->ManagedActors.find(entity);
        if (it == s_Data->ManagedActors.end())
        {
            FBY_ERROR("Failed to get managed actor with entity ID: {}", entity);
            return nullptr;
        }
        return it->second;
    }

    std::unordered_map<fbentt::entity::handle_type, ScriptFieldBufferMap>& ScriptEngine::GetLocalScriptFieldBufferMap()
    {
        return s_Data->LocalScriptFieldBufferMap;
    }

    void ScriptEngine::Shutdown()
    {
        delete s_Data;
    }

    // ------------------------------------ Managed Class ------------------------------------

    ManagedClass::ManagedClass(MonoImage* assemblyImage, const char* namespaceName, const char* className)
        : m_AssemblyImage(assemblyImage), m_NamespaceName(namespaceName), m_ClassName(className)
    {
        // Get a reference to the class
        MonoClass* klass = mono_class_from_name(m_AssemblyImage, namespaceName, className);

        if (klass == nullptr)
        {
            FBY_ERROR("Failed to get mono class from name: {}", GetFullName());
            return;
        }

        m_Class = klass;

        // Get all public fields
        int fieldCount = mono_class_num_fields(m_Class);
        FBY_INFO("Class {} has {} fields:", GetFullName(), fieldCount);

        void* iterator = nullptr;
        while (MonoClassField* field = mono_class_get_fields(m_Class, &iterator))
        {
            const char* name = mono_field_get_name(field);
            FBY_INFO("\t{}", name);

            uint32_t flags = mono_field_get_flags(field);
            if (flags & MONO_FIELD_ATTR_PUBLIC)
            {
                MonoType* fieldType = mono_field_get_type(field);
                ScriptFieldType scriptFieldType = MonoUtils::MonoTypeToScriptFieldType(fieldType);

                m_ScriptFields.push_back(
                    ScriptField{ scriptFieldType, name, field }
                );
            }
        }
    }

    MonoMethod* ManagedClass::GetClassMethod(const char* methodName, int paramCount)
    {
        MonoMethod* method = mono_class_get_method_from_name(m_Class, methodName, paramCount);

        if (method == nullptr)
        {
            FBY_ERROR("No method called \"{}\" with {} parameters in the class", methodName, paramCount);
            return nullptr;
        }

        return method;
    }

    MonoMethod* ManagedClass::TryGetClassMethod(const char* methodName, int paramCount)
    {
        return mono_class_get_method_from_name(m_Class, methodName, paramCount);
    }

    MonoObject* ManagedClass::CreateInstance()
    {
        // Allocate an instance of our class
        MonoObject* instance = mono_object_new(s_Data->AppDomain, m_Class);

        if (instance == nullptr)
        {
            FBY_ERROR("Failed to allocate an instance of the class: {}.{}", m_NamespaceName, m_ClassName);
            return nullptr;
        }

        return instance;
    }

    // ------------------------------------ Managed Actor ------------------------------------

    ManagedActor::ManagedActor(const Ref<ManagedClass>& managedClass, fbentt::entity entity)
        : m_ManagedClass(managedClass)
    {
        m_Constructor = s_Data->ActorClass->GetClassMethod(".ctor", 1);
        m_OnCreateMethod = m_ManagedClass->TryGetClassMethod("OnCreate", 0);
        m_OnUpdateMethod = m_ManagedClass->TryGetClassMethod("OnUpdate", 1);
        m_OnDestroyMethod = m_ManagedClass->TryGetClassMethod("OnDestroy", 0);

        // Allocating the managed instance
        m_Instance = m_ManagedClass->CreateInstance();

        // Call the one-parametered constructor
        void* param = &entity;
        MonoObject* exception = nullptr;
        mono_runtime_invoke(m_Constructor, m_Instance, &param, &exception);

        if (exception)
            mono_unhandled_exception(exception);
    }

    void ManagedActor::SetFieldBuffer(const ScriptField& scriptField, void* buffer)
    {
        SetFieldValueInternal(scriptField, buffer);
    }

    bool ManagedActor::GetFieldValueInternal(const ScriptField& scriptField, void* buffer)
    {
        mono_field_get_value(m_Instance, scriptField.ClassField, buffer);
        return true;
    }

    void ManagedActor::SetFieldValueInternal(const ScriptField& scriptField, void* value)
    {
        mono_field_set_value(m_Instance, scriptField.ClassField, value);
    }

    void ManagedActor::CallOnCreateMethod()
    {
        if (m_OnCreateMethod)
        {
            MonoObject* exception = nullptr;
            mono_runtime_invoke(m_OnCreateMethod, m_Instance, nullptr, &exception);

            if (exception)
                mono_unhandled_exception(exception);
        }
    }

    void ManagedActor::CallOnUpdateMethod(float delta)
    {
        if (m_OnUpdateMethod)
        {
            void* param = &delta;
            MonoObject* exception = nullptr;
            mono_runtime_invoke(m_OnUpdateMethod, m_Instance, &param, &exception);

            if (exception)
                mono_unhandled_exception(exception);
        }
    }

    void ManagedActor::CallOnDestroyMethod()
    {
        if (m_OnDestroyMethod)
        {
            MonoObject* exception = nullptr;
            mono_runtime_invoke(m_OnDestroyMethod, m_Instance, nullptr, &exception);

            if (exception)
                mono_unhandled_exception(exception);
        }
    }

    ManagedActor::~ManagedActor()
    {
    }

}
