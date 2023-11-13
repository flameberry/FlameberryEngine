#pragma once

#include "NativeHost.h"

namespace Flameberry {
    
    struct ManagedFunctions;
    class Scene;
    
    class ScriptEngine
    {
    public:
        static void Init();
        static void Shutdown();
        
        static void OnRuntimeStart(Scene* context);
        static void OnRuntimeUpdate(float delta);
        static void OnRuntimeStop();
    private:
        static void LoadCoreManagedFunctions();
    private:
        static NativeHost s_NativeHost;
        static ManagedFunctions s_ManagedFunctions;
        static Scene* s_SceneContext;
    };
    
}
