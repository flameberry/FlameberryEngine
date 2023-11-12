#include "ScriptEngine.h"

#include <dlfcn.h>
#include <assert.h>
#include <iostream>

#include <dotnet/hostfxr.h>
//#include <dotnet/nethost.h>
#include <dotnet/coreclr_delegates.h>

#include "Core/Core.h"
#include "NativeHost.h"

#define MAX_PATH 512
#define STR(x) x

namespace Flameberry {

    NativeHost ScriptEngine::s_NativeHost;
    
    void ScriptEngine::Init()
    {
        s_NativeHost.Init();
        s_NativeHost.Shutdown();
    }

    void ScriptEngine::Shutdown()
    {
    }

}
