#include "Physics.h"

#include "InterfaceImpls.h"

#include "Core/Core.h"

namespace Flameberry {

	struct PhysicsManagerData
	{
		// This is the max amount of rigid bodies that you can add to the physics system. If you try to add more you'll get an error.
		// Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
		const JPH::uint MaxBodies = 1024;

		// This determines how many mutexes to allocate to protect rigid bodies from concurrent access. Set it to 0 for the default settings.
		const JPH::uint NumBodyMutexes = 0;

		// This is the max amount of body pairs that can be queued at any time (the broad phase will detect overlapping
		// body pairs based on their bounding boxes and will insert them into a queue for the narrowphase). If you make this buffer
		// too small the queue will fill up and the broad phase jobs will start to do narrow phase work. This is slightly less efficient.
		// Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
		const JPH::uint MaxBodyPairs = 1024;

		// This is the maximum size of the contact constraint buffer. If more contacts (collisions between bodies) are detected than this
		// number then these contacts will be ignored and bodies will start interpenetrating / fall through the world.
		// Note: This value is low because this is a simple test. For a real project use something in the order of 10240.
		const JPH::uint MaxContactConstraints = 1024;

		// We need a temp allocator for temporary allocations during the physics update. We're
		// pre-allocating 10 MB to avoid having to do allocations during the physics update.
		// B.t.w. 10 MB is way too much for this example but it is a typical value you can use.
		// If you don't want to pre-allocate you can also use TempAllocatorMalloc to fall back to
		// malloc / free.
		JPH::TempAllocatorImpl TempAllocator;

		// We need a job system that will execute physics jobs on multiple threads. Typically
		// you would implement the JobSystem interface yourself and let Jolt Physics run on top
		// of your own job scheduler. JobSystemThreadPool is an example implementation.
		JPH::JobSystemThreadPool JobSystemThreadPool;

		// Create mapping table from object layer to broadphase layer
		// Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
		BPLayerInterfaceImpl BroadPhaseLayerInterface;

		// Create class that filters object vs broadphase layers
		// Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
		ObjectVsBroadPhaseLayerFilterImpl ObjectVsBroadPhaseLayerFilter;

		// Create class that filters object vs object layers
		// Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
		ObjectLayerPairFilterImpl ObjectVsObjectLayerPairFilter;

		JPH::PhysicsSystem PhysicsSystem;

		PhysicsManagerData()
			: TempAllocator(10 * 1024 * 1024)
			, JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1)
		{
		}
	};

	PhysicsManagerData* s_Data;

	// Callback for traces, connect this to your own trace function if you have one
	static void TraceImpl(const char* inFMT, ...)
	{
		// Format the message
		va_list list;
		va_start(list, inFMT);
		char buffer[1024];
		vsnprintf(buffer, sizeof(buffer), inFMT, list);
		va_end(list);

		FBY_TRACE("JoltPhysics: {}", buffer);
	}

	// Callback for asserts, connect this to your own assert handler if you have one
	static bool AssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, JPH::uint inLine)
	{
		FBY_ASSERT(0, "{}:{}: ({}) {}", inFile, inLine, inExpression, (inMessage != nullptr ? inMessage : ""));
		return true;
	};

	void PhysicsManager::Init()
	{
		// Register allocation hook. In this example we'll just let Jolt use malloc / free but you can override these if you want (see Memory.h).
		// This needs to be done before any other Jolt function is called.
		JPH::RegisterDefaultAllocator();

		// Install trace and assert callbacks
		JPH::Trace = TraceImpl;
		JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = AssertFailedImpl;)

		// Create a factory, this class is responsible for creating instances of classes based on their name or hash and is mainly used for deserialization of saved data.
		// It is not directly used in this example but still required.
		JPH::Factory::sInstance = new JPH::Factory();

		// Register all physics types with the factory and install their collision handlers with the CollisionDispatch class.
		// If you have your own custom shape types you probably need to register their handlers with the CollisionDispatch before calling this function.
		// If you implement your own default material (PhysicsMaterial::sDefault) make sure to initialize it before this function or else this function will create one for you.
		JPH::RegisterTypes();

		// Note: It is very important to create physics manager data here because...
		// it constructs the TempAllocatorImpl class which needs the JPH default allocator to be registered
		s_Data = new PhysicsManagerData();

		// Now we can init the actual physics system.
		s_Data->PhysicsSystem.Init(s_Data->MaxBodies, s_Data->NumBodyMutexes, s_Data->MaxBodyPairs, s_Data->MaxContactConstraints, s_Data->BroadPhaseLayerInterface, s_Data->ObjectVsBroadPhaseLayerFilter, s_Data->ObjectVsObjectLayerPairFilter);

		// A body activation listener gets notified when bodies activate and go to sleep
		// Note that this is called from a job so whatever you do here needs to be thread safe.
		// Registering one is entirely optional.
		// MyBodyActivationListener body_activation_listener;
		// physics_system.SetBodyActivationListener(&body_activation_listener);

		// A contact listener gets notified when bodies (are about to) collide, and when they separate again.
		// Note that this is called from a job so whatever you do here needs to be thread safe.
		// Registering one is entirely optional.
		// MyContactListener contact_listener;
		// physics_system.SetContactListener(&contact_listener);
	}

	void PhysicsManager::Shutdown()
	{
		// Unregisters all types with the factory and cleans up the default material
		JPH::UnregisterTypes();

		// Destroy the factory
		delete JPH::Factory::sInstance;
		JPH::Factory::sInstance = nullptr;

		delete s_Data;
	}

	JPH::BodyInterface& PhysicsManager::GetBodyInterface()
	{
		// The main way to interact with the bodies in the physics system is through the body interface. There is a locking and a non-locking
		// variant of this. We're going to use the locking version (even though we're not planning to access bodies from multiple threads)
		return s_Data->PhysicsSystem.GetBodyInterface();
	}

	void PhysicsManager::OptimizeBroadPhase()
	{
		s_Data->PhysicsSystem.OptimizeBroadPhase();
	}

	void PhysicsManager::Update(float delta, int collisionSteps)
	{
		s_Data->PhysicsSystem.Update(delta, collisionSteps, &s_Data->TempAllocator, &s_Data->JobSystemThreadPool);
	}

} // namespace Flameberry