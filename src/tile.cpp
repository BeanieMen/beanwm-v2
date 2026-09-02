#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>

void dwindleTile(Display *display, Client *clients)
{
    int screen_width = DisplayWidth(display, 0);
    int screen_height = DisplayHeight(display, 0);

    int count = 0;

    for (Client *c = clients; c != NULL; c = c->next)
    {
        count++;
    }

    if (count == 0)
    {
        return;
    }

    Area *areas = (Area *)malloc(sizeof(Area) * count);

    if (areas == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for areas\n");
        return;
    }

    // Start with the entire screen.
    areas[0].client = clients;
    areas[0].x = 0;
    areas[0].y = 0;
    areas[0].width = screen_width;
    areas[0].height = screen_height;

    int area_count = 1;

    Client *c = clients->next;

    while (c != NULL)
    {
        // Split the most recently created area.
        Area old = areas[area_count - 1];

        int new_index = area_count;

        if (area_count % 2 == 1){
            // Split vertically
            int new_width = old.width / 2;
            areas[area_count - 1].width = new_width;
            areas[new_index].width = new_width;
            areas[new_index].client = c;
            areas[new_index].x = old.x + new_width;
            areas[new_index].y = old.y;
            areas[new_index].height = old.height;

        } else {
            // Split horizontally
            int new_height = old.height / 2;
            areas[area_count - 1].height = new_height;
            areas[new_index].height = new_height;
            areas[new_index].client = c;
            areas[new_index].x = old.x;
            areas[new_index].y = old.y + new_height;
            areas[new_index].width = old.width;
        }
        area_count++;
        c = c->next;
    }

    for (int i = 0; i < area_count; i++)
    {
        XMoveResizeWindow(
            display,
            areas[i].client->window,
            areas[i].x,
            areas[i].y,
            areas[i].width,
            areas[i].height);
    }

    free(areas);

    XSync(display, False);
}
