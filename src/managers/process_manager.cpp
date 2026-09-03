#include "process_manager.h"
#include "config_manager.h"
#include <cstdio>
#include <cstdlib>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

ProcessManager::~ProcessManager()
{
    terminateAll();
}

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
        if (setsid() == -1) _exit(127);
        execlp("/bin/sh", "sh", "-c", command.c_str(), nullptr);
        perror(command.c_str());
        _exit(127);
    }
    if (pid > 0)
        processGroups.push_back(pid);
}

void ProcessManager::terminateAll()
{
    for (pid_t pid : processGroups)
    {
        int childStatus = 0;
        if (waitpid(pid, &childStatus, WNOHANG) == pid)
            continue;
        kill(-pid, SIGTERM);
        kill(-pid, SIGKILL);
        waitpid(pid, nullptr, 0);
    }
    processGroups.clear();
}