#include "layout.h"
#include <X11/Xlib.h>
#include <vector>

// dwindleTile: geometry stays per-screen (screenIndex/screenInfo/struts),
// but `workspace` is the GLOBAL workspace number shared across all screens.
// The same ws is passed for every screen, so filtering on
// (screenIndex == screen && workspace == globalWs) shows the same
// workspace on all screens simultaneously. Plain loops only.
void dwindleTile(Display* display, std::vector<Client>& clients, int gap, int screenIndex, int workspace, const ScreenInfo &screenInfo, const ScreenStruts &struts) {
    int start_x = screenInfo.x + struts.left + gap;
    int start_y = screenInfo.y + struts.top + gap;
    int sw = screenInfo.width - struts.left - struts.right - 2 * gap;
    int sh = screenInfo.height - struts.top - struts.bottom - 2 * gap;
    if (sw < 1) sw = 1;
    if (sh < 1) sh = 1;

    std::vector<Client*> workspace_clients;
    workspace_clients.reserve(clients.size());
    for (auto &client : clients)
        if (client.screenIndex == screenIndex && client.workspace == workspace)
            workspace_clients.push_back(&client);

    for (auto &c : clients) {
        if (c.screenIndex == screenIndex) {
            if (c.workspace == workspace) XMapWindow(display, c.window);
            else XUnmapWindow(display, c.window);
        }
    }

    if (workspace_clients.empty()) return;

    std::vector<Area> areas;
    areas.reserve(workspace_clients.size());
    areas.push_back({workspace_clients[0], screenIndex, workspace, start_x, start_y, sw, sh});
    if (areas.back().width < 1) areas.back().width = 1;
    if (areas.back().height < 1) areas.back().height = 1;

    for (size_t i = 1; i < workspace_clients.size(); ++i) {
        Area old = areas.back();
        if (areas.size() % 2 == 1) {
            int nw = (old.width - gap) / 2;
            if (nw < 1) nw = old.width / 2;
            int ow = old.width - nw - gap;
            if (ow < 1) { ow = 1; nw = old.width - gap - ow; }
            areas.back().width = nw;
            areas.push_back({workspace_clients[i], screenIndex, workspace, old.x + nw + gap, old.y, ow, old.height});
        } else {
            int nh = (old.height - gap) / 2;
            if (nh < 1) nh = old.height / 2;
            int oh = old.height - nh - gap;
            if (oh < 1) { oh = 1; nh = old.height - gap - oh; }
            areas.back().height = nh;
            areas.push_back({workspace_clients[i], screenIndex, workspace, old.x, old.y + nh + gap, old.width, oh});
        }
    }

    for (auto &a : areas) {
        if (a.width < 1) a.width = 1;
        if (a.height < 1) a.height = 1;
        a.client->x = a.x;
        a.client->y = a.y;
        a.client->width = a.width;
        a.client->height = a.height;
        XMoveResizeWindow(display, a.client->window, a.x, a.y, a.width, a.height);
    }
}
