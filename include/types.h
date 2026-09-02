#pragma once

#include <X11/Xlib.h>

typedef struct Client Client;

struct Client
{
    Window window;
    Client *next;
};

typedef struct Area Area;

struct Area
{
    Client *client;
    int x;
    int y;
    int width;
    int height;
};

void dwindleTile(Display *display, Client *clients);
