#pragma once

#include <X11/Xlib.h>
#include <vector>

enum ManagementMode
{
    MODE_TILED = 0,
    MODE_FLOATING = 1
};

struct ScreenInfo
{
    int screenIndex;   // X11 screen index (0, 1, 2, ...)
    int x;             // X position of screen on virtual desktop
    int y;             // Y position of screen on virtual desktop
    int width;         // Screen width
    int height;        // Screen height
};

struct Client
{
    Window window;
    int screenIndex;   // Which screen this client belongs to
    int workspace;     // Workspace number (relative to this screen)
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
    int screenIndex;
    int workspace;
    int x;
    int y;
    int width;
    int height;
};