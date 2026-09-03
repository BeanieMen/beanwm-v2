#pragma once

#include "types.h"
#include "config_manager.h"
#include <X11/Xlib.h>
#include <vector>
#include <functional>

class ClientManager
{
private:
    int current_workspace = 1;
    std::vector<Client> tiledClients;
    std::vector<Client> floatingClients;

public:
    ClientManager() = default;
    ~ClientManager() = default;

    int getCurrentWorkspace() const { return current_workspace; }
    void setCurrentWorkspace(int ws) { current_workspace = ws; }

    std::vector<Client> &getTiledClients()        { return tiledClients; }
    const std::vector<Client> &getTiledClients() const { return tiledClients; }
    std::vector<Client> &getFloatingClients()     { return floatingClients; }
    const std::vector<Client> &getFloatingClients() const { return floatingClients; }

    void   addClient(Window window, ManagementMode mode);
    bool   removeClient(Window window);
    Client *findClient(Window window);

    int  getWindowNumber(Window window);
    void updateClientNumbers();

    void switchWorkspace(Display *display, int workspace,
                         std::function<void()> onWorkspaceChanged);
    void moveToWorkspace(Display *display, Window root, int workspace,
                         std::function<void()> onWorkspaceChanged);
    void showWorkspace(Display *display, int workspace);
    void hideWorkspace(Display *display, int workspace);

    void switchTileWinToFloating(Display *display, Window window,
                                 std::function<void()> onTileChanged);
    void updateClientGeometry(Display *display, Client &client);
    Window getTopClientWindow() const;
};
