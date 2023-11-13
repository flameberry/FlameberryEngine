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

namespace Flameberry {
    
    struct ManagedFunctions
    {
        using LoadAssemblyFn = void (*)(const char_t*);
        using PrintAssemblyInfoFn = void (*)();
        
        using CreateActorWithEntityIDFn = void (*)(uint64_t, const char_t*);
        using DestroyAllActorsFn = void (*)();
        using InvokeOnCreateMethodOfActorWithIDFn = void (*)(uint64_t);
        using InvokeOnUpdateMethodOfActorWithIDFn = void (*)(uint64_t, float);
        
        LoadAssemblyFn LoadAppAssembly;
        PrintAssemblyInfoFn PrintAssemblyInfo;
        
        CreateActorWithEntityIDFn CreateActorWithEntityID;
        DestroyAllActorsFn DestroyAllActors;
        InvokeOnCreateMethodOfActorWithIDFn InvokeOnCreateMethodOfActorWithID;
        InvokeOnUpdateMethodOfActorWithIDFn InvokeOnUpdateMethodOfActorWithID;
    };

    NativeHost ScriptEngine::s_NativeHost;
    ManagedFunctions ScriptEngine::s_ManagedFunctions;
    Scene* ScriptEngine::s_SceneContext = nullptr;
    
    void ScriptEngine::Init()
    {
        s_NativeHost.Init();
        
        LoadCoreManagedFunctions();
        
        s_ManagedFunctions.LoadAppAssembly("/Users/flameberry/Developer/FlameberryEngine/SandboxProject/bin/Debug/net7.0/SandboxProject.dll");
        s_ManagedFunctions.PrintAssemblyInfo();
        
//        s_ManagedFunctions.CreateActorWithEntityID(1212121212, "SandboxProject.Player");
//        s_ManagedFunctions.InvokeOnCreateMethodOfActorWithID(1212121212);
//        s_ManagedFunctions.InvokeOnUpdateMethodOfActorWithID(1212121212, 4.5f);
//        s_ManagedFunctions.DestroyAllActors();
    }
    
    void ScriptEngine::LoadCoreManagedFunctions()
    {
        s_ManagedFunctions.LoadAppAssembly = s_NativeHost.LoadManagedFunction<ManagedFunctions::LoadAssemblyFn>("Flameberry.Managed.AppAssemblyManager, Flameberry-ScriptCore", "LoadAppAssembly");
        
        s_ManagedFunctions.PrintAssemblyInfo = s_NativeHost.LoadManagedFunction<ManagedFunctions::PrintAssemblyInfoFn>("Flameberry.Managed.AppAssemblyManager, Flameberry-ScriptCore", "PrintAssemblyInfo");
        
        s_ManagedFunctions.CreateActorWithEntityID = s_NativeHost.LoadManagedFunction<ManagedFunctions::CreateActorWithEntityIDFn>("Flameberry.Managed.ManagedActors, Flameberry-ScriptCore", "CreateActorWithEntityID");

        s_ManagedFunctions.DestroyAllActors = s_NativeHost.LoadManagedFunction<ManagedFunctions::DestroyAllActorsFn>("Flameberry.Managed.ManagedActors, Flameberry-ScriptCore", "DestroyAllActors");
        
        s_ManagedFunctions.InvokeOnCreateMethodOfActorWithID = s_NativeHost.LoadManagedFunction<ManagedFunctions::InvokeOnCreateMethodOfActorWithIDFn>("Flameberry.Managed.ManagedActors, Flameberry-ScriptCore", "InvokeOnCreateMethodOfActorWithID");

        s_ManagedFunctions.InvokeOnUpdateMethodOfActorWithID = s_NativeHost.LoadManagedFunction<ManagedFunctions::InvokeOnUpdateMethodOfActorWithIDFn>("Flameberry.Managed.ManagedActors, Flameberry-ScriptCore", "InvokeOnUpdateMethodOfActorWithID");
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
