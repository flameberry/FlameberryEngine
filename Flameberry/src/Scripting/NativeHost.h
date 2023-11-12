#pragma once

#include <cstddef>

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
    
    struct AssemblyContext
    {
        void* ContextHandle = nullptr;
        load_assembly_and_get_function_pointer_fn LoadAssemblyAndGetFunction = nullptr;
    };
    
    struct ManagedFunctions
    {
        using LoadAssemblyFn = void (*)(const char_t*);
        
        LoadAssemblyFn LoadAssembly;
    };
    
    struct InternalCall
    {
        void* CallPtr = nullptr;
        std::string AssemblyQualifiedName;
    };
    
    class NativeHost
    {
    public:
        void Init();
        void Shutdown();
        
        void SetInternalCall();
        void UploadInternalCalls();
        
        template<typename FuncType>
        FuncType LoadManagedFunction(const char_t* dotnetType, const char_t* methodName, const char_t* delegateTypeName = UNMANAGEDCALLERSONLY_METHOD);
    private:
        void LoadCoreManagedFunctions();
        bool LoadHostFXR();
    private:
        const char_t* m_CoreAssemblyPath = FL_PROJECT_DIR"Flameberry-ScriptCore/bin/Debug/net7.0/Flameberry-ScriptCore.dll";
        const char_t* m_RuntimeConfigPath = FL_PROJECT_DIR"Flameberry-ScriptCore/bin/Debug/net7.0/Flameberry-ScriptCore.runtimeconfig.json";
        
        HostFXRFunctions m_HostFXRFunctions;
        ManagedFunctions m_ManagedFunctions;
        AssemblyContext m_AssemblyContext;
        
        std::vector<InternalCall> m_InternalCalls;
    };
    
    template<typename FuncType>
    FuncType NativeHost::LoadManagedFunction(const char_t* dotnetType, const char_t* methodName, const char_t* delegateTypeName)
    {
        void* functionPtr = nullptr;
        int rc = m_AssemblyContext.LoadAssemblyAndGetFunction(m_CoreAssemblyPath, dotnetType, methodName, delegateTypeName, nullptr, &functionPtr);
        FL_ASSERT(rc == 0 && functionPtr, "Failed to load managed function: {0} from {1}", methodName, dotnetType);
        return (FuncType)functionPtr;
    }
    
}
