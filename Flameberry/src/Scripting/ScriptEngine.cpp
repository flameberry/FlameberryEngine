#include "ScriptEngine.h"

#include <dlfcn.h>
#include <assert.h>
#include <iostream>

#include <dotnet/hostfxr.h>
#include <dotnet/coreclr_delegates.h>

#include "Core/Core.h"

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
        
        using GetComponentCountFn = int (*)();
        using GetComponentHashesFn = void* (*)();
        using FreeComponentStorageMemoryFn = void (*)();
        
        LoadAssemblyFn LoadAppAssembly;
        PrintAssemblyInfoFn PrintAssemblyInfo;
        
        CreateActorWithEntityIDFn CreateActorWithEntityID;
        DestroyAllActorsFn DestroyAllActors;
        InvokeOnCreateMethodOfActorWithIDFn InvokeOnCreateMethodOfActorWithID;
        InvokeOnUpdateMethodOfActorWithIDFn InvokeOnUpdateMethodOfActorWithID;
        
        GetComponentCountFn GetComponentCount;
        GetComponentHashesFn GetComponentHashes;
        FreeComponentStorageMemoryFn FreeComponentStorageMemory;
    };
    
    namespace InternalCalls {
        void LogMessageICall(const char_t* message, uint8_t logLevel)
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
        
        glm::vec3 Entity_GetTransformICall(fbentt::entity entity)
        {
            FL_LOG("Called Entity_GetTransform with ID: {0}", entity);
        }
    }

    NativeHost ScriptEngine::s_NativeHost;
    ManagedFunctions ScriptEngine::s_ManagedFunctions;
    Scene* ScriptEngine::s_SceneContext = nullptr;
    
    struct ComponentInfo
    {
        const char_t* Name;
        int HashCode;
    };
    
    void ScriptEngine::Init()
    {
        s_NativeHost.Init();
        
        LoadCoreManagedFunctions();
        RegisterComponents();
        
        s_ManagedFunctions.LoadAppAssembly("/Users/flameberry/Developer/FlameberryEngine/SandboxProject/bin/Debug/net7.0/SandboxProject.dll");
        s_ManagedFunctions.PrintAssemblyInfo();
        
        s_NativeHost.AddInternalCall("Flameberry.Managed.InternalCallStorage", "LogMessageICall", reinterpret_cast<void*>(&InternalCalls::LogMessageICall));
        
        s_NativeHost.AddInternalCall("Flameberry.Managed.InternalCallStorage", "Entity_GetTransformICall", reinterpret_cast<void*>(&InternalCalls::Entity_GetTransformICall));
        
        s_NativeHost.UploadInternalCalls();
        
        s_ManagedFunctions.CreateActorWithEntityID(1212121212, "SandboxProject.Player");
        s_ManagedFunctions.InvokeOnCreateMethodOfActorWithID(1212121212);
        s_ManagedFunctions.InvokeOnUpdateMethodOfActorWithID(1212121212, 4.5f);
        s_ManagedFunctions.DestroyAllActors();
    }
    
    void ScriptEngine::LoadCoreManagedFunctions()
    {
        s_ManagedFunctions.LoadAppAssembly = s_NativeHost.LoadManagedFunction<ManagedFunctions::LoadAssemblyFn>("Flameberry.Managed.AppAssemblyManager, Flameberry-ScriptCore", "LoadAppAssembly");
        
        s_ManagedFunctions.PrintAssemblyInfo = s_NativeHost.LoadManagedFunction<ManagedFunctions::PrintAssemblyInfoFn>("Flameberry.Managed.AppAssemblyManager, Flameberry-ScriptCore", "PrintAssemblyInfo");
        
        s_ManagedFunctions.CreateActorWithEntityID = s_NativeHost.LoadManagedFunction<ManagedFunctions::CreateActorWithEntityIDFn>("Flameberry.Managed.ManagedActors, Flameberry-ScriptCore", "CreateActorWithEntityID");

        s_ManagedFunctions.DestroyAllActors = s_NativeHost.LoadManagedFunction<ManagedFunctions::DestroyAllActorsFn>("Flameberry.Managed.ManagedActors, Flameberry-ScriptCore", "DestroyAllActors");
        
        s_ManagedFunctions.InvokeOnCreateMethodOfActorWithID = s_NativeHost.LoadManagedFunction<ManagedFunctions::InvokeOnCreateMethodOfActorWithIDFn>("Flameberry.Managed.ManagedActors, Flameberry-ScriptCore", "InvokeOnCreateMethodOfActorWithID");

        s_ManagedFunctions.InvokeOnUpdateMethodOfActorWithID = s_NativeHost.LoadManagedFunction<ManagedFunctions::InvokeOnUpdateMethodOfActorWithIDFn>("Flameberry.Managed.ManagedActors, Flameberry-ScriptCore", "InvokeOnUpdateMethodOfActorWithID");
        
        s_ManagedFunctions.GetComponentCount = s_NativeHost.LoadManagedFunction<ManagedFunctions::GetComponentCountFn>("Flameberry.Runtime.ComponentManager, Flameberry-ScriptCore", "GetComponentCount");
        
        s_ManagedFunctions.GetComponentHashes = s_NativeHost.LoadManagedFunction<ManagedFunctions::GetComponentHashesFn>("Flameberry.Runtime.ComponentManager, Flameberry-ScriptCore", "GetComponentHashes");
        
        s_ManagedFunctions.FreeComponentStorageMemory = s_NativeHost.LoadManagedFunction<ManagedFunctions::FreeComponentStorageMemoryFn>("Flameberry.Runtime.ComponentManager, Flameberry-ScriptCore", "FreeComponentStorageMemory");
    }
    
    void ScriptEngine::RegisterComponents()
    {
        int count = s_ManagedFunctions.GetComponentCount();
        ComponentInfo* ptr = (ComponentInfo*)s_ManagedFunctions.GetComponentHashes();
        
        for (int i = 0; i < count; i++)
        {
            FL_LOG(FL_TYPE_AS_STRING(TransformComponent));
            FL_LOG("HOST: HashCode of {0} is {1}", ptr->Name, ptr->HashCode);
            ptr++;
        }
        s_ManagedFunctions.FreeComponentStorageMemory();
    }

    void ScriptEngine::Shutdown()
    {
        s_NativeHost.Shutdown();
    }

    void ScriptEngine::OnRuntimeStart(Scene* context)
    {
        s_SceneContext = context;
        
        // Call OnCreate of all instances of Actor
        for (const auto& entity : s_SceneContext->m_Registry->view<ScriptComponent>())
        {
            auto& sc = s_SceneContext->m_Registry->get<ScriptComponent>(entity);
            s_ManagedFunctions.CreateActorWithEntityID(entity, sc.FullyQualifiedClassName.c_str());
            s_ManagedFunctions.InvokeOnCreateMethodOfActorWithID(entity); // TODO: Observe if every time the CreateActorWithEntityID function is called, InvokeOnCreateMethodOfActorWithID is also called, if yes then include the Invokation of `OnCreate()` function inside the csharp side itself
        }
    }

    void ScriptEngine::OnRuntimeUpdate(float delta) 
    {
        // Call OnUpdate of all instances of Actor
        for (const auto& entity : s_SceneContext->m_Registry->view<ScriptComponent>())
        {
            auto& sc = s_SceneContext->m_Registry->get<ScriptComponent>(entity);
            s_ManagedFunctions.InvokeOnUpdateMethodOfActorWithID(entity, delta);
        }
    }
    
    void ScriptEngine::OnRuntimeStop()
    {
        // Call OnDestroy of all instances of Actor
        s_ManagedFunctions.DestroyAllActors();
        
        // Destroy all instances of Actors
        s_SceneContext = nullptr;
    }
    
}
