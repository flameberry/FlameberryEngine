#pragma once

#include <stdint.h>

#define MAX_ENTITIES 1000

namespace Flameberry {
    class entity_handle
    {
    public:
        using value_type = uint32_t;
    public:
        entity_handle() = default;
        entity_handle(value_type entityId): handle(entityId), validity(true) {}
        entity_handle(value_type entityId, bool validity): handle(entityId), validity(validity) {}

        inline bool is_valid() const { return validity; }
        inline void set_validity(bool val) { validity = val; }

        operator bool() const { return validity; }

        // Returns the entity handle i.e. a unique `value_type` value
        inline value_type get() const { return handle; }
        inline void set_handle(value_type entity_handle) { handle = entity_handle; }

        bool operator==(const entity_handle& entity) {
            return handle == entity.get() && validity;
        }

        bool operator!=(const entity_handle& entity) {
            return !(*this == entity);
        }
    private:
        value_type handle;
        bool validity;
    };
}