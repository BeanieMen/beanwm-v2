#include "layout_manager.h"
#include "layout.h"

void LayoutManager::tile(Display *display, ClientManager &clientManager)
{
    clientManager.updateClientNumbers();
    int current_workspace = clientManager.getCurrentWorkspace();
    dwindleTile(display, clientManager.getTiledClients(), gap, current_workspace);

    for (auto &c : clientManager.getFloatingClients())
    {
        if (c.workspace == current_workspace)
        {
            XMoveResizeWindow(display, c.window, c.x, c.y, c.width, c.height);
            XRaiseWindow(display, c.window);
            XMapWindow(display, c.window);
        }
        else
            XUnmapWindow(display, c.window);
    }
}
