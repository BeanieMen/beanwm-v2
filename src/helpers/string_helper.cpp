#include "helpers/string_helper.h"
#include <cctype>
#include <cstdlib>
#include <pwd.h>
#include <unistd.h>

std::string trim(const std::string &s)
{
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

std::string toLower(const std::string &s)
{
    std::string result = s;
    for (char &c : result)
        c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    return result;
}

int parseWorkspaceNumber(const std::string &s)
{
    try {
        std::string t = trim(s);
        return t.empty() ? 1 : std::stoi(t);
    }
    catch (...) {
        return 1;
    }
}

std::string getHomeDir()
{
    const char *home = getenv("HOME");
    if (home && home[0] != '\0') return home;
    struct passwd *pw = getpwuid(getuid());
    return pw ? pw->pw_dir : "";
}
