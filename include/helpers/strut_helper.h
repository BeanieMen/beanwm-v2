#pragma once

#include <X11/Xlib.h>
#include "types.h"

struct ScreenStruts
{
    int left = 0;
    int right = 0;
    int top = 0;
    int bottom = 0;
};

bool isDockWindow(Display *display, Window window);
ScreenStruts getScreenStruts(Display *display, Window root, const ScreenInfo &screen);
