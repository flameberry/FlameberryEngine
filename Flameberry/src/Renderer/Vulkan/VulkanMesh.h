#pragma once

#include "VulkanVertex.h"

namespace Flameberry {
    class VulkanMesh
    {
    public:
        VulkanMesh(const std::vector<VulkanVertex>& vertices, const std::vector<uint32_t>& indices);
        ~VulkanMesh();
    private:
    };
}