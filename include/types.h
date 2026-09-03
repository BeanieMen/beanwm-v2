#pragma once

#include <X11/Xlib.h>
#include <vector>

enum ManagementMode
{
    MODE_TILED = 0,
    MODE_FLOATING = 1
};

struct Client
{
    Window window;
    int workspace;
    ManagementMode mode;
};

struct Area
{
    Client *client;
    int workspace;
    int x;
    int y;
    int width;
    int height;
};

void dwindleTile(Display *display, const std::vector<Client> &clients, int gap, int workspace);