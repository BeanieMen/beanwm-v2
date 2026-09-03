#include "client_manager.h"
#include <functional>

void ClientManager::addClient(Window w, ManagementMode mode)
{
    if (findClient(w))
        return;
    if (mode == MODE_TILED)
        tiledClients.push_back(Client{w, current_workspace, MODE_TILED, 0, 0, 800, 600, 0});
    else
        floatingClients.push_back(Client{w, current_workspace, MODE_FLOATING, 0, 0, 800, 600, 0});
    updateClientNumbers();
}

bool ClientManager::removeClient(Window w)
{
    for (auto it = tiledClients.begin(); it != tiledClients.end(); ++it)
    {
        if (it->window == w)
        {
            tiledClients.erase(it);
            updateClientNumbers();
            return true;
        }
    }
    for (auto it = floatingClients.begin(); it != floatingClients.end(); ++it)
    {
        if (it->window == w)
        {
            floatingClients.erase(it);
            updateClientNumbers();
            return true;
        }
    }
    return false;
}

Client *ClientManager::findClient(Window w)
{
    for (auto &c : tiledClients)
        if (c.window == w)
            return &c;
    for (auto &c : floatingClients)
        if (c.window == w)
            return &c;
    return nullptr;
}

void ClientManager::updateClientNumbers()
{
    int number = 0;
    for (auto &client : tiledClients)
    {
        if (client.workspace == current_workspace)
            client.number = ++number;
        else
            client.number = 0;
    }
    for (auto &client : floatingClients)
    {
        if (client.workspace == current_workspace)
            client.number = ++number;
        else
            client.number = 0;
    }
}

int ClientManager::getWindowNumber(Window w)
{
    Client *c = findClient(w);
    return c ? c->number : 0;
}

void ClientManager::switchWorkspace(Display *display, int ws, std::function<void()> onWorkspaceChanged)
{
    if (ws < 1 || ws > WORKSPACE_COUNT || ws == current_workspace)
        return;
    hideWorkspace(display, current_workspace);
    current_workspace = ws;
    showWorkspace(display, current_workspace);
    if (onWorkspaceChanged)
        onWorkspaceChanged();
}

void ClientManager::moveToWorkspace(Display *display, Window root, int ws, std::function<void()> onWorkspaceChanged)
{
    if (ws < 1 || ws > WORKSPACE_COUNT)
        return;
    Window f = 0;
    int r = 0;
    XGetInputFocus(display, &f, &r);
    if (f == None || f == root)
        return;
    Client *c = findClient(f);
    if (!c)
        return;
    int old = c->workspace;
    c->workspace = ws;
    if (old != current_workspace && ws == current_workspace)
        XMapWindow(display, c->window);
    else if (old == current_workspace && ws != current_workspace)
        XUnmapWindow(display, c->window);
    if (ws == current_workspace)
        showWorkspace(display, current_workspace);
    if (onWorkspaceChanged)
        onWorkspaceChanged();
}

void ClientManager::showWorkspace(Display *display, int ws)
{
    for (auto &c : tiledClients)
        if (c.workspace == ws)
            XMapWindow(display, c.window);
    for (auto &c : floatingClients)
        if (c.workspace == ws)
            XMapWindow(display, c.window);
}

void ClientManager::hideWorkspace(Display *display, int ws)
{
    for (auto &c : tiledClients)
        if (c.workspace == ws)
            XUnmapWindow(display, c.window);
    for (auto &c : floatingClients)
        if (c.workspace == ws)
            XUnmapWindow(display, c.window);
}

void ClientManager::switchTileWinToFloating(Display *display, Window w, std::function<void()> onTileChanged)
{
    Client *c = findClient(w);
    if (!c || c->mode != MODE_TILED)
        return;
    updateClientGeometry(display, *c);
    Client saved = *c;
    removeClient(w);
    saved.mode = MODE_FLOATING;
    floatingClients.push_back(saved);
    if (onTileChanged)
        onTileChanged();
}

void ClientManager::updateClientGeometry(Display *display, Client &c)
{
    XWindowAttributes a{};
    if (XGetWindowAttributes(display, c.window, &a))
    {
        c.x = a.x;
        c.y = a.y;
        c.width = a.width;
        c.height = a.height;
    }
}
