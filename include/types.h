#pragma once

#include <X11/Xlib.h>
#include <vector>

typedef struct Client Client;

struct Client
{
    Window window;
    int workspace;
};

typedef struct Area Area;

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
