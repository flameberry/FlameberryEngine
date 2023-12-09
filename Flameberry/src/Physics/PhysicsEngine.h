#pragma once

#include "extensions/PxExtensionsAPI.h"

namespace Flameberry {

    class PhysicsEngine
    {
    public:
        static void Init(int numThreads = 0);
        static void Shutdown();

        static physx::PxPhysics* GetPhysics() { return s_Physics; }
        static physx::PxTolerancesScale GetTolerancesScale() { return s_TolerancesScale; }
        static physx::PxCpuDispatcher* GetCPUDispatcher() { return s_CPUDispatcher; }
    private:
        static physx::PxDefaultErrorCallback s_DefaultErrorCallback;
        static physx::PxDefaultAllocator s_DefaultAllocatorCallback;
        static physx::PxFoundation* s_Foundation;
        static physx::PxPhysics* s_Physics;
        static physx::PxCooking* s_Cooking;
        static physx::PxCpuDispatcher* s_CPUDispatcher;

        static physx::PxTolerancesScale s_TolerancesScale;
    };

}
