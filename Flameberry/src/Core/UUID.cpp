#include "UUID.h"

#include <random>

namespace Flameberry {
    static std::random_device s_RandomDevice;
    static std::mt19937_64 s_Engine(s_RandomDevice());
    static std::uniform_int_distribution<uint64_t> s_UniformDistribution;

    static uint32_t s_CountOfIDsGenerated = 0;

    UUID::UUID()
        : m_UUID(s_UniformDistribution(s_Engine))
    {
        s_CountOfIDsGenerated++;
    }

    UUID::UUID(uint64_t uuid)
        : m_UUID(uuid)
    {}

    UUID::~UUID()
    {}
}
