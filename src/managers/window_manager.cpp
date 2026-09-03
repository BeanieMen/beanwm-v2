#include "window_manager.h"
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <utility>

static bool wm_detected = false;
static int detectError(Display *, XErrorEvent *e)
{
    if (e->error_code == BadAccess)
        wm_detected = true;
    return 0;
}

WindowManager::WindowManager() : display(nullptr), root(0)
{
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);
    display = XOpenDisplay(nullptr);
    if (!display)
    {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }
    XSetErrorHandler(handleXError);
    root = DefaultRootWindow(display);
    setup();
}

WindowManager::~WindowManager()
{
    if (display)
        XCloseDisplay(display);
}

void WindowManager::run()
{
    while (true)
    {
        XNextEvent(display, &event);
        handleEvent();
    }
}

void WindowManager::setup()
{
    clientManager.setCurrentWorkspace(1);
    XSetErrorHandler(detectError);
    XSelectInput(display, root, SubstructureRedirectMask | SubstructureNotifyMask);
    XSync(display, False);
    XSetErrorHandler(handleXError);
    if (wm_detected)
    {
        fprintf(stderr, "Another WM is running on :%s\n", XDisplayString(display));
        exit(1);
    }
    XSelectInput(display, root, EnterWindowMask | SubstructureRedirectMask | SubstructureNotifyMask | KeyPressMask);

    Window parent;
    Window *children = nullptr;
    unsigned int n = 0;
    if (XQueryTree(display, root, &root, &parent, &children, &n))
    {
        for (unsigned int i = 0; i < n; ++i)
        {
            XWindowAttributes attributes{};
            if (!XGetWindowAttributes(display, children[i], &attributes) || attributes.override_redirect)
                continue;
            clientManager.addClient(children[i], MODE_TILED);
            XSelectInput(display, children[i], EnterWindowMask);
        }
        if (children)
            XFree(children);
    }
    XSetInputFocus(display, root, RevertToPointerRoot, CurrentTime);
    tile();
    keybindingManager.setupKeybindings();
    keybindingManager.grabKeys(display, root);

    XUngrabButton(display, AnyButton, AnyModifier, root);
    auto grabBtn = [&](unsigned int mod)
    {
        XGrabButton(display, Button1, mod, root, False, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
        XGrabButton(display, Button1, mod | LockMask, root, False, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
        XGrabButton(display, Button1, mod | Mod2Mask, root, False, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
        XGrabButton(display, Button1, mod | LockMask | Mod2Mask, root, False, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
    };
    grabBtn(MODKEY);
    XSync(display, False);
}

void WindowManager::tile()
{
    layoutManager.tile(display, clientManager);
}

void WindowManager::switchWorkspace(int ws)
{
    clientManager.switchWorkspace(display, ws, [this]() {
        tile();
    });
}

void WindowManager::moveToWorkspace(int ws)
{
    clientManager.moveToWorkspace(display, root, ws, [this]() {
        tile();
    });
}

void WindowManager::handleEvent()
{
    switch (event.type)
    {
    case MapRequest:
        handleMapRequest();
        break;
    case ConfigureRequest:
        handleConfigureRequest();
        break;
    case DestroyNotify:
        handleDestroyNotify();
        break;
    case KeyPress:
        handleKeyPress();
        break;
    case EnterNotify:
        handleEnterNotify();
        break;
    case ButtonPress:
        handleButtonPress();
        break;
    case MotionNotify:
        handleMotionNotify();
        break;
    case ButtonRelease:
        handleButtonRelease();
        break;
    }
}

void WindowManager::handleMapRequest()
{
    Window w = event.xmaprequest.window;
    XWindowAttributes attributes{};
    if (XGetWindowAttributes(display, w, &attributes) && attributes.override_redirect)
        return;
    clientManager.addClient(w, MODE_TILED);
    XSelectInput(display, w, EnterWindowMask);
    XMapWindow(display, w);
    XSetInputFocus(display, w, RevertToPointerRoot, CurrentTime);
    tile();
}

void WindowManager::handleConfigureRequest()
{
    XConfigureRequestEvent &request = event.xconfigurerequest;
    Client *client = clientManager.findClient(request.window);
    if (!client)
    {
        XWindowChanges changes{request.x, request.y, request.width, request.height,
                               request.border_width, request.above, request.detail};
        XConfigureWindow(display, request.window, request.value_mask, &changes);
        return;
    }
    if (client->mode == MODE_FLOATING)
    {
        if (request.value_mask & CWX) client->x = request.x;
        if (request.value_mask & CWY) client->y = request.y;
        if (request.value_mask & CWWidth) client->width = request.width;
        if (request.value_mask & CWHeight) client->height = request.height;
        XMoveResizeWindow(display, client->window, client->x, client->y, client->width, client->height);
    }
    else
        tile();
}

void WindowManager::handleDestroyNotify()
{
    if (clientManager.removeClient(event.xdestroywindow.window))
        tile();
}

void WindowManager::handleKeyPress()
{
    keybindingManager.handleKeyPress(display, event, *this);
}

void WindowManager::handleEnterNotify()
{
    Window w = event.xcrossing.window;
    if (w == root || draggedWindow != None)
        return;
    XSetInputFocus(display, w, RevertToPointerRoot, CurrentTime);
}

void WindowManager::handleButtonPress()
{
    if (event.xbutton.button != Button1)
        return;

    Window w = event.xbutton.subwindow;

    if (w == None || w == root)
        return;

    Client *c = clientManager.findClient(w);
    if (!c)
        return;

    draggedWindow = w;
    dragTarget = None;

    dragStartX = event.xbutton.x_root;
    dragStartY = event.xbutton.y_root;

    dragWindowX = c->x;
    dragWindowY = c->y;

    dragIsFloating = (c->mode == MODE_FLOATING);

    XGrabPointer(
        display,
        root,
        False,
        ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
        GrabModeAsync,
        GrabModeAsync,
        None,
        None,
        CurrentTime
    );

    XSetInputFocus(display, w, RevertToPointerRoot, CurrentTime);
    XRaiseWindow(display, w);
}

void WindowManager::handleMotionNotify()
{
    if (draggedWindow == None)
        return;
    Client *c = clientManager.findClient(draggedWindow);
    if (!c)
        return;
    if (dragIsFloating)
    {
        int nx = dragWindowX + event.xmotion.x_root - dragStartX;
        int ny = dragWindowY + event.xmotion.y_root - dragStartY;
        c->x = nx;
        c->y = ny;
        XMoveWindow(display, c->window, c->x, c->y);
        XFlush(display);
    }
    else
    {
        int px = event.xmotion.x_root;
        int py = event.xmotion.y_root;
        int current_workspace = clientManager.getCurrentWorkspace();
        auto &tiledClients = clientManager.getTiledClients();

        Client *hover = nullptr;
        for (auto &client : tiledClients)
        {
            if (client.workspace != current_workspace)
                continue;
            if (px >= client.x && px < client.x + client.width &&
                py >= client.y && py < client.y + client.height)
            {
                hover = &client;
                break;
            }
        }

        if (hover && hover->window != draggedWindow)
        {
            Window targetWin = hover->window;
            int srcIdx = -1;
            int tgtIdx = -1;
            for (size_t i = 0; i < tiledClients.size(); ++i)
            {
                if (tiledClients[i].window == draggedWindow)
                    srcIdx = static_cast<int>(i);
                if (tiledClients[i].window == targetWin)
                    tgtIdx = static_cast<int>(i);
            }

            if (srcIdx != -1 && tgtIdx != -1)
            {
                int srcNum = clientManager.getWindowNumber(draggedWindow);
                int tgtNum = clientManager.getWindowNumber(targetWin);
                fprintf(stderr, "[drag] swap window %d (id 0x%lx) with window %d (id 0x%lx) at %d,%d\n",
                        srcNum, draggedWindow, tgtNum, targetWin, px, py);
                fflush(stderr);

                std::swap(tiledClients[srcIdx], tiledClients[tgtIdx]);
                tile();
                dragTarget = targetWin;
                XSetInputFocus(display, draggedWindow, RevertToPointerRoot, CurrentTime);
            }
        }
    }
}

void WindowManager::handleButtonRelease()
{
    if (event.xbutton.button != Button1 || draggedWindow == None)
        return;

    Client *c = clientManager.findClient(draggedWindow);
    if (c && dragIsFloating)
    {
        clientManager.updateClientGeometry(display, *c);
    }
    else if (c && !dragIsFloating)
    {
        int srcNum = clientManager.getWindowNumber(draggedWindow);
        fprintf(stderr, "[drag] release window %d (id 0x%lx)\n", srcNum, draggedWindow);
        fflush(stderr);
        XSetInputFocus(display, draggedWindow, RevertToPointerRoot, CurrentTime);
        XRaiseWindow(display, draggedWindow);
    }

    XUngrabPointer(display, CurrentTime);
    draggedWindow = None;
    dragTarget = None;
    dragIsFloating = false;
    XFlush(display);
}

int WindowManager::handleXError(Display *d, XErrorEvent *e)
{
    if (e->error_code == BadAccess || e->error_code == BadWindow)
        return 0;
    char buf[256];
    XGetErrorText(d, e->error_code, buf, sizeof(buf));
    fprintf(stderr, "X Error: %s req=%d min=%d res=0x%lx\n", buf, e->request_code, e->minor_code, e->resourceid);
    return 0;
}

Window WindowManager::GetFocusedWindow()
{
    Window f = 0;
    int r = 0;
    XGetInputFocus(display, &f, &r);
    return f;
}
