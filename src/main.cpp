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
    };

    int height = screen_height / count;
    int y = 0;

    for (Client *c = clients; c != NULL; c = c->next)
    {
        XMoveResizeWindow(display, c->window, 0, y, screen_width, height);
        y += height;
    }
};

int main(void)
{
    Display *display = XOpenDisplay(NULL);

    if (display == NULL)
    {
        fprintf(stderr, "Cannot open display\n");
        return 1;
    }

    printf("Connected to display: %s\n", DisplayString(display));
    const char *display_name = DisplayString(display);
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

    XSelectInput(display, root, SubstructureRedirectMask | SubstructureNotifyMask | KeyPressMask);
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

            XMoveResizeWindow(display, window, 100, 100, 800, 600);
            XMapWindow(display, window);
            XSetInputFocus(display, window, RevertToPointerRoot, CurrentTime);
            tile(display, clients);
            break;
        }
        case CreateNotify:
        {
            printf("Window created: %lu\n",
                   event.xcreatewindow.window);
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
                    break;
                }
                current = &((*current)->next);
            }
            tile(display, clients);
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
                    // 1. Force X11 display target
                    setenv("DISPLAY", DisplayString(display), 1);

                    // 2. Clear Wayland display variable so apps don't bypass Xephyr
                    unsetenv("WAYLAND_DISPLAY");

                    // 3. Force backend drivers to X11
                    setenv("WINIT_UNIX_BACKEND", "x11", 1); // For Rust/Winit (Alacritty)
                    setenv("GDK_BACKEND", "x11", 1);        // For GTK apps
                    setenv("QT_QPA_PLATFORM", "x11", 1);    // For Qt apps

                    execlp("alacritty", "alacritty", NULL);

                    perror("execlp");
                    _exit(1);
                };
            };
            break;
        };
        };
    };
};