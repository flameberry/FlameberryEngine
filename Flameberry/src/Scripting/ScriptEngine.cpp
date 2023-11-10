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
    typedef void (*native_log_ptr)(int message);
    typedef void (*set_native_log_icall)(native_log_ptr callbackFunction);

    void NativeLog(int parameter)
    {
        FL_WARN("NativeLog: {0}", parameter);
    }

    void ScriptEngine::Init()
    {
        NativeHost::Init();
        
        AssemblyContext context = NativeHost::CreateContext("/Users/flameberry/Developer/Scripting/bin/Debug/net7.0/Scripting.runtimeconfig.json");
        
        const char_t* dotnetlib_path = STR("/Users/flameberry/Developer/Scripting/bin/Debug/net7.0/Scripting.dll");

        const char_t* dotnet_type = STR("Scripting.Class1, Scripting");
        const char_t* dotnet_type_method = STR("Hello");
        // <SnippetLoadAndGet>
        // Function pointer to managed delegate
        component_entry_point_fn hello = nullptr;

        int rc = context.LoadAssemblyAndGetFunction(dotnetlib_path,
                                                        dotnet_type,
                                                        dotnet_type_method,
                                                        nullptr /*delegate_type_name*/,
                                                        nullptr,
                                                        (void**)&hello);
    
        if (rc != 0 || hello == nullptr)
        {
            std::cerr << "Failure: load_assembly_and_get_function_pointer(): " << std::hex << std::showbase << rc << std::endl;
            FL_DEBUGBREAK();
        }

        struct lib_args
        {
            const native_log_ptr ptr;
            int size;
        };
        
        native_log_ptr nativeLogPtr;

        for (int i = 0; i < 3; ++i)
        {
            lib_args args
            {
                NativeLog,
                i
            };

            hello(&args, sizeof(args));
        }

        NativeHost::ShutdownContext(context);
    }

    void ScriptEngine::Shutdown()
    {
    }

}
