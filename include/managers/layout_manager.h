#pragma once

#include "client_manager.h"
#include "config.h"
#include "helpers/strut_helper.h"
#include <X11/Xlib.h>

class LayoutManager
{
private:
    int gap = GAP;

public:
    LayoutManager() = default;
    ~LayoutManager() = default;

    int getGap() const { return gap; }
    void setGap(int g) { gap = g; }

    void tile(Display *display, Window root, ClientManager &clientManager);
};
