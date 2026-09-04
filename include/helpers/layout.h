#pragma once

#include <X11/Xlib.h>
#include <vector>
#include "types.h"
#include "helpers/strut_helper.h"

void dwindleTile(Display *display, std::vector<Client> &clients, int gap, int screenIndex, int workspace, const ScreenInfo &screen, const ScreenStruts &struts = {});
