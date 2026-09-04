#pragma once

#include "types.h"
#include "config_manager.h"
#include <X11/Xlib.h>
#include <vector>

class ClientManager
{
private:
    int current_workspace = 1;   // Global — same for all screens
    std::vector<Client> tiledClients;
    std::vector<Client> floatingClients;

public:
    ClientManager() = default;
    ~ClientManager() = default;

    // Single global workspace number shared across all screens
    int  getCurrentWorkspace() const { return current_workspace; }
    void setCurrentWorkspace(int ws) { current_workspace = ws; }

    std::vector<Client> &getTiledClients()              { return tiledClients; }
    const std::vector<Client> &getTiledClients() const  { return tiledClients; }
    std::vector<Client> &getFloatingClients()            { return floatingClients; }
    const std::vector<Client> &getFloatingClients() const { return floatingClients; }

    void   addClient(Window window, int screenIndex, ManagementMode mode);
    void   addClient(Window window, int screenIndex, int workspace, ManagementMode mode);
    bool   removeClient(Window window);
    Client *findClient(Window window);

    int  getWindowNumber(Window window);
    void updateClientNumbers();

    // Workspace ops are global (affect all screens simultaneously).
    // Caller re-tiles after switch/move.
    void switchWorkspace(Display *display, int workspace);
    void moveToWorkspace(Display *display, Window root, int workspace);

    // Show/hide all clients on the given global workspace regardless of screen
    void showWorkspace(Display *display, int workspace);
    void hideWorkspace(Display *display, int workspace);

    void switchTileWinToFloating(Display *display, Window window);
    void updateClientGeometry(Display *display, Client &client);

    // Best focus candidate on the global workspace (first tiled, else first floating)
    Window getTopClientWindow() const;
};
