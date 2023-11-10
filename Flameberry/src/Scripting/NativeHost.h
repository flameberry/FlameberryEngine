#pragma once

#include <cstddef>
#include <dotnet/coreclr_delegates.h>

namespace Flameberry {
    
    struct HostFXRFunctions;
    
    struct AssemblyContext
    {
        void* ContextHandle = nullptr;
        load_assembly_and_get_function_pointer_fn LoadAssemblyAndGetFunction = nullptr;
    };
    
    class NativeHost
    {
    public:
        static void Init();
        static AssemblyContext CreateContext(const char* runtimeConfigPath);
        static void ShutdownContext(const AssemblyContext& context);
    private:
        static bool LoadHostFXR();
    private:
        static HostFXRFunctions s_HostFXRFunctions;
    };
    
}
