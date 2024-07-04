#include "UUID.h"

#include <random>

namespace Flameberry {
	static std::random_device s_RandomDevice;
	static std::mt19937_64 s_Engine(s_RandomDevice());
	static std::uniform_int_distribution<UUID::value_type> s_UniformDistribution;

	UUID::UUID()
		: m_UUID(s_UniformDistribution(s_Engine)) {}

	UUID::UUID(value_type uuid)
		: m_UUID(uuid) {}

	UUID::~UUID() {}
} // namespace Flameberry
