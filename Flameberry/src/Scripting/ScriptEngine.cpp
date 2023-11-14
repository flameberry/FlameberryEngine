#include "ScriptEngine.h"

#include <dlfcn.h>
#include <assert.h>
#include <iostream>

#include <dotnet/hostfxr.h>
#include <dotnet/coreclr_delegates.h>

#include "Core/Core.h"
#include "Core/Input.h"

#include "ECS/Scene.h"
#include "ECS/Components.h"

#define MAX_PATH 512
#define STR(x) x
#define FL_TYPE_AS_STRING(Type) #Type

namespace Flameberry {
    
    struct ManagedFunctions
    {
        using LoadAssemblyFn = void (*)(const char_t*);
        using PrintAssemblyInfoFn = void (*)();
        
        using CreateActorWithEntityIDFn = void (*)(uint64_t, const char_t*);
        using DestroyAllActorsFn = void (*)();
        using InvokeOnCreateMethodOfActorWithIDFn = void (*)(uint64_t);
        using InvokeOnUpdateMethodOfActorWithIDFn = void (*)(uint64_t, float);
        
        using GetComponentHashCodeFn = int (*)(const char_t*);
        
        LoadAssemblyFn LoadAppAssembly;
        PrintAssemblyInfoFn PrintAssemblyInfo;
        
        CreateActorWithEntityIDFn CreateActorWithEntityID;
        DestroyAllActorsFn DestroyAllActors;
        InvokeOnCreateMethodOfActorWithIDFn InvokeOnCreateMethodOfActorWithID;
        InvokeOnUpdateMethodOfActorWithIDFn InvokeOnUpdateMethodOfActorWithID;

        GetComponentHashCodeFn GetComponentHashCode;
    };
    
    struct ScriptEngineData
    {
        NativeHost NativeHost;
        ManagedFunctions ManagedFunctions;
        Scene* SceneContext = nullptr;
    };
    
    ScriptEngineData* ScriptEngine::s_Data = nullptr;
    static std::unordered_map<int, std::function<bool(fbentt::entity)>> g_EntityHasComponentFunctionMap;
    
    namespace InternalCalls {
        
        void LogMessage(const char_t* message, uint8_t logLevel)
        {
            switch (flamelogger::LogLevel(logLevel))
            {
                case flamelogger::LogLevel::TRACE: FL_TRACE(message); break;
                case flamelogger::LogLevel::LOG: FL_LOG(message); break;
                case flamelogger::LogLevel::INFO: FL_INFO(message); break;
                case flamelogger::LogLevel::WARNING: FL_WARN(message); break;
                case flamelogger::LogLevel::ERROR: FL_ERROR(message); break;
                case flamelogger::LogLevel::CRITICAL: FL_CRITICAL(message); break;
                default: FL_LOG(message); break;
            }
        }
        
        bool Input_IsKeyDown(uint16_t keyCode)
        {
            return Input::IsKeyPressed((KeyCode)keyCode);
        }
        
        bool Entity_HasComponent(uint64_t entity, int hashcode)
        {
            FL_BASIC_ASSERT(g_EntityHasComponentFunctionMap.find(hashcode) != g_EntityHasComponentFunctionMap.end());
            return g_EntityHasComponentFunctionMap.at(hashcode)(entity);
        }
        
        glm::vec3 TransformComponent_GetTranslation(uint64_t entity)
        {
            Scene* context = ScriptEngine::GetSceneContext();
            auto& transform = context->GetRegistry()->get<TransformComponent>(entity);
            return transform.Translation;
        }
        
        void TransformComponent_SetTranslation(uint64_t entity, glm::vec3* translation)
        {
            Scene* context = ScriptEngine::GetSceneContext();
            context->GetRegistry()->get<TransformComponent>(entity).Translation = *translation;
        }
        
    }
    
    Scene* ScriptEngine::GetSceneContext() { return s_Data->SceneContext; }
    
