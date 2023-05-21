#pragma once

#include <iostream>
#include <unordered_map>

namespace Flameberry {
    class UUID
    {
    public:
        explicit UUID();
        UUID(uint64_t uuid);
        ~UUID();

        operator uint64_t() const { return m_UUID; }
    private:
        uint64_t m_UUID;
    };
}

namespace std {
    template<>
    struct hash<Flameberry::UUID>
    {
        size_t operator()(const Flameberry::UUID& key) const
        {
            return hash<uint64_t>()((uint64_t)key);
        }
    };
}