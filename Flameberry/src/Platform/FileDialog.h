#pragma once

#include <string>

namespace Flameberry {
    class FileDialog
    {
    public:
        static std::string OpenDialog();
        static std::string SaveDialog();
    };
}
