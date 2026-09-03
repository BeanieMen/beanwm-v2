#include "window_manager.h"
#include "config.h"
#include <X11/XKBlib.h>
#include <cstdlib>
#include <unistd.h>

static std::string trim(const std::string& value)
{
    size_t start = value.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return "";
    size_t end = value.find_last_not_of(" \t\r\n");
    return value.substr(start, end - start + 1);
}

static int parseWorkspaceNumber(const std::string& value)
{
    try {
        return value.empty() ? 1 : std::stoi(value);
    }
    catch (...) {
        return 1;
    }
}


bool WindowManager::parseKeyBinding(
    const std::string &combo,
    const std::string &action_string,
    KeyBinding &binding)
{
    binding.action = -1;
    binding.arg = 0;
    binding.cmd.clear();

    size_t plus_pos = combo.rfind('+');
    std::string mod_part = plus_pos == std::string::npos ? "" : combo.substr(0, plus_pos);
    std::string key_part = plus_pos == std::string::npos ? combo : combo.substr(plus_pos + 1);

    KeySym key = parseKeyString(key_part);
    if (key == NoSymbol) return false;

    binding.key = key;

    binding.modifiers = mod_part.empty() ? 0 : parseModString(mod_part);
    std::string action = trim(action_string);
    if (action == "exec terminal" || action == "exec term")
        binding.action = ACTION_SPAWN_TERMINAL;
    else if (action.rfind("workspace", 0) == 0) {
        binding.action = ACTION_SWITCH_WORKSPACE;
        binding.arg = parseWorkspaceNumber(trim(action.substr(9)));
    } else if (action.rfind("move", 0) == 0) {
        binding.action = ACTION_MOVE_WORKSPACE;
        binding.arg = parseWorkspaceNumber(trim(action.substr(4)));
    } else if (action == "kill_focused")
        binding.action = ACTION_KILL;
    else if (action == "float")
        binding.action = ACTION_FLOAT;
    else if (action == "quit" || action == "exit")
        binding.action = ACTION_QUIT;
    else if (action.rfind("exec ", 0) == 0) {
        binding.action = ACTION_EXEC;
        binding.cmd = trim(action.substr(5));
    }
    return binding.action != -1;
}

void WindowManager::setupKeybindings()
{
    keybindings.clear();
    keybindings.reserve(sizeof(DEFAULT_BINDS) / sizeof(DEFAULT_BINDS[0]));
    for (const auto &d : DEFAULT_BINDS) {
        KeyBinding binding{};
        if (parseKeyBinding(d.combo, d.action, binding))
            keybindings.push_back(binding);
    }
}

void WindowManager::handleKeyPress()
{
    KeySym k = XLookupKeysym(&event.xkey, 0);
    unsigned int m = event.xkey.state & CLEANMASK;

    for (auto &b : keybindings) {
        if (b.key != k || b.modifiers != m) continue;

        if (b.action == ACTION_SPAWN_TERMINAL) {
            spawnTerminal();
        } else if (b.action == ACTION_SWITCH_WORKSPACE) {
            switchWorkspace(b.arg);
        } else if (b.action == ACTION_MOVE_WORKSPACE) {
            moveToWorkspace(b.arg);
        } else if (b.action == ACTION_KILL) {
            Window focused = GetFocusedWindow();
            Client *client = findClient(focused);
            if (!client) return;
            Atom delete_atom = XInternAtom(display, "WM_DELETE_WINDOW", False);
            Atom protocols_atom = XInternAtom(display, "WM_PROTOCOLS", False);
            Atom *protocols = nullptr;
            int count = 0;
            bool supports_delete = false;
            if (XGetWMProtocols(display, client->window, &protocols, &count)) {
                for (int i = 0; i < count; ++i)
                    if (protocols[i] == delete_atom)
                        supports_delete = true;
                XFree(protocols);
            }
            if (supports_delete) {
                XEvent close_event{};
                close_event.type = ClientMessage;
                close_event.xclient.window = client->window;
                close_event.xclient.message_type = protocols_atom;
                close_event.xclient.format = 32;
                close_event.xclient.data.l[0] = delete_atom;
                XSendEvent(display, client->window, False, NoEventMask, &close_event);
            } else {
                XKillClient(display, client->window);
            }
        } else if (b.action == ACTION_FLOAT) {
            Window focused = GetFocusedWindow();
            Client *client = findClient(focused);
            if (client && client->mode == MODE_TILED) {
                switchTileWinToFloating(client->window);
            }
        } else if (b.action == ACTION_QUIT) {
            if (display) XCloseDisplay(display);
            exit(0);
        } else if (b.action == ACTION_EXEC && !b.cmd.empty()) {
            if (fork() == 0) {
                execlp(b.cmd.c_str(), b.cmd.c_str(), nullptr);
                _exit(127);
            }
        }
        return;
    }
}


unsigned int WindowManager::parseModString(const std::string &s)
{
    unsigned int modifiers = 0;
    size_t start = 0;
    while (true)
    {
        size_t position = s.find('+', start);
        std::string token = trim(position == std::string::npos ? s.substr(start) : s.substr(start, position - start));
        for (auto &entry : MOD_ENTRIES)
        {
            if (token == entry.name)
            {
                modifiers |= entry.mask;
                break;
            }
        }
        if (position == std::string::npos)
            break;
        start = position + 1;
    }
    return modifiers;
}

KeySym WindowManager::parseKeyString(const std::string &s)
{
    std::string text = trim(s);
    for (auto &entry : KEY_ENTRIES)
    {
        if (text == entry.name)
            return entry.keysym;
    }
    if (text.size() == 1)
    {
        char character = text[0];
        if (character >= '0' && character <= '9')
            return XK_0 + (character - '0');
        if (character >= 'a' && character <= 'z')
            return XK_a + (character - 'a');
        if (character >= 'A' && character <= 'Z')
            return XK_A + (character - 'A');
    }
    KeySym key = XStringToKeysym(text.c_str());
    if (key == NoSymbol)
    {
        for (char &character : text)
            character = static_cast<char>(tolower(static_cast<unsigned char>(character)));
        key = XStringToKeysym(text.c_str());
    }
    return key;
}
