#include "ScriptEngine.h"

#include <fstream>

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/mono-config.h>

#include "Core/Core.h"

#include "ECS/Components.h"

#define FBY_REGISTER_COMPONENT(Type) ::Flameberry::RegisterComponent<Type>(#Type)
#define FBY_ADD_INTERNAL_CALL(InternalCall) mono_add_internal_call("Flameberry.Managed.InternalCalls::"#InternalCall, (const void*)InternalCalls::InternalCall);

namespace Flameberry {

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

        MonoObject* InstantiateClass(MonoAssembly* assembly, MonoDomain* appDomain, const char* namespaceName, const char* className)
        {
            // Get a reference to the class we want to instantiate
            MonoClass* testingClass = GetClassInAssembly(assembly, namespaceName, className);

            // Allocate an instance of our class
            MonoObject* classInstance = mono_object_new(appDomain, testingClass);

            if (classInstance == nullptr)
            {
                FBY_ERROR("Failed to allocate an instance of the class: {}.{}", namespaceName, className);
                return nullptr;
            }

            // Call the parameterless (default) constructor
            mono_runtime_object_init(classInstance);
            // Return the instance
            return classInstance;
        }

        void CallPrintFloatVarMethod(MonoObject* objectInstance)
        {
            // Get the MonoClass pointer from the instance
            MonoClass* instanceClass = mono_object_get_class(objectInstance);

            // Get a reference to the method in the class
            MonoMethod* method = mono_class_get_method_from_name(instanceClass, "OnCreate", 0);

            if (!method)
            {
                FBY_ERROR("No method called \"OnCreate\" with 0 parameters in the class");
                return;
            }

            // Call the C# method on the objectInstance instance, and get any potential exceptions
            MonoObject* exception = nullptr;
            mono_runtime_invoke(method, objectInstance, nullptr, &exception);

            // Handle the exception
            if (exception)
                mono_print_unhandled_exception(exception);
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

        using HasComponentFunction = bool(*)(fbentt::entity entity);
        std::unordered_map<MonoType*, HasComponentFunction> ComponentTypeHashToHasComponentFunction;

        // Used to manage runtime actors in the scene
        std::vector<Ref<ManagedActor>> ManagedActors;
    };

    static ScriptEngineData* s_Data;

    namespace InternalCalls {

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
        s_Data->ClassFullNameToManagedClass.clear();

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
                managedActor->CallOnCreateMethod();

                s_Data->ManagedActors.push_back(managedActor);
            }
        }
    }

    void ScriptEngine::OnRuntimeUpdate(float delta)
    {
        for (const auto& managedActor : s_Data->ManagedActors)
            managedActor->CallOnUpdateMethod(delta);
    }

    void ScriptEngine::OnRuntimeStop()
    {
        s_Data->ActiveScene = nullptr;

        for (const auto& managedActor : s_Data->ManagedActors)
            managedActor->CallOnDestroyMethod();

        s_Data->ManagedActors.clear();
    }

    const std::unordered_map<std::string, Ref<ManagedClass>>& Flameberry::ScriptEngine::GetActorClassDictionary()
    {
        return s_Data->ClassFullNameToManagedClass;
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
