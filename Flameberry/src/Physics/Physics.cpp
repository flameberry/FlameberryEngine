#include "Physics.h"

#include "Core/Core.h"
#include "PxPhysicsAPI.h"
#include "cooking/PxCooking.h"

namespace Flameberry {

    physx::PxDefaultErrorCallback PhysicsContext::s_DefaultErrorCallback;
    physx::PxDefaultAllocator PhysicsContext::s_DefaultAllocatorCallback;
    physx::PxFoundation* PhysicsContext::s_Foundation;
    physx::PxPhysics* PhysicsContext::s_Physics;
    physx::PxCooking* PhysicsContext::s_Cooking;
    physx::PxCpuDispatcher* PhysicsContext::s_CPUDispatcher;

    physx::PxTolerancesScale PhysicsContext::s_TolerancesScale;

    void PhysicsContext::Init()
    {
        s_Foundation = PxCreateFoundation(PX_PHYSICS_VERSION, s_DefaultAllocatorCallback, s_DefaultErrorCallback);
        FL_ASSERT(s_Foundation, "Failed to create PxFoundation object!");

        bool recordMemoryAllocations = true;

        s_Physics = PxCreatePhysics(PX_PHYSICS_VERSION, *s_Foundation, s_TolerancesScale, recordMemoryAllocations, nullptr);
        FL_ASSERT(s_Physics, "Failed to create PxPhysics object!");

        s_Cooking = PxCreateCooking(PX_PHYSICS_VERSION, *s_Foundation, physx::PxCookingParams(s_TolerancesScale));
        FL_ASSERT(s_Cooking, "Failed to create PxCooking object!");

        s_CPUDispatcher = physx::PxDefaultCpuDispatcherCreate(2);
        FL_ASSERT(s_CPUDispatcher, "Failed to create PxCpuDispatcher object!");
    }

    void PhysicsContext::Release()
    {
        s_Cooking->release();
        s_Physics->release();
        s_Foundation->release();
    }

}
