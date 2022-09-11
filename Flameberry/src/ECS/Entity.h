#pragma once

#include "Component.h"

#define MAX_ENTITIES 1000

namespace Flameberry {
    class entity_handle
    {
    public:
        entity_handle() = default;
        entity_handle(uint64_t entityId) : handle(entityId), validity(true) {}

        inline bool is_valid() const { return validity; }
        inline void set_validity(bool val) { validity = val; }

        // Returns the entity handle i.e. a unique uint64_t value
        inline uint64_t get() const { return handle; }
        inline void set_handle(uint64_t entity_handle) { handle = entity_handle; }
    private:
        uint64_t handle;
        bool validity;
    };
}