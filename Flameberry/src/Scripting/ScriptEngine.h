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

    enum class ScriptFieldType
    {
        Invalid = 0, Char,
        Byte, Short, Int, Long,
        UByte, UShort, UInt, ULong,
        Float, Double,
        Boolean, Vector2, Vector3, Vector4,
        Actor
    };

    struct ScriptField
    {
        ScriptFieldType Type;
        std::string Name;
        MonoClassField* ClassField;
    };

    class ManagedClass
    {
    public:
        ManagedClass(MonoImage* assemblyImage, const char* namespaceName, const char* className);

        MonoObject* CreateInstance();
        MonoMethod* GetClassMethod(const char* methodName, int paramCount);
        MonoMethod* TryGetClassMethod(const char* methodName, int paramCount);

        std::string GetFullName() const { return fmt::format("{}.{}", m_NamespaceName, m_ClassName); }
        const std::unordered_map<std::string, ScriptField>& GetScriptFields() const { return m_ScriptFields; }
    private:
    private:
        const char* m_NamespaceName;
        const char* m_ClassName;

        std::unordered_map<std::string, ScriptField> m_ScriptFields;

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

        template<typename T>
        T GetFieldValue(const std::string& name)
        {
            bool success = GetFieldValueInternal(name, s_ScriptFieldBuffer);
            if (!success)
                return T();
            return *(T*)s_ScriptFieldBuffer;
        }

        template<typename T>
        void SetFieldValue(const std::string& name, const T& value)
        {
            SetFieldValueInternal(name, (void*)&value);
        }
    private:
        bool GetFieldValueInternal(const std::string& name, void* buffer);
        void SetFieldValueInternal(const std::string& name, void* value);
    private:
        MonoMethod* m_Constructor = nullptr;
        MonoMethod* m_OnCreateMethod = nullptr;
        MonoMethod* m_OnUpdateMethod = nullptr;
        MonoMethod* m_OnDestroyMethod = nullptr;

        MonoObject* m_Instance;
        Ref<ManagedClass> m_ManagedClass;

        inline static uint8_t s_ScriptFieldBuffer[16];
    };

    struct ScriptEngineData;

    class ScriptEngine
    {
    public:
        static void Init(const std::filesystem::path& appAssemblyPath);
        static void Shutdown();

        static void ReloadAppAssembly();
        static void LoadAppAssembly(const std::string& assemblyPath);
        static const std::unordered_map<std::string, Ref<ManagedClass>>& GetActorClassDictionary();

        static void OnRuntimeStart(const Scene* scene);
        static void OnRuntimeStop();
        static void OnRuntimeUpdate(float delta);

        static Ref<ManagedActor> GetManagedActor(fbentt::entity entity);
    private:
        static void InitMono();
        static void LoadCoreAssembly();
        static void LoadAssembliesAndSetup();
        static void RegisterAllComponents();
    };

}
