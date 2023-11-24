#include "NativeHost.h"

#include <dlfcn.h>
#include <cstring>

#include <dotnet/hostfxr.h>
#include <dotnet/coreclr_delegates.h>

#include "Core/Core.h"

namespace Flameberry {

#ifdef WINDOWS
    void* LoadLibrary(const char_t* path)
    {
        HMODULE h = ::LoadLibraryW(path);
        FBY_ASSERT(h, "HMODULE is nullptr: {}", path);
        return (void*)h;
    }
    void* GetExport(void* h, const char* name)
    {
        void* f = ::GetProcAddress((HMODULE)h, name);
        FBY_ASSERT(f, "ProcAddress of {} is nullptr!", name);
        return f;
    }
#else
    void* LoadLibrary(const char_t* path)
    {
        void* h = dlopen(path, RTLD_LAZY | RTLD_LOCAL);
        FBY_ASSERT(h, "HMODULE is nullptr: {}", path);
        return h;
    }
    void* GetExport(void* h, const char* name)
    {
        void* f = dlsym(h, name);
        FBY_ASSERT(f, "ProcAddress of {} is nullptr!", name);
        return f;
    }
#endif

    struct CoreManagedFunctions
    {
        using SetInternalCallsFn = void (*)(const void*, int);

        SetInternalCallsFn SetInternalCalls;
    };
    static CoreManagedFunctions g_CoreManagedFunctions;

    bool NativeHost::LoadHostFXR()
    {
        // Load hostfxr and get desired exports
        void* lib = LoadLibrary(FBY_HOSTFXR_LIBRARY_PATH);
        m_HostFXRFunctions.InitForRuntimeConfigFPtr = (hostfxr_initialize_for_runtime_config_fn)GetExport(lib, "hostfxr_initialize_for_runtime_config");
        m_HostFXRFunctions.GetRuntimeDelegateFPtr = (hostfxr_get_runtime_delegate_fn)GetExport(lib, "hostfxr_get_runtime_delegate");
        m_HostFXRFunctions.CloseFPtr = (hostfxr_close_fn)GetExport(lib, "hostfxr_close");
        m_HostFXRFunctions.SetErrorWriterFPtr = (hostfxr_set_error_writer_fn)GetExport(lib, "hostfxr_set_error_writer");

        return (m_HostFXRFunctions.InitForRuntimeConfigFPtr && m_HostFXRFunctions.GetRuntimeDelegateFPtr && m_HostFXRFunctions.CloseFPtr && m_HostFXRFunctions.SetErrorWriterFPtr);
    }

    void NativeHost::Init()
    {
        FBY_ASSERT(LoadHostFXR(), "Failed to load Host FXR!");

        m_HostFXRFunctions.SetErrorWriterFPtr([](const char_t* InMessage) {
            FBY_ERROR(InMessage);
            });

        // Load .NET Core
        int rc = m_HostFXRFunctions.InitForRuntimeConfigFPtr(m_RuntimeConfigPath, nullptr, &m_AssemblyContext.ContextHandle);
        if (rc != 0 || m_AssemblyContext.ContextHandle == nullptr)
        {
            FBY_ERROR("HostFXR - InitForRuntimeConfig failed: {}", rc);
            m_HostFXRFunctions.CloseFPtr(m_AssemblyContext.ContextHandle);
            return;
        }

        // Get the load assembly function pointer
        void* loadAssemblyAndGetFunctionPtr = nullptr;
        rc = m_HostFXRFunctions.GetRuntimeDelegateFPtr(m_AssemblyContext.ContextHandle, hdt_load_assembly_and_get_function_pointer, &loadAssemblyAndGetFunctionPtr);

        if (rc != 0 || loadAssemblyAndGetFunctionPtr == nullptr)
            FBY_ERROR("Get delegate failed: {}", rc);

        m_AssemblyContext.LoadAssemblyAndGetFunction = (load_assembly_and_get_function_pointer_fn)loadAssemblyAndGetFunctionPtr;

        g_CoreManagedFunctions.SetInternalCalls = LoadManagedFunction<CoreManagedFunctions::SetInternalCallsFn>("Flameberry.Managed.InternalCallManager, Flameberry-ScriptCore", "SetInternalCalls");
    }

    void NativeHost::AddInternalCall(const char_t* typeName, const char_t* methodName, void* funcPtr)
    {
        std::string assemblyQualifiedName(typeName);
        assemblyQualifiedName += "+";
        assemblyQualifiedName += methodName;
        assemblyQualifiedName += ", Flameberry-ScriptCore";

        const auto& name = m_InternalCallNameArray.emplace_back(assemblyQualifiedName);

        InternalCall call;
        call.AssemblyQualifiedName = name.c_str();
        call.NativeFunctionPtr = funcPtr;
        m_InternalCallArray.emplace_back(call);
    }

    void NativeHost::UploadInternalCalls()
    {
        g_CoreManagedFunctions.SetInternalCalls(m_InternalCallArray.data(), static_cast<int32_t>(m_InternalCallArray.size()));
        m_InternalCallArray.clear();
        m_InternalCallNameArray.clear();
    }

    void NativeHost::Shutdown()
    {
        m_HostFXRFunctions.CloseFPtr(m_AssemblyContext.ContextHandle);
    }

}
