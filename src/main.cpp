#include <stdio.h>
#include <X11/Xlib.h>
#include <stdlib.h>
#include <X11/keysym.h>
#include <unistd.h>
#include <sys/types.h>

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

void tile(Display *display, Client *clients)
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

int main(void)
{
    Display *display = XOpenDisplay(NULL);

    if (display == NULL)
    {
        fprintf(stderr, "Cannot open display\n");
        return 1;
    }

    printf("Connected to display: %s\n", DisplayString(display));
    Window root = DefaultRootWindow(display);
    XEvent event;
    Client *clients = NULL;

    // alt + enter mask
    XGrabKey(
        display,
        XKeysymToKeycode(display, XK_Return),
        Mod1Mask,
        root,
        True,
        GrabModeAsync,
        GrabModeAsync);

    XSelectInput(display, root, EnterWindowMask | SubstructureRedirectMask | SubstructureNotifyMask | KeyPressMask);
    XSync(display, False);
    while (1)
    {
        XNextEvent(display, &event);

        switch (event.type)
        {
        case MapRequest:
        {
            printf("Window wants to be mapped: %lu\n",
                   event.xmaprequest.window);
            Window window = event.xmaprequest.window;
            Client *client = (Client *)malloc(sizeof(Client));

            if (client == NULL)
            {
                fprintf(stderr, "Failed to allocate memory for client\n");
                continue;
            }

            client->window = window;
            client->next = clients;
            clients = client;

            XSelectInput(display, window, EnterWindowMask);

            XMoveResizeWindow(display, window, 100, 100, 800, 600);
            XMapWindow(display, window);
            XSetInputFocus(display, window, RevertToPointerRoot, CurrentTime);
            tile(display, clients);
            break;
        }
        case CreateNotify:
        {
            printf(
                "Window created: %lu, parent: %lu\n",
                event.xcreatewindow.window,
                event.xcreatewindow.parent);
            for (Client *c = clients; c != NULL; c = c->next)
            {
                printf("Client window: %lu\n", c->window);
            }
            break;
        }

        case DestroyNotify:
        {
            printf("Window destroyed: %lu\n",
                   event.xdestroywindow.window);
            Client **current = &clients;
            while (*current != NULL)
            {
                if ((*current)->window == event.xdestroywindow.window)
                {
                    Client *to_delete = *current;
                    *current = (*current)->next;
                    free(to_delete);
                    tile(display, clients);
                    break;
                }
                current = &((*current)->next);
            }
            break;
        }

        case KeyPress:
        {
            if (event.xkey.keycode == XKeysymToKeycode(display, XK_Return) &&
                (event.xkey.state & Mod1Mask))
            {
                printf("Spawning Alacritty on %s\n", DisplayString(display));

                if (fork() == 0)
                {
                    execlp("alacritty", "alacritty", (char *)NULL);

                    perror("execlp");
                    _exit(1);
                }
            }

            break;
        }
        case EnterNotify:
        {
            Window window = event.xcrossing.window;
            XSetInputFocus(display, window, RevertToPointerRoot, CurrentTime);
            break;
        };
        };
    };
};