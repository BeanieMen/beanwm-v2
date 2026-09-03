#include "window_manager.h"
#include "config.h"
#include <X11/XKBlib.h>
#include <cctype>
#include <string>
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

bool WindowManager::parseKeyBinding(
    const std::string &combo,
    const std::string &action_string,
    KeyBinding &binding)
{
    size_t last_plus = combo.rfind('+');
    std::string modifier_part = last_plus == std::string::npos ? "" : combo.substr(0, last_plus);
    std::string key_part = last_plus == std::string::npos ? combo : combo.substr(last_plus + 1);
    KeySym key = parseKeyString(key_part);
    if (key == NoSymbol)
        return false;

    binding.modifiers = modifier_part.empty() ? 0 : parseModString(modifier_part);
    binding.key = key;
    binding.action = -1;
    binding.arg = 0;
    binding.cmd.clear();

    if (action_string.rfind("exec", 0) == 0)
    {
            std::string command = trim(action_string.substr(4));
        if (command == "terminal" || command == "term")
            binding.action = ACTION_SPAWN_TERMINAL;
        else
        {
            binding.action = ACTION_EXEC;
            binding.cmd = command;
        }
    }
    else if (action_string.rfind("workspace", 0) == 0)
    {
        std::string number = trim(action_string.substr(9));
        try { binding.arg = number.empty() ? 1 : std::stoi(number); }
        catch (...) { binding.arg = 1; }
        binding.action = ACTION_SWITCH_WORKSPACE;
    }
    else if (action_string.rfind("move", 0) == 0)
    {
        std::string number = trim(action_string.substr(4));
        try { binding.arg = number.empty() ? 1 : std::stoi(number); }
        catch (...) { binding.arg = 1; }
        binding.action = ACTION_MOVE_WORKSPACE;
    }
    else if (action_string.rfind("kill_focused", 0) == 0)
    {
        binding.action = ACTION_KILL;
    }

    return binding.action != -1;
}

void WindowManager::setupKeybindings()
{
    keybindings.clear();
    for (auto &d : DEFAULT_BINDS)
    {
        std::string combo = d.combo;
        std::string actionStr = d.action;
        KeyBinding binding{};
        if (parseKeyBinding(combo, actionStr, binding))
            keybindings.push_back(binding);
    }
}

void WindowManager::handleKeyPress()
{
    KeySym k = XLookupKeysym(&event.xkey, 0);
    unsigned int m = event.xkey.state & CLEANMASK;
    for (auto &b : keybindings)
    {
        if (b.key != k || b.modifiers != m)
            continue;
        if (b.action == ACTION_SPAWN_TERMINAL)
            spawnTerminal();
        else if (b.action == ACTION_SWITCH_WORKSPACE)
            switchWorkspace(b.arg);
        else if (b.action == ACTION_MOVE_WORKSPACE)
            moveToWorkspace(b.arg);
        else if (b.action == ACTION_KILL)
        {
            Window focused = 0;
            int revert = 0;
            XGetInputFocus(display, &focused, &revert);
            if (focused == None || focused == root)
                return;
            Client *c = findClient(focused); // only kill managed windows
            if (!c)
                return;
            Atom wm_delete = XInternAtom(display, "WM_DELETE_WINDOW", False);
            Atom wm_protocols = XInternAtom(display, "WM_PROTOCOLS", False);
            Atom *p = nullptr;
            int n = 0;
            bool has = false;
            if (XGetWMProtocols(display, c->window, &p, &n))
            {
                for (int i = 0; i < n; ++i)
                    if (p[i] == wm_delete)
                        has = true;
                XFree(p);
            }
            if (has)
            {
                XEvent ev{};
                ev.type = ClientMessage;
                ev.xclient.window = c->window;
                ev.xclient.message_type = wm_protocols;
                ev.xclient.format = 32;
                ev.xclient.data.l[0] = wm_delete;
                XSendEvent(display, c->window, False, NoEventMask, &ev);
            }
            else
                XKillClient(display, c->window);
            XSync(display, False);
        }
        else if (b.action == ACTION_QUIT)
        {
            if (display)
                XCloseDisplay(display);
            exit(0);
        }
        else if (b.action == ACTION_EXEC)
        {
            if (fork() == 0)
            {
                execlp(b.cmd.c_str(), b.cmd.c_str(), nullptr);
                exit(1);
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
    return key == NoSymbol ? FALLBACK_KEYSYM : key;
}
