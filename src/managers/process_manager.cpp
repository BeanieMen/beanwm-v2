#include "process_manager.h"
#include "config_manager.h"
#include <cstdio>
#include <unistd.h>

void ProcessManager::spawnTerminal()
{
    spawnProcess(ConfigManager::instance().get().terminal);
}

void ProcessManager::spawnProcess(const std::string &command)
{
    if (command.empty()) return;
    if (fork() == 0)
    {
        execlp(command.c_str(), command.c_str(), nullptr);
        perror(command.c_str());
        _exit(127);
    }
}