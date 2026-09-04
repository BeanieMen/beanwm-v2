#include "layout_manager.h"
#include "config_manager.h"
#include "helpers/layout.h"

void LayoutManager::tile(Display *display, Window root, ClientManager &clientManager,
                          const std::vector<ScreenInfo> &screens)
{
    clientManager.updateClientNumbers();
    int gap = ConfigManager::instance().get().gap;

    // GLOBAL workspace: one number shared by all screens.
    // Fetch once so every screen tiles the same workspace atomically.
    int ws = clientManager.getCurrentWorkspace();

    for (const auto &screen : screens)
    {
        ScreenStruts struts = getScreenStruts(display, root, screen);
        dwindleTile(display, clientManager.getTiledClients(), gap, screen.screenIndex, ws, screen, struts);

        for (auto &c : clientManager.getFloatingClients())
        {
            if (c.screenIndex != screen.screenIndex) continue;
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
}
