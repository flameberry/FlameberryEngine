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
        static const std::vector<std::string>& GetActorTypeNamesInAppAssembly();
    private:
        static void RegisterComponents();
        static void LoadCoreManagedFunctions();
        static void RegisterInternalCalls();
        static void LoadAppAssembly();
    private:
        static ScriptEngineData* s_Data;
        
        template<typename Component>
        friend void RegisterComponent(const char_t*);
    };
    
}
