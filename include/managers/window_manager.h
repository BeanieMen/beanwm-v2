#pragma once

#include "types.h"
#include "config.h"
#include "client_manager.h"
#include "layout_manager.h"
#include "keybinding_manager.h"
#include "process_manager.h"

#include <X11/Xlib.h>
#include <X11/keysym.h>

class WindowManager
{
private:
    Display *display;
    Window root;
    XEvent event;

    ClientManager clientManager;
    LayoutManager layoutManager;
    KeybindingManager keybindingManager;
    ProcessManager processManager;

    Window draggedWindow = None;
    Window dragTarget = None;
    int dragStartX = 0;
    int dragStartY = 0;
    int dragWindowX = 0;
    int dragWindowY = 0;
    bool dragIsFloating = false;

public:
    WindowManager();
    ~WindowManager();
    void run();

    ClientManager &getClientManager() { return clientManager; }
    LayoutManager &getLayoutManager() { return layoutManager; }
    KeybindingManager &getKeybindingManager() { return keybindingManager; }
    ProcessManager &getProcessManager() { return processManager; }

    void tile();
    void switchWorkspace(int workspace);
    void moveToWorkspace(int workspace);

    Window GetFocusedWindow();

private:
    void setup();
    void handleEvent();
    void handleMapRequest();
    void handleConfigureRequest();
    void handleDestroyNotify();
    void handleKeyPress();
    void handleEnterNotify();
    void handleButtonPress();
    void handleMotionNotify();
    void handleButtonRelease();

    static int handleXError(Display *display, XErrorEvent *error_event);
};
