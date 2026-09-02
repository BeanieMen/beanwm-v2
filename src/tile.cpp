#include "types.h"
#include <algorithm>
#include <cstdio>
#include <vector>
#include <X11/Xlib.h>

void dwindleTile(Display *display, const std::vector<Client> &clients, int gap, int workspace)
{
    int screen_width = DisplayWidth(display, 0);
    int screen_height = DisplayHeight(display, 0);


    std::vector<Client*> workspace_clients;
    workspace_clients.reserve(clients.size());
    for (const auto &c : clients) {
        if (c.workspace == workspace) {
            workspace_clients.push_back(const_cast<Client*>(&c));
        }
    }

    printf("Tiling %lu clients on workspace %d\n", workspace_clients.size(), workspace);
    if (workspace_clients.empty()) {
        return;
    }


    for (const auto &c : clients) {
        if (c.workspace == workspace) {
            XMapWindow(display, c.window);
        } else {
            XUnmapWindow(display, c.window);
        }
    }

    std::vector<Area> areas;
    areas.reserve(workspace_clients.size());


    areas.push_back(Area{
        workspace_clients[0],
        workspace,
        gap,
        gap,
        screen_width - 2 * gap,
        screen_height - 2 * gap
    });
    if (areas.back().width < 1) areas.back().width = 1;
    if (areas.back().height < 1) areas.back().height = 1;


    
    for (size_t i = 1; i < workspace_clients.size(); ++i) {
        Area old = areas.back();

        if (areas.size() % 2 == 1) {
            // Split vertically - uniform gap between
            int new_width = (old.width - gap) / 2;
            if (new_width < 1) new_width = old.width / 2;
            if (new_width < 1) new_width = 1;
            int other_width = old.width - new_width - gap;
            if (other_width < 1) {
                other_width = 1;
                new_width = old.width - gap - other_width;
                if (new_width < 1) new_width = 1;
            }
            areas.back().width = new_width;
            Area next{};
            next.width = other_width;
            next.workspace = workspace;
            next.client = workspace_clients[i];
            next.x = old.x + new_width + gap;
            next.y = old.y;
            next.height = old.height;
            areas.push_back(next);
        } else {
            // Split horizontally - uniform gap between
            int new_height = (old.height - gap) / 2;
            if (new_height < 1) new_height = old.height / 2;
            if (new_height < 1) new_height = 1;
            int other_height = old.height - new_height - gap;
            if (other_height < 1) {
                other_height = 1;
                new_height = old.height - gap - other_height;
                if (new_height < 1) new_height = 1;
            }
            areas.back().height = new_height;
            Area next{};
            next.height = other_height;
            next.workspace = workspace;
            next.client = workspace_clients[i];
            next.x = old.x;
            next.y = old.y + new_height + gap;
            next.width = old.width;
            areas.push_back(next);
        }
    }

    for (const auto &area : areas) {
        int w = area.width < 1 ? 1 : area.width;
        int h = area.height < 1 ? 1 : area.height;
        XMoveResizeWindow(
            display,
            area.client->window,
            area.x,
            area.y,
            w,
            h);
    }

    XSync(display, False);
}
