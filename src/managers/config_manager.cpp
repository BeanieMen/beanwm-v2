#include "config_manager.h"
#include "helpers/string_helper.h"
#include <X11/keysym.h>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

ConfigManager &ConfigManager::instance()
{
    static ConfigManager inst;
    return inst;
}

static bool pathExists(const std::string &path)
{
    return access(path.c_str(), F_OK) == 0;
}

static void makeParentDirs(const std::string &path)
{
    size_t slash = path.find_last_of('/');
    if (slash == std::string::npos || slash == 0) return;
    std::string dir = path.substr(0, slash);
    std::string cur;
    size_t start = 0;
    if (!dir.empty() && dir[0] == '/') { cur = "/"; start = 1; }
    while (start < dir.size())
    {
        size_t pos = dir.find('/', start);
        std::string part = pos == std::string::npos ? dir.substr(start)
                                                    : dir.substr(start, pos - start);
        if (!part.empty())
        {
            if (cur.empty()) cur = part;
            else if (cur == "/") cur += part;
            else { cur += "/"; cur += part; }
            mkdir(cur.c_str(), 0755);
        }
        if (pos == std::string::npos) break;
        start = pos + 1;
    }
}

std::string ConfigManager::findConfigPath() const
{
    // 1. $XDG_CONFIG_HOME/beanwm/config
    const char *xdg = getenv("XDG_CONFIG_HOME");
    if (xdg && xdg[0] != '\0')
    {
        std::string path = std::string(xdg) + "/beanwm/config";
        if (pathExists(path))
            return path;
    }

    // 2. ~/.config/beanwm/config (auto-copy from system default if missing)
    std::string home = getHomeDir();
    if (!home.empty())
    {
        std::string userPath = home + "/.config/beanwm/config";
        ensureUserConfig(userPath, "/etc/beanwm/config");
        if (pathExists(userPath))
            return userPath;
    }
    return "";
}

void ConfigManager::ensureUserConfig(const std::string &userPath,
                                     const std::string &systemPath) const
{
    if (pathExists(userPath))
        return;
    if (!pathExists(systemPath))
        return;

    makeParentDirs(userPath);

    std::ifstream src(systemPath, std::ios::binary);
    if (!src.is_open()) return;
    std::ofstream dst(userPath, std::ios::binary);
    if (!dst.is_open()) return;
    char buf[8192];
    while (src)
    {
        src.read(buf, sizeof(buf));
        std::streamsize n = src.gcount();
        if (n > 0) dst.write(buf, n);
    }
    dst.flush();
    if (dst)
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
    config_.cleanMask = ~(LockMask | Mod2Mask);
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

        if (section == "autostart")
        {
            std::string cmd = line;
            if (cmd.rfind("exec ", 0) == 0)
                cmd = trim(cmd.substr(5));
            else if (cmd.rfind("exec=", 0) == 0)
                cmd = trim(cmd.substr(5));
            else {
                size_t eq = cmd.find('=');
                if (eq != std::string::npos && trim(cmd.substr(0, eq)) == "exec")
                    cmd = trim(cmd.substr(eq + 1));
            }
            if (!cmd.empty())
                config_.autostart.push_back(cmd);
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
