#include "NativeHost.h"

#include <dlfcn.h>

#include <dotnet/hostfxr.h>
#include <dotnet/coreclr_delegates.h>

#include "Core/Core.h"

namespace Flameberry {
    
    struct HostFXRFunctions
    {
        hostfxr_initialize_for_runtime_config_fn InitForRuntimeConfigFPtr;
        hostfxr_get_runtime_delegate_fn GetRuntimeDelegateFPtr;
        hostfxr_run_app_fn RunAppFPtr;
        hostfxr_close_fn CloseFPtr;
        hostfxr_set_error_writer_fn SetErrorWriterFPtr;
    };
    
    HostFXRFunctions NativeHost::s_HostFXRFunctions;
    
#ifdef WINDOWS
    void* LoadLibrary(const char_t* path)
    {
        HMODULE h = ::LoadLibraryW(path);
        FL_ASSERT(h, "HMODULE is nullptr: {0}", path);
        return (void*)h;
    }
    void* GetExport(void* h, const char* name)
    {
        void* f = ::GetProcAddress((HMODULE)h, name);
        FL_ASSERT(f, "ProcAddress of {0} is nullptr!", name);
        return f;
    }
#else
    void* LoadLibrary(const char_t* path)
    {
        void* h = dlopen(path, RTLD_LAZY | RTLD_LOCAL);
        FL_ASSERT(h, "HMODULE is nullptr: {0}", path);
        return h;
    }
    void* GetExport(void* h, const char* name)
    {
        void* f = dlsym(h, name);
        FL_ASSERT(f, "ProcAddress of {0} is nullptr!", name);
        return f;
    }
#endif
    
    bool NativeHost::LoadHostFXR()
    {
        // Load hostfxr and get desired exports
        void* lib = LoadLibrary("/usr/local/share/dotnet/host/fxr/7.0.0/libhostfxr.dylib");
        s_HostFXRFunctions.InitForRuntimeConfigFPtr = (hostfxr_initialize_for_runtime_config_fn)GetExport(lib, "hostfxr_initialize_for_runtime_config");
        s_HostFXRFunctions.GetRuntimeDelegateFPtr = (hostfxr_get_runtime_delegate_fn)GetExport(lib, "hostfxr_get_runtime_delegate");
        s_HostFXRFunctions.CloseFPtr = (hostfxr_close_fn)GetExport(lib, "hostfxr_close");
        s_HostFXRFunctions.SetErrorWriterFPtr = (hostfxr_set_error_writer_fn)GetExport(lib, "hostfxr_set_error_writer");
        
        return (s_HostFXRFunctions.InitForRuntimeConfigFPtr && s_HostFXRFunctions.GetRuntimeDelegateFPtr && s_HostFXRFunctions.CloseFPtr && s_HostFXRFunctions.SetErrorWriterFPtr);
    }
    
    void NativeHost::Init() 
    {
        FL_ASSERT(LoadHostFXR(), "Failed to load Host FXR!");
        
        s_HostFXRFunctions.SetErrorWriterFPtr([](const char_t* InMessage) {
            FL_ERROR(InMessage);
        });
    }
    
    AssemblyContext NativeHost::CreateContext(const char* runtimeConfigPath)
    {
        AssemblyContext context;
        
        // Load .NET Core
        int rc = s_HostFXRFunctions.InitForRuntimeConfigFPtr(runtimeConfigPath, nullptr, &context.ContextHandle);
        if (rc != 0 || context.ContextHandle == nullptr)
        {
            std::cerr << "Init failed: " << std::hex << std::showbase << rc << std::endl;
            s_HostFXRFunctions.CloseFPtr(context.ContextHandle);
            return context;
        }
        
        // Get the load assembly function pointer
        void* loadAssemblyAndGetFunctionPtr = nullptr;
        rc = s_HostFXRFunctions.GetRuntimeDelegateFPtr(context.ContextHandle,
                               hdt_load_assembly_and_get_function_pointer,
                               &loadAssemblyAndGetFunctionPtr);
        if (rc != 0 || loadAssemblyAndGetFunctionPtr == nullptr)
            std::cerr << "Get delegate failed: " << std::hex << std::showbase << rc << std::endl;
        
        context.LoadAssemblyAndGetFunction = (load_assembly_and_get_function_pointer_fn)loadAssemblyAndGetFunctionPtr;
        return context;
    }
    
    void NativeHost::ShutdownContext(const AssemblyContext& context)
    {
        s_HostFXRFunctions.CloseFPtr(context.ContextHandle);
    }
    
}
