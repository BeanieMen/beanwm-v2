#pragma once

#include <X11/Xlib.h>
#include <vector>

#include "types.h"

void dwindleTile(
    Display *display,
    const std::vector<Client> &clients,
    int gap,
    int workspace
);
