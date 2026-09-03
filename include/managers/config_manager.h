#pragma once

#include <X11/Xlib.h>
#include <string>
#include <vector>
#include <utility>

struct Config
{
    std::string terminal    = "alacritty";
    int gap                 = 5;
    int workspaceCount      = 9;
    unsigned int modKey     = Mod4Mask;
    unsigned int cleanMask  = ShiftMask | ControlMask | Mod1Mask | Mod4Mask;

    // Keybindings: { "Mod4+Return", "exec terminal" }
    std::vector<std::pair<std::string, std::string>> binds;
};

class ConfigManager
{
public:
    static ConfigManager &instance();

    void load();
    const Config &get() const { return config_; }

private:
    ConfigManager() = default;
    ConfigManager(const ConfigManager &) = delete;
    ConfigManager &operator=(const ConfigManager &) = delete;

    std::string findConfigPath() const;
    void ensureUserConfig(const std::string &userPath, const std::string &systemPath) const;
    void parseFile(const std::string &path);
    void parseGeneralLine(const std::string &key, const std::string &value);
    unsigned int parseModKey(const std::string &name) const;

    Config config_;
};
