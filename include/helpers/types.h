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
    int x;
    int y;
    int width;
    int height;
    int number;
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