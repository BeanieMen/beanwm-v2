#pragma once

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <string>
#include <vector>

class WindowManager;

enum KeyAction
{
    ACTION_SWITCH_WORKSPACE = 0,
    ACTION_MOVE_WORKSPACE   = 1,
    ACTION_KILL             = 2,
    ACTION_QUIT             = 3,
    ACTION_EXEC             = 4,
    ACTION_FLOAT            = 5,
};

struct KeyBinding
{
    unsigned int modifiers;
    KeySym key;
    int action;
    int arg;
    std::string cmd;
};

// Key symbol tables — these are parser implementation details, not user config.
struct ModEntry { const char *name; unsigned int mask; };
inline constexpr ModEntry MOD_ENTRIES[] = {
    {"Mod4",    Mod4Mask},
    {"Super",   Mod4Mask},
    {"Mod1",    Mod1Mask},
    {"Alt",     Mod1Mask},
    {"Shift",   ShiftMask},
    {"Control", ControlMask},
    {"Ctrl",    ControlMask},
};

struct KeyEntry { const char *name; KeySym keysym; };
inline constexpr KeyEntry KEY_ENTRIES[] = {
    {"Return",    XK_Return},
    {"Enter",     XK_Return},
    {"Escape",    XK_Escape},
    {"BackSpace", XK_BackSpace},
    {"Tab",       XK_Tab},
    {"Space",     XK_space},
    {"Up",        XK_Up},
    {"Down",      XK_Down},
    {"Left",      XK_Left},
    {"Right",     XK_Right},
    {"F1",  XK_F1},  {"F2",  XK_F2},  {"F3",  XK_F3},  {"F4",  XK_F4},
    {"F5",  XK_F5},  {"F6",  XK_F6},  {"F7",  XK_F7},  {"F8",  XK_F8},
    {"F9",  XK_F9},  {"F10", XK_F10}, {"F11", XK_F11}, {"F12", XK_F12},
};

class KeybindingManager
{
private:
    std::vector<KeyBinding> keybindings;

public:
    KeybindingManager() = default;
    ~KeybindingManager() = default;

    void setupKeybindings();
    bool parseKeyBinding(const std::string &combo,
                         const std::string &action_string,
                         KeyBinding &binding);

    void grabKeys(Display *display, Window root);
    void handleKeyPress(Display *display, XEvent &event, WindowManager &wm);

    unsigned int parseModString(const std::string &str);
    KeySym parseKeyString(const std::string &str);
};
