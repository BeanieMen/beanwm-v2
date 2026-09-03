#include "layout.h"
#include "config.h"
#include <X11/Xlib.h>
#include <vector>

void dwindleTile(Display* display, std::vector<Client>& clients, int gap, int workspace) {
    int screen = DefaultScreen(display);
    int sw = DisplayWidth(display, screen);
    int sh = DisplayHeight(display, screen);

    std::vector<Client*> workspace_clients;
    workspace_clients.reserve(clients.size());
    for (auto &client : clients)
        if (client.workspace == workspace)
            workspace_clients.push_back(&client);
    if (workspace_clients.empty()) return;

    for (auto &c : clients) {
        if (c.workspace == workspace) XMapWindow(display, c.window);
        else XUnmapWindow(display, c.window);
    }

    std::vector<Area> areas;
    areas.reserve(workspace_clients.size());
    areas.push_back({workspace_clients[0], workspace, gap, gap, sw - 2*gap, sh - 2*gap});
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
            areas.push_back({workspace_clients[i], workspace, old.x + nw + gap, old.y, ow, old.height});
        } else {
            int nh = (old.height - gap) / 2;
            if (nh < 1) nh = old.height / 2;
            int oh = old.height - nh - gap;
            if (oh < 1) { oh = 1; nh = old.height - gap - oh; }
            areas.back().height = nh;
            areas.push_back({workspace_clients[i], workspace, old.x, old.y + nh + gap, old.width, oh});
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

