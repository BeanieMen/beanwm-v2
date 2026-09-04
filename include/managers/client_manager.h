#pragma once

#include "types.h"
#include "config_manager.h"
#include <X11/Xlib.h>
#include <vector>
#include <map>
#include <functional>

class ClientManager
{
private:
    std::map<int, int> current_workspaces;
    std::vector<Client> tiledClients;
    std::vector<Client> floatingClients;

public:
    ClientManager() = default;
    ~ClientManager() = default;

    int getCurrentWorkspace(int screenIndex = 0) const {
        auto it = current_workspaces.find(screenIndex);
        return (it != current_workspaces.end()) ? it->second : 1;
    }
    void setCurrentWorkspace(int screenIndex, int ws) {
        current_workspaces[screenIndex] = ws;
    }

    std::vector<Client> &getTiledClients()        { return tiledClients; }
    const std::vector<Client> &getTiledClients() const { return tiledClients; }
    std::vector<Client> &getFloatingClients()     { return floatingClients; }
    const std::vector<Client> &getFloatingClients() const { return floatingClients; }

    void   addClient(Window window, int screenIndex, ManagementMode mode);
    void   addClient(Window window, int screenIndex, int workspace, ManagementMode mode);
    bool   removeClient(Window window);
    Client *findClient(Window window);

    int  getWindowNumber(Window window);
    void updateClientNumbers();

    void switchWorkspace(Display *display, int screenIndex, int workspace,
                         std::function<void()> onWorkspaceChanged);
    void moveToWorkspace(Display *display, Window root, int screenIndex, int workspace,
                         std::function<void()> onWorkspaceChanged);
    void showWorkspace(Display *display, int screenIndex, int workspace);
    void hideWorkspace(Display *display, int screenIndex, int workspace);

    void switchTileWinToFloating(Display *display, Window window,
                                 std::function<void()> onTileChanged);
    void updateClientGeometry(Display *display, Client &client);
    Window getTopClientWindow(int screenIndex = 0) const;
};
