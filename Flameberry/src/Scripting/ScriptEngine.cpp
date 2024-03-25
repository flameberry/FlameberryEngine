#include "ScriptEngine.h"

#include <fstream>

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/mono-config.h>

#include "Core/Core.h"

#include "ECS/Components.h"

namespace Flameberry {

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

        // Stores the Flameberry.Actor class present in Flameberry-ScriptCore
        // Used to retrieve the .ctor constructor for initilizing user subclasses of `Actor`
        Ref<ManagedClass> ActorClass;

        std::unordered_map<std::string, Ref<ManagedClass>> ClassFullNameToManagedClass;
        std::vector<Ref<ManagedActor>> ManagedActors;
    };

    static ScriptEngineData* s_Data;

    void ScriptEngine::Init()
    {
        s_Data = new ScriptEngineData();
        InitMono();

        // TODO: Load this from .fbproj
        LoadAppAssembly(FBY_PROJECT_DIR"SandboxProject/Content/Scripting/Binaries/Release/net7.0/SandboxProject.dll");

        // for (const auto& [fullName, managedClass] : s_Data->ClassFullNameToManagedClass)
        // {
        //     Ref<ManagedActor> managedActor = CreateRef<ManagedActor>(managedClass, (fbentt::entity)16);
        //     managedActor->CallOnCreateMethod();
        //     managedActor->CallOnUpdateMethod(0.13f);
        // }
    }

    void ScriptEngine::LoadAppAssembly(const std::string& assemblyPath)
    {
        s_Data->AppAssembly = MonoUtils::LoadMonoAssembly(assemblyPath);
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

        // Create App Domain
        s_Data->AppDomain = mono_domain_create_appdomain("Flameberry-ScriptCoreRuntime", nullptr);
        mono_domain_set(s_Data->AppDomain, true);

        // Loading Core Mono Assembly
        s_Data->CoreAssembly = MonoUtils::LoadMonoAssembly(FBY_PROJECT_DIR"Flameberry-ScriptCore/bin/Release/net7.0/Flameberry-ScriptCore.dll");
        s_Data->CoreAssemblyImage = mono_assembly_get_image(s_Data->CoreAssembly);

        // Load the core Flameberry.Actor class
        s_Data->ActorClass = CreateRef<ManagedClass>(s_Data->CoreAssemblyImage, "Flameberry", "Actor");
    }

    void Flameberry::ScriptEngine::OnRuntimeStart(const Scene* scene)
    {
        for (const auto entity : scene->GetRegistry()->view<ScriptComponent>())
        {
            auto& sc = scene->GetRegistry()->get<ScriptComponent>(entity);

            Ref<ManagedClass> managedClass = s_Data->ClassFullNameToManagedClass[sc.AssemblyQualifiedClassName];
            Ref<ManagedActor> managedActor = CreateRef<ManagedActor>(managedClass, entity);
            managedActor->CallOnCreateMethod();

            s_Data->ManagedActors.push_back(managedActor);
        }
    }

    void Flameberry::ScriptEngine::OnRuntimeUpdate(float delta)
    {
        for (const auto& managedActor : s_Data->ManagedActors)
            managedActor->CallOnUpdateMethod(delta);
    }

    void Flameberry::ScriptEngine::OnRuntimeStop()
    {
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
        m_OnCreateMethod = m_ManagedClass->GetClassMethod("OnCreate", 0);
        m_OnUpdateMethod = m_ManagedClass->GetClassMethod("OnUpdate", 1);

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
        MonoObject* exception = nullptr;
        mono_runtime_invoke(m_OnCreateMethod, m_Instance, nullptr, &exception);

        if (exception)
            mono_unhandled_exception(exception);
    }

    void ManagedActor::CallOnUpdateMethod(float delta)
    {
        void* param = &delta;
        MonoObject* exception = nullptr;
        mono_runtime_invoke(m_OnUpdateMethod, m_Instance, &param, &exception);

        if (exception)
            mono_unhandled_exception(exception);
    }

    ManagedActor::~ManagedActor()
    {
    }

}
