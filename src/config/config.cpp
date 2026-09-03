#include "window_manager.h"
#include "config.h"
#include "parser.h"

#include <X11/keysym.h>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>
#include <unistd.h>

std::string trim(const std::string& value) {
    size_t start = value.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = value.find_last_not_of(" \t\r\n");
    return value.substr(start, end - start + 1);
}

int parseIntOrDefault(const std::string& value, int fallback) {
    try {
        return value.empty() ? fallback : std::stoi(value);
    } catch (...) {
        return fallback;
    }
}

bool parseConfigLine(const std::string& line, std::string& key, std::string& value) {
    std::string text = trim(line);
    if (text.empty() || text[0] == '#') return false;
    size_t separator = text.find_first_of(" \t");
    if (separator == std::string::npos) return false;
    key = text.substr(0, separator);
    value = trim(text.substr(separator + 1));
    return !value.empty();
}

void WindowManager::loadRuntimeConfig() {
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);
    gap = GAP;
    runtime_workspace_count = WORKSPACE_COUNT;
    runtime_terminal = TERMINAL;
    MOD_MASK = MODKEY;
    has_runtime_keybinds = false;

    const char* paths[] = {"config", "./config", "beanwm.conf", "./beanwm.conf"};
    std::string chosen;
    for (auto p : paths) {
        std::ifstream f(p);
        if (f.is_open()) { chosen = p; break; }
    }
    if (chosen.empty()) return;

    std::ifstream file(chosen);
    if (!file.is_open()) return;

    std::string line;
    std::vector<KeyBinding> binds;
    bool has_binds = false;

    while (std::getline(file, line)) {
        std::string key, value;
        if (!parseConfigLine(line, key, value)) {
            std::string t = trim(line);
            if (t.rfind("bindsym", 0) != 0) continue;
            std::string rest = trim(t.substr(7));
            size_t sp = rest.find(' ');
            if (sp == std::string::npos) continue;
            std::string combo = trim(rest.substr(0, sp));
            std::string actionStr = trim(rest.substr(sp + 1));
            if (combo.empty() || actionStr.empty()) continue;
            KeyBinding binding{};
            if (parseKeyBinding(combo, actionStr, binding)) {
                binds.push_back(binding);
                has_binds = true;
            }
            continue;
        }

        if (key == "gap") {
            int value_number = parseIntOrDefault(value, gap);
            if (value_number >= 0 && value_number < 100) gap = value_number;
        } else if (key == "mod") {
            if (value == "Mod4" || value == "Super") MOD_MASK = Mod4Mask;
            else if (value == "Mod1" || value == "Alt") MOD_MASK = Mod1Mask;
        } else if (key == "terminal") {
            runtime_terminal = value;
        } else if (key == "workspaces") {
            int value_number = parseIntOrDefault(value, runtime_workspace_count);
            if (value_number >= 1 && value_number <= 20) runtime_workspace_count = value_number;
        }
    }

    if (has_binds) {
        keybindings = binds;
        has_runtime_keybinds = true;
    }
}
