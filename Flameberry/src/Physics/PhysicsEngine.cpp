#include "PhysicsEngine.h"

#include "Core/Core.h"
#include "PxPhysicsAPI.h"
#include "cooking/PxCooking.h"

namespace Flameberry {

    physx::PxDefaultErrorCallback PhysicsEngine::s_DefaultErrorCallback;
    physx::PxDefaultAllocator PhysicsEngine::s_DefaultAllocatorCallback;
    physx::PxFoundation* PhysicsEngine::s_Foundation;
    physx::PxPhysics* PhysicsEngine::s_Physics;
    physx::PxCooking* PhysicsEngine::s_Cooking;
    physx::PxCpuDispatcher* PhysicsEngine::s_CPUDispatcher;

    physx::PxTolerancesScale PhysicsEngine::s_TolerancesScale;

    void PhysicsEngine::Init()
    {
        s_Foundation = PxCreateFoundation(PX_PHYSICS_VERSION, s_DefaultAllocatorCallback, s_DefaultErrorCallback);
        FBY_ASSERT(s_Foundation, "Failed to create PxFoundation object!");

        bool recordMemoryAllocations = true;

        s_Physics = PxCreatePhysics(PX_PHYSICS_VERSION, *s_Foundation, s_TolerancesScale, recordMemoryAllocations, nullptr);
        FBY_ASSERT(s_Physics, "Failed to create PxPhysics object!");

        s_Cooking = PxCreateCooking(PX_PHYSICS_VERSION, *s_Foundation, physx::PxCookingParams(s_TolerancesScale));
        FBY_ASSERT(s_Cooking, "Failed to create PxCooking object!");

        s_CPUDispatcher = physx::PxDefaultCpuDispatcherCreate(2);
        FBY_ASSERT(s_CPUDispatcher, "Failed to create PxCpuDispatcher object!");
    }

    void PhysicsEngine::Shutdown()
    {
        s_Cooking->release();
        s_Physics->release();
        s_Foundation->release();
    }

}
