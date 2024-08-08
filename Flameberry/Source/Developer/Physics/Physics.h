#pragma once

namespace JPH {

	class BodyInterface;

}

namespace Flameberry {

	class PhysicsManager
	{
	public:
		static void Init();
		static void Shutdown();
		static void OptimizeBroadPhase();
		static void Update(float delta, int collisionSteps);

		static JPH::BodyInterface& GetBodyInterface();
	};

} // namespace Flameberry