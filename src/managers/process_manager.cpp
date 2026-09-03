#include "process_manager.h"
#include "config.h"
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

void ProcessManager::spawnTerminal()
{
    spawnProcess(TERMINAL);
}

void ProcessManager::spawnProcess(const std::string &command)
{
    if (fork() == 0)
    {
        execlp(command.c_str(), command.c_str(), nullptr);
        perror(command.c_str());
        _exit(127);
    }
}
