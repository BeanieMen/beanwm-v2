#pragma once

#include "types.h"

#include <X11/Xlib.h>


class WindowManager
{
private:
    Display *display;
    Window root;
    XEvent event;
    Client *clients;

public:
    // Opens the X11 display and initializes the window manager.
    WindowManager();

    // Frees clients and closes the X11 display.
    ~WindowManager();

    // Starts the main X11 event loop.
    void run();

private:
    // Configures X11 event masks and keyboard shortcuts.
    void setup();

    // Receives the current X11 event and dispatches it to a handler.
    void handleEvent();

    // Handles a window asking the WM to be mapped.
    void handleMapRequest();

    // Handles notification that a new X11 window was created.
    void handleCreateNotify();

    // Handles notification that a managed window was destroyed.
    void handleDestroyNotify();

    // Handles keyboard input and WM keybindings.
    void handleKeyPress();

    // Focuses the window when the mouse enters it.
    void handleEnterNotify();

    // Starts Alacritty as a child process.
    void spawnTerminal();

    // Adds a window to the list of managed clients.
    void addClient(Window window);

    // Removes a window from the list of managed clients.
    bool removeClient(Window window);

    // Calculates and applies the current tiling layout.
    void tile();
};
