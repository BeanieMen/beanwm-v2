#pragma once

#include <X11/Xlib.h>
#include <X11/keysym.h>

// Appearance — uniform gap (outer edge and between windows are the same)
inline constexpr int GAP = 5;

// Workspaces — number of workspaces (1..9)
// Keybindings Mod+1..9 and Mod+Shift+1..9 are generated from this value.
inline constexpr int WORKSPACE_COUNT = 9;

// Modifier key for shortcuts — Mod4Mask is Super/Windows, Mod1Mask is Alt
inline constexpr unsigned int MODKEY = Mod4Mask;

// Terminal spawned on Mod+Return
inline constexpr const char TERMINAL[] = "alacritty";
inline constexpr const char* TERMINAL_CMD[] = { "alacritty", nullptr };

// Mod names for key_parser — config defines everything under input/
struct ModEntry { const char* name; unsigned int mask; };
inline constexpr ModEntry MOD_ENTRIES[] = {
    {"Mod4", Mod4Mask},
    {"Super", Mod4Mask},
    {"Mod1", Mod1Mask},
    {"Alt", Mod1Mask},
    {"Shift", ShiftMask},
    {"Control", ControlMask},
    {"Ctrl", ControlMask},
};

// Special keys for key_parser — single chars 0-9 a-z handled generically, these are extras
struct KeyEntry { const char* name; KeySym keysym; };
inline constexpr KeyEntry KEY_ENTRIES[] = {
    {"Return", XK_Return},
    {"Enter", XK_Return},
    {"q", XK_q},
    {"Q", XK_q},
};

// Mask used to compare modifiers (ignore Lock/NumLock) — config defines input handling
inline constexpr unsigned int CLEANMASK = ShiftMask | ControlMask | Mod1Mask | Mod4Mask;
inline constexpr KeySym FALLBACK_KEYSYM = XK_q;

// Default keybindings — config defines everything under input/
struct DefaultBind { const char* combo; const char* action; };
inline constexpr DefaultBind DEFAULT_BINDS[] = {
    {"Mod4+Return", "exec terminal"},
    {"Mod4+1", "workspace 1"},
    {"Mod4+2", "workspace 2"},
    {"Mod4+3", "workspace 3"},
    {"Mod4+4", "workspace 4"},
    {"Mod4+5", "workspace 5"},
    {"Mod4+6", "workspace 6"},
    {"Mod4+7", "workspace 7"},
    {"Mod4+8", "workspace 8"},
    {"Mod4+9", "workspace 9"},
    {"Mod4+Shift+1", "move 1"},
    {"Mod4+Shift+2", "move 2"},
    {"Mod4+Shift+3", "move 3"},
    {"Mod4+Shift+4", "move 4"},
    {"Mod4+Shift+5", "move 5"},
    {"Mod4+Shift+6", "move 6"},
    {"Mod4+Shift+7", "move 7"},
    {"Mod4+Shift+8", "move 8"},
    {"Mod4+Shift+9", "move 9"},
    {"Mod4+Shift+q", "kill_focused"},
    {"Mod4+Shift+f", "float"},
    {"Mod4+Shift+Escape", "quit"},
};
