#include "layout_manager.h"
#include "config_manager.h"
#include "helpers/layout.h"

void LayoutManager::tile(Display *display, Window root, ClientManager &clientManager)
{
    clientManager.updateClientNumbers();
    int ws             = clientManager.getCurrentWorkspace();
    int gap            = ConfigManager::instance().get().gap;
    ScreenStruts struts = getScreenStruts(display, root);

    dwindleTile(display, clientManager.getTiledClients(), gap, ws, struts);

    for (auto &c : clientManager.getFloatingClients())
    {
        if (c.workspace == ws)
        {
            XMoveResizeWindow(display, c.window, c.x, c.y, c.width, c.height);
            XRaiseWindow(display, c.window);
            XMapWindow(display, c.window);
        }
        else
            XUnmapWindow(display, c.window);
    }
}
