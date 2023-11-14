#pragma once

#include "NativeHost.h"

namespace Flameberry {
    
    struct ManagedFunctions;
    class Scene;
    struct ScriptEngineData;
    
    class ScriptEngine
    {
    public:
        static void Init();
        static void Shutdown();
        
        static void OnRuntimeStart(Scene* context);
        static void OnRuntimeUpdate(float delta);
        static void OnRuntimeStop();
        
        static Scene* GetSceneContext();
    private:
        static void RegisterComponents();
        static void LoadCoreManagedFunctions();
    private:
        static ScriptEngineData* s_Data;
        
        template<typename Component>
        friend void RegisterComponent(const char_t*);
    };
    
}
