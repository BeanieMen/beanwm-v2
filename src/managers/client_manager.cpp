#include "client_manager.h"
#include "config_manager.h"
#include <X11/Xlib.h>

void ClientManager::addClient(Window w, int screenIndex, ManagementMode mode)
{
    if (findClient(w)) return;
    int ws = getCurrentWorkspace();
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
    for (size_t i = 0; i < tiledClients.size(); ++i)
    {
        if (tiledClients[i].window == w)
        {
            tiledClients.erase(tiledClients.begin() + (long)i);
            updateClientNumbers();
            return true;
        }
    }
    for (size_t i = 0; i < floatingClients.size(); ++i)
    {
        if (floatingClients[i].window == w)
        {
            floatingClients.erase(floatingClients.begin() + (long)i);
            updateClientNumbers();
            return true;
        }
    }
    return false;
}

Client *ClientManager::findClient(Window w)
{
    for (size_t i = 0; i < tiledClients.size(); ++i)
        if (tiledClients[i].window == w) return &tiledClients[i];
    for (size_t i = 0; i < floatingClients.size(); ++i)
        if (floatingClients[i].window == w) return &floatingClients[i];
    return nullptr;
}

void ClientManager::updateClientNumbers()
{
    // Single global counter: numbers unique across tiled + floating on the visible ws
    int globalWs = getCurrentWorkspace();
    int n = 0;
    for (size_t i = 0; i < tiledClients.size(); ++i)
    {
        if (tiledClients[i].workspace == globalWs)
            tiledClients[i].number = ++n;
        else
            tiledClients[i].number = 0;
    }
    for (size_t i = 0; i < floatingClients.size(); ++i)
    {
        if (floatingClients[i].workspace == globalWs)
            floatingClients[i].number = ++n;
        else
            floatingClients[i].number = 0;
    }
}

int ClientManager::getWindowNumber(Window w)
{
    Client *c = findClient(w);
    return c ? c->number : 0;
}

void ClientManager::switchWorkspace(Display *display, int ws)
{
    int count = ConfigManager::instance().get().workspaceCount;
    int cur = getCurrentWorkspace();
    if (ws < 1 || ws > count || ws == cur) return;
    hideWorkspace(display, cur);
    setCurrentWorkspace(ws);
    showWorkspace(display, ws);
}

void ClientManager::moveToWorkspace(Display *display, Window root, int ws)
{
    int count = ConfigManager::instance().get().workspaceCount;
    if (ws < 1 || ws > count) return;
    Window f = 0;
    int r = 0;
    XGetInputFocus(display, &f, &r);
    if (f == None || f == root) return;
    Client *c = findClient(f);
    if (!c) return;
    int oldWs = c->workspace;
    int curWs = getCurrentWorkspace();
    c->workspace = ws;
    if (oldWs != curWs && ws == curWs)
        XMapWindow(display, c->window);
    else if (oldWs == curWs && ws != curWs)
        XUnmapWindow(display, c->window);
}

void ClientManager::showWorkspace(Display *display, int ws)
{
    for (size_t i = 0; i < tiledClients.size(); ++i)
        if (tiledClients[i].workspace == ws)
            XMapWindow(display, tiledClients[i].window);
    for (size_t i = 0; i < floatingClients.size(); ++i)
        if (floatingClients[i].workspace == ws)
            XMapWindow(display, floatingClients[i].window);
}

void ClientManager::hideWorkspace(Display *display, int ws)
{
    for (size_t i = 0; i < tiledClients.size(); ++i)
        if (tiledClients[i].workspace == ws)
            XUnmapWindow(display, tiledClients[i].window);
    for (size_t i = 0; i < floatingClients.size(); ++i)
        if (floatingClients[i].workspace == ws)
            XUnmapWindow(display, floatingClients[i].window);
}

void ClientManager::switchTileWinToFloating(Display *display, Window w)
{
    Client *c = findClient(w);
    if (!c || c->mode != MODE_TILED) return;
    updateClientGeometry(display, *c);
    Client saved = *c;
    removeClient(w);
    saved.mode = MODE_FLOATING;
    floatingClients.push_back(saved);
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

Window ClientManager::getTopClientWindow() const
{
    int ws = getCurrentWorkspace();
    for (size_t i = 0; i < tiledClients.size(); ++i)
        if (tiledClients[i].workspace == ws) return tiledClients[i].window;
    for (size_t i = 0; i < floatingClients.size(); ++i)
        if (floatingClients[i].workspace == ws) return floatingClients[i].window;
    return None;
}
