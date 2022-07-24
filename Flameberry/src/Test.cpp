#include "Test.h"

namespace Flameberry {
    void PrintWelcome()
    {
#ifdef FL_DEBUG
        std::cout << "Welcome to the Flameberry Engine!- Debug" << std::endl;
#else
        std::cout << "Welcome to the Flameberry Engine!- Release" << std::endl;
#endif
    }
}