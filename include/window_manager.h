#pragma once

#include "types.h"

#include <X11/Xlib.h>
#include <X11/keysym.h>

#include <algorithm>
#include <functional>
#include <vector>

class WindowManager
{
private:
    static constexpr int WORKSPACE_COUNT = 9;

    using KeyAction = std::function<void()>;

    struct KeyBinding
    {
        unsigned int modifiers;
        KeySym key;
        KeyAction action;
    };

    Display *display;
    Window root;
    XEvent event;

    std::vector<Client> clients;

    int current_workspace;
    unsigned int MOD_MASK = Mod4Mask;
    std::vector<KeyBinding> keybindings;

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

    // Configures keyboard shortcuts.
    void setupKeybindings();

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

    // Switches to another workspace.
    void switchWorkspace(int workspace);

    // Moves the focused window to another workspace.
    void moveToWorkspace(int workspace);

    // Shows all windows belonging to a workspace.
    void showWorkspace(int workspace);

    // Hides all windows belonging to a workspace.
    void hideWorkspace(int workspace);

    // Finds a client by its window.
    Client *findClient(Window window);

    // Handles asynchronous X11 errors.
    static int handleXError(
        Display *display,
        XErrorEvent *error_event
    );
};