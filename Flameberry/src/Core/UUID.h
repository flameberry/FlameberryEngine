#pragma once

#include <iostream>

namespace Flameberry {
    class UUID
    {
    public:
        UUID();
        UUID(uint64_t uuid);
        ~UUID();

        operator uint64_t() const { return m_UUID; }
    private:
        uint64_t m_UUID;
    };
}