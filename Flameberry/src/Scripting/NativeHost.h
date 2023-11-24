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

    struct InternalCall
    {
        const char_t* AssemblyQualifiedName;
        void* NativeFunctionPtr = nullptr;
    };

    class NativeHost
    {
    public:
        void Init();
        void Shutdown();

        void AddInternalCall(const char_t* typeName, const char_t* methodName, void* funcPtr);
        void UploadInternalCalls();

        template<typename FuncType>
        FuncType LoadManagedFunction(const char_t* dotnetType, const char_t* methodName, const char_t* delegateTypeName = UNMANAGEDCALLERSONLY_METHOD);
    private:
        bool LoadHostFXR();
    private:
        const char_t* m_CoreAssemblyPath = FBY_PROJECT_DIR"Flameberry-ScriptCore/bin/Debug/net7.0/Flameberry-ScriptCore.dll";
        const char_t* m_RuntimeConfigPath = FBY_PROJECT_DIR"Flameberry-ScriptCore/bin/Debug/net7.0/Flameberry-ScriptCore.runtimeconfig.json";

        HostFXRFunctions m_HostFXRFunctions;
        AssemblyContext m_AssemblyContext;

        std::vector<InternalCall> m_InternalCallArray;
        std::vector<std::string> m_InternalCallNameArray;
    };

    template<typename FuncType>
    FuncType NativeHost::LoadManagedFunction(const char_t* dotnetType, const char_t* methodName, const char_t* delegateTypeName)
    {
        void* functionPtr = nullptr;
        int rc = m_AssemblyContext.LoadAssemblyAndGetFunction(m_CoreAssemblyPath, dotnetType, methodName, delegateTypeName, nullptr, &functionPtr);
        FBY_ASSERT(rc == 0 && functionPtr, "Failed to load managed function: {} from {}", methodName, dotnetType);
        return (FuncType)functionPtr;
    }

}
