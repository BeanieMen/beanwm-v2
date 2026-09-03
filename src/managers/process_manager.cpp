#include "process_manager.h"
#include "config.h"
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <vector>
#include <sstream>

void ProcessManager::spawnTerminal()
{
    spawnProcess(TERMINAL);
}

void ProcessManager::spawnProcess(const std::string &command)
{
    if (fork() == 0)
    {
        std::istringstream stream(command);
        std::vector<std::string> args;
        std::string arg;

        while (stream >> arg)
            args.push_back(arg);

        std::vector<char *> argv;
        for (auto &a : args)
            argv.push_back(a.data());

        argv.push_back(nullptr);

        execvp(argv[0], argv.data());

        perror(argv[0]);
        _exit(127);
    }
}