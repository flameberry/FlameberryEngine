#include "ScriptEngine.h"

#include <dlfcn.h>
#include <assert.h>
#include <iostream>

#include <dotnet/hostfxr.h>
//#include <dotnet/nethost.h>
#include <dotnet/coreclr_delegates.h>

#include "Core/Core.h"

#define MAX_PATH 512
#define STR(x) x

namespace Flameberry {
    typedef void (*native_log_ptr)(int message);
    typedef void (*set_native_log_icall)(native_log_ptr callbackFunction);


    // Forward declarations
    void* load_library(const char_t*);
    void* get_export(void*, const char*);

#ifdef WINDOWS
    void* load_library(const char_t* path)
    {
        HMODULE h = ::LoadLibraryW(path);
        assert(h != nullptr);
        return (void*)h;
    }
    void* get_export(void* h, const char* name)
    {
        void* f = ::GetProcAddress((HMODULE)h, name);
        assert(f != nullptr);
        return f;
    }
#else
    void* load_library(const char_t* path)
    {
        void* h = dlopen(path, RTLD_LAZY | RTLD_LOCAL);
        assert(h != nullptr);
        return h;
    }
    void* get_export(void* h, const char* name)
    {
        void* f = dlsym(h, name);
        assert(f != nullptr);
        return f;
    }
#endif

    // Globals to hold hostfxr exports
    hostfxr_initialize_for_dotnet_command_line_fn init_for_cmd_line_fptr;
    hostfxr_initialize_for_runtime_config_fn init_for_config_fptr;
    hostfxr_get_runtime_delegate_fn get_delegate_fptr;
    hostfxr_run_app_fn run_app_fptr;
    hostfxr_close_fn close_fptr;
    hostfxr_handle cxt = nullptr;
    hostfxr_set_error_writer_fn SetHostFXRErrorWriter = nullptr;

    // Forward declarations
    load_assembly_and_get_function_pointer_fn get_dotnet_load_assembly(const char_t* assembly);

    bool load_hostfxr()
    {
        // Load hostfxr and get desired exports
        void* lib = load_library("/usr/local/share/dotnet/host/fxr/7.0.0/libhostfxr.dylib");
        init_for_config_fptr = (hostfxr_initialize_for_runtime_config_fn)get_export(lib, "hostfxr_initialize_for_runtime_config");
        get_delegate_fptr = (hostfxr_get_runtime_delegate_fn)get_export(lib, "hostfxr_get_runtime_delegate");
        close_fptr = (hostfxr_close_fn)get_export(lib, "hostfxr_close");
        SetHostFXRErrorWriter = (hostfxr_set_error_writer_fn)get_export(lib, "hostfxr_set_error_writer");

        return (init_for_config_fptr && get_delegate_fptr && close_fptr && SetHostFXRErrorWriter);
    }

    load_assembly_and_get_function_pointer_fn get_dotnet_load_assembly(const char_t* config_path)
    {
        // Load .NET Core
        void* load_assembly_and_get_function_pointer = nullptr;
        int rc = init_for_config_fptr(config_path, nullptr, &cxt);
        if (rc != 0 || cxt == nullptr)
        {
            std::cerr << "Init failed: " << std::hex << std::showbase << rc << std::endl;
            close_fptr(cxt);
            return nullptr;
        }

        // Get the load assembly function pointer
        rc = get_delegate_fptr(cxt,
            hdt_load_assembly_and_get_function_pointer,
            &load_assembly_and_get_function_pointer);
        if (rc != 0 || load_assembly_and_get_function_pointer == nullptr)
            std::cerr << "Get delegate failed: " << std::hex << std::showbase << rc << std::endl;

        return (load_assembly_and_get_function_pointer_fn)load_assembly_and_get_function_pointer;
    }

    void NativeLog(int parameter)
    {
        FL_WARN("NativeLog: {0}", parameter);
    }

    void ScriptEngine::Init()
    {
        if (!load_hostfxr())
            assert(false && "Failure: load_hostfxr()");

        SetHostFXRErrorWriter([](const char_t* InMessage)
            {
                FL_ERROR(InMessage);
            });

        //
        // STEP 2: Initialize and start the .NET Core runtime
        //
        const std::string config_path = "/Users/flameberry/Developer/Scripting/bin/Debug/net7.0/Scripting.runtimeconfig.json";
        load_assembly_and_get_function_pointer_fn load_assembly_and_get_function_pointer = nullptr;
        load_assembly_and_get_function_pointer = get_dotnet_load_assembly(config_path.c_str());
        FL_ASSERT(load_assembly_and_get_function_pointer != nullptr, "Failure: get_dotnet_load_assembly()");

        //
        // STEP 3: Load managed assembly and get function pointer to a managed method
        //
        const char_t* dotnetlib_path = STR("/Users/flameberry/Developer/Scripting/bin/Debug/net7.0/Scripting.dll");

        const char_t* dotnet_type = STR("Scripting.Class1, Scripting");
        const char_t* dotnet_type_method = STR("Hello");
        // <SnippetLoadAndGet>
        // Function pointer to managed delegate
        component_entry_point_fn hello = nullptr;

        //        int rc = load_assembly_and_get_function_pointer(dotnetlib_path,
        //                                                        dotnet_type,
        //                                                        dotnet_type_method,
        //                                                        nullptr /*delegate_type_name*/,
        //                                                        nullptr,
        //                                                        (void**)&hello);
        //    
        //        if (rc != 0 || hello == nullptr)
        //        {
        //            std::cerr << "Failure: load_assembly_and_get_function_pointer(): " << std::hex << std::showbase << rc << std::endl;
        //            FL_DEBUGBREAK();
        //        }

        const char_t* type0 = "Scripting.Class1, Scripting";
        const char_t* type_method0 = "SetNativeLogICall";

        set_native_log_icall SetNativeICall;

        int rc = load_assembly_and_get_function_pointer(dotnetlib_path,
            type0,
            type_method0,
            nullptr /*delegate_type_name*/,
            nullptr,
            (void**)&hello);

        SetNativeICall = (set_native_log_icall)hello;

        if (rc != 0 || SetNativeICall == nullptr)
        {
            std::cerr << "Failure: load_assembly_and_get_function_pointer(): " << std::hex << std::showbase << rc << std::endl;
            FL_DEBUGBREAK();
        }

        struct lib_args
        {
            void* ptr;
            int size;
        };

        native_log_ptr nativeLog;

        for (int i = 0; i < 3; ++i)
        {
            // <SnippetCallManaged>
            lib_args args
            {
                nativeLog,
                1
            };

            hello(&args, sizeof(args));
            // </SnippetCallManaged>
        }

        close_fptr(cxt);
    }

    void ScriptEngine::Shutdown()
    {
    }

}
