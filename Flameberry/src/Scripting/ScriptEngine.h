#pragma once

#include "NativeHost.h"

namespace Flameberry {
    
    class ScriptEngine
    {
    public:
        static void Init();
        static void Shutdown();
    private:
        static NativeHost s_NativeHost;
    };
    
}
