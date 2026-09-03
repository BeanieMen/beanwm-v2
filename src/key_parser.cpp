#include "window_manager.h"

#include <X11/keysym.h>
#include <cctype>
#include <sstream>

unsigned int WindowManager::parseModString(
    const std::string &str)
{
    unsigned int mask = 0;

    std::stringstream ss(str);
    std::string token;

    while (std::getline(ss, token, '+'))
    {
        token.erase(
            0,
            token.find_first_not_of(" \t")
        );

        token.erase(
            token.find_last_not_of(" \t") + 1
        );

        if (token == "Mod4" || token == "Super")
            mask |= Mod4Mask;

        else if (token == "Mod1" || token == "Alt")
            mask |= Mod1Mask;

        else if (token == "Shift")
            mask |= ShiftMask;

        else if (token == "Control" || token == "Ctrl")
            mask |= ControlMask;
    }

    return mask;
};