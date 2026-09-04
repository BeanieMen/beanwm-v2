#pragma once

#include "client_manager.h"
#include "helpers/strut_helper.h"
#include <X11/Xlib.h>
#include <vector>

class LayoutManager
{
public:
    LayoutManager() = default;
    ~LayoutManager() = default;

    void tile(Display *display, Window root, ClientManager &clientManager,
              const std::vector<ScreenInfo> &screens);
};
