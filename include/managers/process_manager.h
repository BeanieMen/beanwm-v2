#pragma once

#include <sys/types.h>
#include <string>
#include <vector>

class ProcessManager
{
public:
    ProcessManager() = default;
    ~ProcessManager();

    void spawnTerminal();
    void spawnProcess(const std::string &command);
    void terminateAll();

private:
    std::vector<pid_t> processGroups;
};
