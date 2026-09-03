#include "process_manager.h"
#include "config_manager.h"
#include <cstdio>
#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>

void ProcessManager::spawnTerminal()
{
    spawnProcess(ConfigManager::instance().get().terminal);
}

void ProcessManager::spawnProcess(const std::string &command)
{
    if (command.empty()) return;
    pid_t pid = fork();
    if (pid == 0)
    {
        if (fork() == 0)
        {
            setsid();
            execlp("/bin/sh", "sh", "-c", command.c_str(), nullptr);
            perror(command.c_str());
            _exit(127);
        }
        _exit(0);
    }
    if (pid > 0)
    {
        waitpid(pid, nullptr, 0);
    }
}