#pragma once

#include "Component.h"

#define MAX_ENTITIES 1000

namespace Flameberry {
    struct Entity {
        uint64_t entityId;
        Entity(uint64_t entityId) : entityId(entityId), m_Validity(true) {}
        inline bool GetValidity() const { return m_Validity; }
        inline void SetValidity(bool validity) { m_Validity = validity; }
    private:
        bool m_Validity;
    };
}