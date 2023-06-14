#include "ecs.hpp"

namespace fbentt {
    null_t null;

    entity_handle::entity_handle() {
        *this = null;
    }
}