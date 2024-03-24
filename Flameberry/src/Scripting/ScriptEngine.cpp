#include "ScriptEngine.h"

#include <fstream>

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/mono-config.h>

#include "Core/Core.h"

namespace Flameberry {

    struct ScriptEngineData
    {
        MonoDomain* RootDomain, * AppDomain;
    };

    ScriptEngineData* ScriptEngine::s_Data;

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

        MonoAssembly* LoadCSharpAssembly(const std::string& assemblyPath)
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
            MonoMethod* method = mono_class_get_method_from_name(instanceClass, "PrintFloatVar", 0);

            if (!method)
            {
                FBY_ERROR("No method called \"PrintFloatVar\" with 0 parameters in the class");
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

    void ScriptEngine::Init()
    {
        s_Data = new ScriptEngineData();
        InitMono();
    }

    void ScriptEngine::InitMono()
    {
        // TODO: Remove the absolute path usage
        mono_set_dirs("/Users/flameberry/Installations/mono/install/lib", "/Users/flameberry/Installations/mono/install/etc");
        mono_config_parse("/Users/flameberry/Installations/mono/install/etc/mono/config");

        // Initialize Mono JIT Runtime
        MonoDomain* rootDomain = mono_jit_init("Flameberry-ScriptCoreRuntime");
        if (!rootDomain)
        {
            FBY_ERROR("Failed to initialize mono jit runtime");
            return;
        }

        // Store Root Domain
        s_Data->RootDomain = rootDomain;
        FBY_INFO("Initialized Mono JIT runtime");

        // Create App Domain
        s_Data->AppDomain = mono_domain_create_appdomain("Flameberry-ScriptCoreRuntimeAppDomain", nullptr);
        mono_domain_set(s_Data->AppDomain, true);

        // Loading CSharp Assembly
        MonoAssembly* assembly = MonoUtils::LoadCSharpAssembly("/Users/flameberry/Developer/ScriptingTest/ScriptingTest/bin/Debug/net7.0/ScriptingTest.dll");
        MonoUtils::PrintAssemblyTypes(assembly);

        MonoObject* testInstance = MonoUtils::InstantiateClass(assembly, s_Data->AppDomain, "ScriptingTest", "Class1");
        MonoUtils::CallPrintFloatVarMethod(testInstance);
    }

    void ScriptEngine::Shutdown()
    {
        delete s_Data;
    }
}
