#include <stdio.h>
#include <stdlib.h>
#include <X11/keysym.h>
#include <unistd.h>
#include <sys/types.h>
#include <X11/Xlib.h>
#include <types.h>
#include "window_manager.h"

WindowManager::WindowManager() : display(nullptr), root(0), clients(nullptr)
{
    display = XOpenDisplay(NULL);

    if (display == NULL)
    {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }

    root = DefaultRootWindow(display);

    printf("Connected to display: %s\n", DisplayString(display));

    setup();
}

WindowManager::~WindowManager()
{
    // Free all clients
    Client *current = clients;
    while (current != NULL)
    {
        Client *to_delete = current;
        current = current->next;
        free(to_delete);
    }

    if (display != NULL)
    {
        XCloseDisplay(display);
    }
}

void WindowManager::run()
{
    while (true)
    {
        XNextEvent(display, &event);
        handleEvent();
    }
}

void WindowManager::setup()
{
    XGrabKey(display, XKeysymToKeycode(display, XK_Return), Mod4Mask, root, True, GrabModeAsync, GrabModeAsync);
    XSelectInput(display, root, EnterWindowMask | SubstructureRedirectMask | SubstructureNotifyMask | KeyPressMask);
    XSync(display, False);
}

void WindowManager::handleEvent()
{
    switch (event.type)
    {
    case MapRequest:
        handleMapRequest();
        break;
    case CreateNotify:
        handleCreateNotify();
        break;
    case DestroyNotify:
        handleDestroyNotify();
        break;
    case KeyPress:
        handleKeyPress();
        break;
    case EnterNotify:
        handleEnterNotify();
        break;
    default:
        break;
    }
}

void WindowManager::handleMapRequest()
{
    Window window = event.xmaprequest.window;
    printf("Window wants to be mapped: %lu\n", window);

    addClient(window);
    XSelectInput(display, window, EnterWindowMask);
    XMapWindow(display, window);
    XSetInputFocus(display, window, RevertToPointerRoot, CurrentTime);
    tile();
}

void WindowManager::handleCreateNotify()
{
    printf("Window created: %lu, parent: %lu\n", event.xcreatewindow.window, event.xcreatewindow.parent);
    for (Client *c = clients; c != NULL; c = c->next)
    {
        printf("Client window: %lu\n", c->window);
    }
}

void WindowManager::handleDestroyNotify()
{
    Window window = event.xdestroywindow.window;
    printf("Window destroyed: %lu\n", window);
    bool removed = removeClient(window);
    if (removed)
    {
        tile();
    }
}

void WindowManager::handleKeyPress()
{
    if (
        event.xkey.keycode ==
            XKeysymToKeycode(display, XK_Return) &&
        (event.xkey.state & Mod4Mask))
    {
        spawnTerminal();
    }
}

void WindowManager::handleEnterNotify()
{
    Window window = event.xcrossing.window;
    XSetInputFocus(display, window, RevertToPointerRoot, CurrentTime);
}

void WindowManager::spawnTerminal()
{
    if (fork() == 0)
    {
        execlp("alacritty", "alacritty", NULL);
        fprintf(stderr, "Failed to execute alacritty\n");
        exit(1);
    }
}

void WindowManager::addClient(Window window)
{
    Client *new_client = (Client *)malloc(sizeof(Client));
    new_client->window = window;
    new_client->next = clients;
    clients = new_client;
}

bool WindowManager::removeClient(Window window)
{
    Client **current = &clients;

    while (*current != NULL)
    {
        printf("Checking client: %lu\n", (*current)->window);

        if ((*current)->window == window)
        {
            Client *to_delete = *current;
            *current = (*current)->next;

            printf("Removing client: %lu\n", window);

            free(to_delete);
            return true;
        }

        current = &((*current)->next);
    }
    return false;
}

void WindowManager::tile()
{
    dwindleTile(display, clients);
}