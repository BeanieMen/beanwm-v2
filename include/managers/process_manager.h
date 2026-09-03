#pragma once

#include <string>

class ProcessManager
{
public:
    ProcessManager() = default;
    ~ProcessManager() = default;

    void spawnTerminal();
    void spawnProcess(const std::string &command);
};
