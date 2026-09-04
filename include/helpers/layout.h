#pragma once

#include <X11/Xlib.h>
#include <vector>
#include "types.h"
#include "helpers/strut_helper.h"

// `workspace` is the GLOBAL workspace number shared across all screens
// (one current_workspace in ClientManager). Geometry stays per-screen via
// screenIndex/screen + struts; callers pass the same ws for every screen.
void dwindleTile(Display *display, std::vector<Client> &clients, int gap, int screenIndex, int workspace, const ScreenInfo &screen, const ScreenStruts &struts = {});
