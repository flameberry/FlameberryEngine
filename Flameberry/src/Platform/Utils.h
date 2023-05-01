#pragma once

#include <string>

namespace Flameberry {
    namespace platform {
        void OpenInFinder(const char* path);
        std::string OpenDialog();
        std::string SaveDialog();
    }
}
