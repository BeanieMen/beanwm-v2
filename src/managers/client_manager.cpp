#include "client_manager.h"
#include "config_manager.h"

void ClientManager::addClient(Window w, int screenIndex, ManagementMode mode)
{
    if (findClient(w)) return;
    int ws = getCurrentWorkspace(screenIndex);
    if (mode == MODE_TILED)
        tiledClients.push_back(Client{w, screenIndex, ws, MODE_TILED, 0, 0, 800, 600, 0});
    else
        floatingClients.push_back(Client{w, screenIndex, ws, MODE_FLOATING, 0, 0, 800, 600, 0});
    updateClientNumbers();
}

void ClientManager::addClient(Window w, int screenIndex, int workspace, ManagementMode mode)
{
    if (findClient(w)) return;
    if (mode == MODE_TILED)
        tiledClients.push_back(Client{w, screenIndex, workspace, MODE_TILED, 0, 0, 800, 600, 0});
    else
        floatingClients.push_back(Client{w, screenIndex, workspace, MODE_FLOATING, 0, 0, 800, 600, 0});
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
    for (auto &c : tiledClients)    if (c.window == w) return &c;
    for (auto &c : floatingClients) if (c.window == w) return &c;
    return nullptr;
}

void ClientManager::updateClientNumbers()
{
    // Per-screen, per-workspace numbering
    std::map<std::pair<int,int>, int> counters;
    for (auto &c : tiledClients)
    {
        int ws  = getCurrentWorkspace(c.screenIndex);
        auto key = std::make_pair(c.screenIndex, c.workspace);
        c.number = (c.workspace == ws) ? ++counters[key] : 0;
    }
    for (auto &c : floatingClients)
    {
        int ws  = getCurrentWorkspace(c.screenIndex);
        auto key = std::make_pair(c.screenIndex, c.workspace);
        c.number = (c.workspace == ws) ? ++counters[key] : 0;
    }
}

int ClientManager::getWindowNumber(Window w)
{
    Client *c = findClient(w);
    return c ? c->number : 0;
}

void ClientManager::switchWorkspace(Display *display, int screenIndex, int ws,
                                     std::function<void()> onWorkspaceChanged)
{
    int count = ConfigManager::instance().get().workspaceCount;
    if (ws < 1 || ws > count || ws == getCurrentWorkspace(screenIndex)) return;
    hideWorkspace(display, screenIndex, getCurrentWorkspace(screenIndex));
    setCurrentWorkspace(screenIndex, ws);
    showWorkspace(display, screenIndex, ws);
    if (onWorkspaceChanged) onWorkspaceChanged();
}

void ClientManager::moveToWorkspace(Display *display, Window root, int screenIndex, int ws,
                                     std::function<void()> onWorkspaceChanged)
{
    int count = ConfigManager::instance().get().workspaceCount;
    if (ws < 1 || ws > count) return;
    Window f = 0;
    int r = 0;
    XGetInputFocus(display, &f, &r);
    if (f == None || f == root) return;
    Client *c = findClient(f);
    if (!c) return;
    int oldWs  = c->workspace;
    int curWs  = getCurrentWorkspace(screenIndex);
    c->workspace = ws;
    if (oldWs != curWs && ws == curWs)
        XMapWindow(display, c->window);
    else if (oldWs == curWs && ws != curWs)
        XUnmapWindow(display, c->window);
    if (onWorkspaceChanged) onWorkspaceChanged();
}

void ClientManager::showWorkspace(Display *display, int screenIndex, int ws)
{
    for (auto &c : tiledClients)
        if (c.screenIndex == screenIndex && c.workspace == ws)
            XMapWindow(display, c.window);
    for (auto &c : floatingClients)
        if (c.screenIndex == screenIndex && c.workspace == ws)
            XMapWindow(display, c.window);
}

void ClientManager::hideWorkspace(Display *display, int screenIndex, int ws)
{
    for (auto &c : tiledClients)
        if (c.screenIndex == screenIndex && c.workspace == ws)
            XUnmapWindow(display, c.window);
    for (auto &c : floatingClients)
        if (c.screenIndex == screenIndex && c.workspace == ws)
            XUnmapWindow(display, c.window);
}

void ClientManager::switchTileWinToFloating(Display *display, Window w,
                                             std::function<void()> onTileChanged)
{
    Client *c = findClient(w);
    if (!c || c->mode != MODE_TILED) return;
    updateClientGeometry(display, *c);
    Client saved = *c;
    removeClient(w);
    saved.mode = MODE_FLOATING;
    floatingClients.push_back(saved);
    if (onTileChanged) onTileChanged();
}

void ClientManager::updateClientGeometry(Display *display, Client &c)
{
    XWindowAttributes a{};
    if (XGetWindowAttributes(display, c.window, &a))
    {
        c.x = a.x;
        c.y = a.y;
        c.width  = a.width;
        c.height = a.height;
    }
}

Window ClientManager::getTopClientWindow(int screenIndex) const
{
    int ws = getCurrentWorkspace(screenIndex);
    for (const auto &c : tiledClients)
        if (c.screenIndex == screenIndex && c.workspace == ws) return c.window;
    for (const auto &c : floatingClients)
        if (c.screenIndex == screenIndex && c.workspace == ws) return c.window;
    return None;
}
