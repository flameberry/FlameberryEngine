#pragma once

#include <string>

#include "ECS/Scene.h"

extern "C" {
    typedef struct _MonoClass MonoClass;
    typedef struct _MonoObject MonoObject;
    typedef struct _MonoMethod MonoMethod;
    typedef struct _MonoAssembly MonoAssembly;
    typedef struct _MonoImage MonoImage;
    typedef struct _MonoClassField MonoClassField;
    typedef struct _MonoString MonoString;
}

namespace Flameberry {

    class ManagedClass
    {
    public:
        ManagedClass(MonoImage* assemblyImage, const char* namespaceName, const char* className);

        MonoObject* CreateInstance();
        MonoMethod* GetClassMethod(const char* methodName, int paramCount);
        MonoMethod* TryGetClassMethod(const char* methodName, int paramCount);

        const std::string& GetFullName() const { return fmt::format("{}.{}", m_NamespaceName, m_ClassName); }
    private:
    private:
        const char* m_NamespaceName;
        const char* m_ClassName;

        MonoClass* m_Class;
        MonoImage* m_AssemblyImage;
    };

    class ManagedActor
    {
    public:
        ManagedActor(const Ref<ManagedClass>& managedClass, fbentt::entity entity);
        ~ManagedActor();

        void CallOnCreateMethod();
        void CallOnUpdateMethod(float delta);
        void CallOnDestroyMethod();
    private:
        MonoMethod* m_Constructor = nullptr;
        MonoMethod* m_OnCreateMethod = nullptr;
        MonoMethod* m_OnUpdateMethod = nullptr;
        MonoMethod* m_OnDestroyMethod = nullptr;

        MonoObject* m_Instance;
        Ref<ManagedClass> m_ManagedClass;
    };

    struct ScriptEngineData;

    class ScriptEngine
    {
    public:
        static void Init();
        static void Shutdown();

        static void ReloadAppAssembly();
        static void LoadAppAssembly(const std::string& assemblyPath);
        static const std::unordered_map<std::string, Ref<ManagedClass>>& GetActorClassDictionary();

        static void OnRuntimeStart(const Scene* scene);
        static void OnRuntimeStop();
        static void OnRuntimeUpdate(float delta);
    private:
        static void LoadCoreAssembly();
        static void InitMono();
        static void LoadAssembliesAndSetup();
    };

}
