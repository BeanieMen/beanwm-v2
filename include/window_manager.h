#pragma once

#include "types.h"
#include "config.h"

#include <X11/Xlib.h>
#include <X11/keysym.h>

#include <string>
#include <vector>

class WindowManager
{
private:
    static constexpr int WORKSPACE_COUNT = ::WORKSPACE_COUNT;
    int current_workspace;

    enum
    {
        ACTION_SPAWN_TERMINAL = 0,
        ACTION_SWITCH_WORKSPACE = 1,
        ACTION_MOVE_WORKSPACE = 2,
        ACTION_KILL = 3,
        ACTION_QUIT = 4,
        ACTION_EXEC = 5,
        ACTION_FLOAT = 6
    };

    struct KeyBinding
    {
        unsigned int modifiers;
        KeySym key;
        int action;
        int arg;
        std::string cmd;
    };

    Display *display;
    Window root;
    XEvent event;

    std::vector<Client> tiledClients;
    std::vector<Client> floatingClients;
    std::vector<KeyBinding> keybindings;

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

private:
    void setup();
    void setupKeybindings();
    bool parseKeyBinding(const std::string &combo, const std::string &action_string, KeyBinding &binding);
    unsigned int parseModString(const std::string &str);
    KeySym parseKeyString(const std::string &str);

    void handleEvent();
    void handleMapRequest();
    void handleConfigureRequest();
    void handleDestroyNotify();
    void handleKeyPress();
    void handleEnterNotify();
    void handleButtonPress();
    void handleMotionNotify();
    void handleButtonRelease();

    void spawnTerminal();
    void spawnProcess(const std::string &command);

    void addClient(Window window, ManagementMode mode);
    bool removeClient(Window window);

    void tile();
    void switchTileWinToFloating(Window &window);
    void updateClientGeometry(Client &client);

    void switchWorkspace(int workspace);
    void moveToWorkspace(int workspace);
    void showWorkspace(int workspace);
    void hideWorkspace(int workspace);

    Client *findClient(Window window);
    int getWindowNumber(Window window);
    void updateClientNumbers();

    static int handleXError(
        Display *display,
        XErrorEvent *error_event);
    Window GetFocusedWindow();
};
