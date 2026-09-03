#include "keybinding_manager.h"
#include "window_manager.h"
#include "config_manager.h"
#include "helpers/string_helper.h"
#include <X11/XKBlib.h>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>

bool KeybindingManager::parseKeyBinding(const std::string &combo,
                                         const std::string &action_string,
                                         KeyBinding &binding)
{
    binding.action = -1;
    binding.arg    = 0;
    binding.cmd.clear();

    size_t plus_pos = combo.rfind('+');
    std::string mod_part = plus_pos == std::string::npos ? "" : combo.substr(0, plus_pos);
    std::string key_part = plus_pos == std::string::npos ? combo : combo.substr(plus_pos + 1);

    KeySym key = parseKeyString(key_part);
    if (key == NoSymbol) return false;

    binding.key       = key;
    binding.modifiers = mod_part.empty() ? 0 : parseModString(mod_part);

    std::string action = trim(action_string);

    if (action.rfind("workspace", 0) == 0) {
        binding.action = ACTION_SWITCH_WORKSPACE;
        binding.arg    = parseWorkspaceNumber(action.substr(9));
    } else if (action.rfind("move", 0) == 0) {
        binding.action = ACTION_MOVE_WORKSPACE;
        binding.arg    = parseWorkspaceNumber(action.substr(4));
    } else if (action == "kill_focused")
        binding.action = ACTION_KILL;
    else if (action == "float")
        binding.action = ACTION_FLOAT;
    else if (action == "quit" || action == "exit")
        binding.action = ACTION_QUIT;
    else if (action == "rebuild_reload" || action == "rebuild" || action == "reload" || action == "restart")
        binding.action = ACTION_REBUILD_RELOAD;
    else if (action.rfind("exec ", 0) == 0) {
        binding.action = ACTION_EXEC;
        binding.cmd    = trim(action.substr(5));
    }

    return binding.action != -1;
}

void KeybindingManager::setupKeybindings()
{
    const auto &binds = ConfigManager::instance().get().binds;
    keybindings.clear();
    keybindings.reserve(binds.size());
    for (const auto &[combo, action] : binds)
    {
        KeyBinding binding{};
        if (parseKeyBinding(combo, action, binding))
            keybindings.push_back(binding);
    }
}

void KeybindingManager::grabKeys(Display *display, Window root)
{
    XUngrabKey(display, AnyKey, AnyModifier, root);
    for (auto &b : keybindings)
    {
        KeyCode c = XKeysymToKeycode(display, b.key);
        if (!c) continue;
        XGrabKey(display, c, b.modifiers, root, False, GrabModeAsync, GrabModeAsync);
        XGrabKey(display, c, b.modifiers | LockMask, root, False, GrabModeAsync, GrabModeAsync);
        XGrabKey(display, c, b.modifiers | Mod2Mask, root, False, GrabModeAsync, GrabModeAsync);
        XGrabKey(display, c, b.modifiers | LockMask | Mod2Mask, root, False, GrabModeAsync, GrabModeAsync);
    }
}

void KeybindingManager::handleKeyPress(Display *display, XEvent &event, WindowManager &wm)
{
    KeySym k        = XLookupKeysym(&event.xkey, 0);
    unsigned int m  = event.xkey.state & ConfigManager::instance().get().cleanMask;

    for (auto &b : keybindings)
    {
        if (b.key != k || b.modifiers != m) continue;

        if (b.action == ACTION_SWITCH_WORKSPACE) {
            wm.switchWorkspace(b.arg);
        } else if (b.action == ACTION_MOVE_WORKSPACE) {
            wm.moveToWorkspace(b.arg);
        } else if (b.action == ACTION_KILL) {
            Window focused = wm.GetFocusedWindow();
            Client *client = wm.getClientManager().findClient(focused);
            if (!client) return;
            Atom del   = XInternAtom(display, "WM_DELETE_WINDOW", False);
            Atom proto = XInternAtom(display, "WM_PROTOCOLS", False);
            Atom *protocols = nullptr;
            int count = 0;
            bool supports_delete = false;
            if (XGetWMProtocols(display, client->window, &protocols, &count)) {
                for (int i = 0; i < count; ++i)
                    if (protocols[i] == del) supports_delete = true;
                XFree(protocols);
            }
            if (supports_delete) {
                XEvent e{};
                e.type                 = ClientMessage;
                e.xclient.window       = client->window;
                e.xclient.message_type = proto;
                e.xclient.format       = 32;
                e.xclient.data.l[0]    = del;
                XSendEvent(display, client->window, False, NoEventMask, &e);
            } else {
                XKillClient(display, client->window);
            }
        } else if (b.action == ACTION_FLOAT) {
            Window focused = wm.GetFocusedWindow();
            wm.getClientManager().switchTileWinToFloating(display, focused, [&wm]() {
                wm.tile();
            });
        } else if (b.action == ACTION_QUIT) {
            XCloseDisplay(display);
            exit(0);
        } else if (b.action == ACTION_REBUILD_RELOAD) {
            wm.rebuildAndReload();
        } else if (b.action == ACTION_EXEC && !b.cmd.empty()) {
            std::string cmd = b.cmd;
            if (cmd == "terminal")
                cmd = ConfigManager::instance().get().terminal;
            wm.getProcessManager().spawnProcess(cmd);
        }
        return;
    }
}

unsigned int KeybindingManager::parseModString(const std::string &s)
{
    unsigned int modifiers = 0;
    size_t start = 0;
    while (true)
    {
        size_t pos   = s.find('+', start);
        std::string token = trim(pos == std::string::npos ? s.substr(start) : s.substr(start, pos - start));
        for (const auto &entry : MOD_ENTRIES)
            if (token == entry.name) { modifiers |= entry.mask; break; }
        if (pos == std::string::npos) break;
        start = pos + 1;
    }
    return modifiers;
}

KeySym KeybindingManager::parseKeyString(const std::string &s)
{
    std::string text = trim(s);
    for (const auto &entry : KEY_ENTRIES)
        if (toLower(text) == toLower(entry.name)) return entry.keysym;

    if (text.size() == 1)
    {
        char c = text[0];
        if (c >= '0' && c <= '9') return XK_0 + (c - '0');
        if (c >= 'a' && c <= 'z') return XK_a + (c - 'a');
        if (c >= 'A' && c <= 'Z') return XK_A + (c - 'A');
    }

    KeySym key = XStringToKeysym(text.c_str());
    if (key == NoSymbol)
    {
        key = XStringToKeysym(toLower(text).c_str());
    }
    return key;
}
