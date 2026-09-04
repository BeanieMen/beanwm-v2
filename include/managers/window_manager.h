#pragma once

#include "types.h"
#include "client_manager.h"
#include "layout_manager.h"
#include "keybinding_manager.h"
#include "process_manager.h"
#include "config_manager.h"

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <vector>

class WindowManager
{
private:
    Display *display;
    Window   root;
    XEvent   event;

    ClientManager     clientManager;
    LayoutManager     layoutManager;
    KeybindingManager keybindingManager;
    ProcessManager    processManager;

    Window draggedWindow    = None;
    Window dragTarget       = None;
    int    dragStartX       = 0;
    int    dragStartY       = 0;
    int    dragWindowX      = 0;
    int    dragWindowY      = 0;
    int    dragWindowW      = 0;   // saved width for cross-screen scaling
    int    dragWindowH      = 0;   // saved height for cross-screen scaling
    bool   dragIsFloating   = false;
    int    dragScreenIndex  = 0;   // screen client started drag on

    /* Multi-screen support */
    std::vector<ScreenInfo> screens;   // Detected monitor geometries (Xinerama)
    int currentScreenIndex = 0;        // Which screen has keyboard focus

public:
    WindowManager();
    ~WindowManager();
    void run();

    ClientManager     &getClientManager()    { return clientManager; }
    LayoutManager     &getLayoutManager()    { return layoutManager; }
    KeybindingManager &getKeybindingManager() { return keybindingManager; }
    ProcessManager    &getProcessManager()   { return processManager; }

    void tile();
    void switchWorkspace(int workspace);
    void moveToWorkspace(int workspace);
    void quit();
    void rebuildAndReload();

    Window GetFocusedWindow();
    void reloadConfig();

    /* Multi-screen helpers */
    const std::vector<ScreenInfo> &getScreens() const { return screens; }
    int getCurrentScreenIndex() const { return currentScreenIndex; }
    const ScreenInfo *getScreenAt(int x, int y) const;   // find screen containing (x,y)
    const ScreenInfo *screenForWindow(Window w) const;   // find screen a window lives on

    void detectScreens();

private:
    void setup();
    void saveState();
    bool restoreState();
    int  screenIndexForPoint(int x, int y) const;

    void handleEvent();
    void handleMapRequest();
    void handleConfigureRequest();
    void handleDestroyNotify();
    void handleKeyPress();
    void handleEnterNotify();
    void handleButtonPress();
    void handleMotionNotify();
    void handleButtonRelease();
    void handlePropertyNotify();
    void handleExpose();

    static int handleXError(Display *display, XErrorEvent *error_event);
};