    void ScriptEngine::Init()
    {
        s_Data = new ScriptEngineData();
        
        s_Data->NativeHost.Init();
        
        LoadCoreManagedFunctions();
        RegisterComponents();
        
        s_Data->ManagedFunctions.LoadAppAssembly("/Users/flameberry/Developer/FlameberryEngine/SandboxProject/bin/Debug/net7.0/SandboxProject.dll");
        s_Data->ManagedFunctions.PrintAssemblyInfo();
        
        s_Data->NativeHost.AddInternalCall("Flameberry.Managed.InternalCallStorage", "LogMessage", reinterpret_cast<void*>(&InternalCalls::LogMessage));
        
        s_Data->NativeHost.AddInternalCall("Flameberry.Managed.InternalCallStorage", "Entity_HasComponent", reinterpret_cast<void*>(&InternalCalls::Entity_HasComponent));
        
        s_Data->NativeHost.AddInternalCall("Flameberry.Managed.InternalCallStorage", "Input_IsKeyDown", reinterpret_cast<void*>(&InternalCalls::Input_IsKeyDown));
        
        s_Data->NativeHost.AddInternalCall("Flameberry.Managed.InternalCallStorage", "TransformComponent_GetTranslation", reinterpret_cast<void*>(&InternalCalls::TransformComponent_GetTranslation));        
        
        s_Data->NativeHost.AddInternalCall("Flameberry.Managed.InternalCallStorage", "TransformComponent_SetTranslation", reinterpret_cast<void*>(&InternalCalls::TransformComponent_SetTranslation));
        
        s_Data->NativeHost.UploadInternalCalls();
        
//        s_Data->ManagedFunctions.CreateActorWithEntityID(1212121212, "SandboxProject.Player");
//        s_Data->ManagedFunctions.InvokeOnCreateMethodOfActorWithID(1212121212);
//        s_Data->ManagedFunctions.InvokeOnUpdateMethodOfActorWithID(1212121212, 4.5f);
//        s_Data->ManagedFunctions.DestroyAllActors();
    }
    
    void ScriptEngine::LoadCoreManagedFunctions()
    {
        s_Data->ManagedFunctions.LoadAppAssembly = s_Data->NativeHost.LoadManagedFunction<ManagedFunctions::LoadAssemblyFn>("Flameberry.Managed.AppAssemblyManager, Flameberry-ScriptCore", "LoadAppAssembly");
        
        s_Data->ManagedFunctions.PrintAssemblyInfo = s_Data->NativeHost.LoadManagedFunction<ManagedFunctions::PrintAssemblyInfoFn>("Flameberry.Managed.AppAssemblyManager, Flameberry-ScriptCore", "PrintAssemblyInfo");
        
        s_Data->ManagedFunctions.CreateActorWithEntityID = s_Data->NativeHost.LoadManagedFunction<ManagedFunctions::CreateActorWithEntityIDFn>("Flameberry.Managed.ManagedActors, Flameberry-ScriptCore", "CreateActorWithEntityID");

        s_Data->ManagedFunctions.DestroyAllActors = s_Data->NativeHost.LoadManagedFunction<ManagedFunctions::DestroyAllActorsFn>("Flameberry.Managed.ManagedActors, Flameberry-ScriptCore", "DestroyAllActors");
        
        s_Data->ManagedFunctions.InvokeOnCreateMethodOfActorWithID = s_Data->NativeHost.LoadManagedFunction<ManagedFunctions::InvokeOnCreateMethodOfActorWithIDFn>("Flameberry.Managed.ManagedActors, Flameberry-ScriptCore", "InvokeOnCreateMethodOfActorWithID");

        s_Data->ManagedFunctions.InvokeOnUpdateMethodOfActorWithID = s_Data->NativeHost.LoadManagedFunction<ManagedFunctions::InvokeOnUpdateMethodOfActorWithIDFn>("Flameberry.Managed.ManagedActors, Flameberry-ScriptCore", "InvokeOnUpdateMethodOfActorWithID");
        
        s_Data->ManagedFunctions.GetComponentHashCode = s_Data->NativeHost.LoadManagedFunction<ManagedFunctions::GetComponentHashCodeFn>("Flameberry.Runtime.ComponentManager, Flameberry-ScriptCore", "GetComponentHashCode");
    }
    
