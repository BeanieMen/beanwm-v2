#pragma once

#include "config.h"
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <string>
#include <vector>

class WindowManager;

enum KeyAction
{
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

class KeybindingManager
{
private:
    std::vector<KeyBinding> keybindings;

public:
    KeybindingManager() = default;
    ~KeybindingManager() = default;

    void setupKeybindings();
    bool parseKeyBinding(const std::string &combo, const std::string &action_string, KeyBinding &binding);
    unsigned int parseModString(const std::string &str);
    KeySym parseKeyString(const std::string &str);

    void grabKeys(Display *display, Window root);
    void handleKeyPress(Display *display, XEvent &event, WindowManager &wm);
};
