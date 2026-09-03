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
    static constexpr int DEFAULT_GAP = ::GAP;
    static constexpr unsigned int DEFAULT_MODKEY = ::MODKEY;

    enum {
        ACTION_SPAWN_TERMINAL = 0,
        ACTION_SWITCH_WORKSPACE = 1,
        ACTION_MOVE_WORKSPACE = 2,
        ACTION_QUIT = 3,
        ACTION_EXEC = 4
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

    std::vector<Client> clients;

    int current_workspace;
    unsigned int MOD_MASK = MODKEY;
    std::vector<KeyBinding> keybindings;

    // Runtime editable config (i3-like) — loaded from ./config, overrides compiled defaults
    int gap = GAP;
    int runtime_workspace_count = WORKSPACE_COUNT;
    std::string runtime_terminal = TERMINAL;
    bool has_runtime_keybinds = false;

public:
    WindowManager();
    ~WindowManager();
    void run();

private:
    void setup();
    void setupKeybindings();
    void loadRuntimeConfig();
    bool parseKeyBinding(const std::string& combo, const std::string& action_string, KeyBinding& binding);
    unsigned int parseModString(const std::string &str);
    KeySym parseKeyString(const std::string &str);

    void handleEvent();
    void handleMapRequest();
    void handleCreateNotify();
    void handleDestroyNotify();
    void handleKeyPress();
    void handleEnterNotify();

    void spawnTerminal();

    void addClient(Window window);
    bool removeClient(Window window);

    void tile();

    void switchWorkspace(int workspace);
    void moveToWorkspace(int workspace);
    void showWorkspace(int workspace);
    void hideWorkspace(int workspace);

    Client *findClient(Window window);

    static int handleXError(
        Display *display,
        XErrorEvent *error_event
    );
};