    template<typename Component>
    void RegisterComponent(const char_t* assemblyQualifiedClassName)
    {
        int hashcode = ScriptEngine::s_Data->ManagedFunctions.GetComponentHashCode(assemblyQualifiedClassName);\
        if (hashcode == -1)
        {
            FL_ERROR("Failed to find component type: {0}", assemblyQualifiedClassName);
            FL_DEBUGBREAK();
            return;
        }
        g_EntityHasComponentFunctionMap[hashcode] = [](fbentt::entity entity) { return ScriptEngine::s_Data->SceneContext->GetRegistry()->has<Component>(entity); };
    }
    
    void ScriptEngine::RegisterComponents()
    {
        RegisterComponent<TransformComponent>("Flameberry.Runtime.TransformComponent");
        RegisterComponent<CameraComponent>("Flameberry.Runtime.CameraComponent");
        RegisterComponent<SkyLightComponent>("Flameberry.Runtime.SkyLightComponent");
        RegisterComponent<MeshComponent>("Flameberry.Runtime.MeshComponent");
        RegisterComponent<DirectionalLightComponent>("Flameberry.Runtime.DirectionalLightComponent");
        RegisterComponent<PointLightComponent>("Flameberry.Runtime.PointLightComponent");
        RegisterComponent<NativeScriptComponent>("Flameberry.Runtime.NativeScriptComponent");
        RegisterComponent<ScriptComponent>("Flameberry.Runtime.ScriptComponent");
        RegisterComponent<RigidBodyComponent>("Flameberry.Runtime.RigidBodyComponent");
        RegisterComponent<BoxColliderComponent>("Flameberry.Runtime.BoxColliderComponent");
        RegisterComponent<SphereColliderComponent>("Flameberry.Runtime.SphereColliderComponent");
        RegisterComponent<CapsuleColliderComponent>("Flameberry.Runtime.CapsuleColliderComponent");
    }

    void ScriptEngine::Shutdown()
    {
        s_Data->NativeHost.Shutdown();
        delete s_Data;
    }

    void ScriptEngine::OnRuntimeStart(Scene* context)
    {
        s_Data->SceneContext = context;
        
        // Call OnCreate of all instances of Actor
        for (const auto& entity : s_Data->SceneContext->m_Registry->view<ScriptComponent>())
        {
            auto& sc = s_Data->SceneContext->m_Registry->get<ScriptComponent>(entity);
            s_Data->ManagedFunctions.CreateActorWithEntityID(entity, sc.FullyQualifiedClassName.c_str());
            s_Data->ManagedFunctions.InvokeOnCreateMethodOfActorWithID(entity); // TODO: Observe if every time the CreateActorWithEntityID function is called, InvokeOnCreateMethodOfActorWithID is also called, if yes then include the Invokation of `OnCreate()` function inside the csharp side itself
        }
    }

    void ScriptEngine::OnRuntimeUpdate(float delta) 
    {
        // Call OnUpdate of all instances of Actor
        for (const auto& entity : s_Data->SceneContext->m_Registry->view<ScriptComponent>())
        {
            auto& sc = s_Data->SceneContext->m_Registry->get<ScriptComponent>(entity);
            s_Data->ManagedFunctions.InvokeOnUpdateMethodOfActorWithID(entity, delta);
        }
    }
    
    void ScriptEngine::OnRuntimeStop()
    {
        // Call OnDestroy of all instances of Actor
        s_Data->ManagedFunctions.DestroyAllActors();
        
        // Destroy all instances of Actors
        s_Data->SceneContext = nullptr;
    }
    
}
