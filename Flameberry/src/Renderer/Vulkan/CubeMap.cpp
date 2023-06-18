#include "CubeMap.h"

#include <stb_image/stb_image.h>
#include "Core/Core.h"

namespace Flameberry {

    CubeMap::CubeMap()
    {
        stbi_set_flip_vertically_on_load(true);
        int width, height, nrComponents;
        float* data = stbi_loadf("newport_loft.hdr", &width, &height, &nrComponents, 0);

        FL_ASSERT(data, "Failed to load cubemap!");

        // unsigned int hdrTexture;
        // glGenTextures(1, &hdrTexture);
        // glBindTexture(GL_TEXTURE_2D, hdrTexture);
        // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);

        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }

}