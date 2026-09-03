#include "config_manager.h"
#include "helpers/string_helper.h"
#include <X11/keysym.h>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>

ConfigManager &ConfigManager::instance()
{
    static ConfigManager inst;
    return inst;
}

std::string ConfigManager::findConfigPath() const
{
    // 1. $XDG_CONFIG_HOME/beanwm/config
    const char *xdg = getenv("XDG_CONFIG_HOME");
    if (xdg && xdg[0] != '\0')
    {
        std::string path = std::string(xdg) + "/beanwm/config";
        if (std::filesystem::exists(path))
            return path;
    }

    // 2. ~/.config/beanwm/config (auto-copy from system default if missing)
    std::string home = getHomeDir();
    if (!home.empty())
    {
        std::string userPath = home + "/.config/beanwm/config";
        ensureUserConfig(userPath, "/etc/beanwm/config");
        if (std::filesystem::exists(userPath))
            return userPath;
    }
    return "";
}

void ConfigManager::ensureUserConfig(const std::string &userPath,
                                     const std::string &systemPath) const
{
    if (std::filesystem::exists(userPath))
        return;
    if (!std::filesystem::exists(systemPath))
        return;

    std::error_code ec;
    std::filesystem::create_directories(
        std::filesystem::path(userPath).parent_path(), ec);
    if (ec) return;

    std::filesystem::copy_file(
        systemPath, userPath,
        std::filesystem::copy_options::skip_existing, ec);
    if (!ec)
        fprintf(stderr, "[beanwm] Created user config at %s\n", userPath.c_str());
}

void ConfigManager::load()
{
    config_ = Config{};
    std::string path = findConfigPath();
    if (path.empty())
    {
        fprintf(stderr, "[beanwm] No config file found — using built-in defaults\n");
        return;
    }
    fprintf(stderr, "[beanwm] Loading config: %s\n", path.c_str());
    parseFile(path);
    config_.cleanMask = ShiftMask | ControlMask | Mod1Mask | config_.modKey;
}

void ConfigManager::parseFile(const std::string &path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        fprintf(stderr, "[beanwm] Cannot open config: %s\n", path.c_str());
        return;
    }

    std::string section;
    std::string line;

    while (std::getline(file, line))
    {
        size_t hash = line.find('#');
        if (hash != std::string::npos)
            line = line.substr(0, hash);

        line = trim(line);
        if (line.empty()) continue;

        if (line.front() == '[' && line.back() == ']')
        {
            section = line.substr(1, line.size() - 2);
            continue;
        }

        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key   = trim(line.substr(0, eq));
        std::string value = trim(line.substr(eq + 1));
        if (key.empty()) continue;

        if (section == "general")
            parseGeneralLine(key, value);
        else if (section == "binds")
            config_.binds.emplace_back(key, value);
    }
}

void ConfigManager::parseGeneralLine(const std::string &key, const std::string &value)
{
    if (key == "terminal")
        config_.terminal = value;
    else if (key == "gap")
    {
        try { config_.gap = std::stoi(value); } catch (...) {}
    }
    else if (key == "workspaces")
    {
        try { config_.workspaceCount = std::stoi(value); } catch (...) {}
    }
    else if (key == "modkey")
        config_.modKey = parseModKey(value);
}

unsigned int ConfigManager::parseModKey(const std::string &name) const
{
    if (name == "Mod4" || name == "Super")   return Mod4Mask;
    if (name == "Mod1" || name == "Alt")     return Mod1Mask;
    if (name == "Control" || name == "Ctrl") return ControlMask;
    if (name == "Shift")                      return ShiftMask;
    return Mod4Mask;
}
