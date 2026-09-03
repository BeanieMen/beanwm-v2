#include "window_manager.h"
#include "layout.h"
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

WindowManager::WindowManager() : display(nullptr), root(0), tiledClients{}, floatingClients{}
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
    current_workspace = 1;
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
            addClient(children[i], MODE_TILED);
            XSelectInput(display, children[i], EnterWindowMask);
        }
        if (children)
            XFree(children);
    }
    XSetInputFocus(display, root, RevertToPointerRoot, CurrentTime);
    tile();
    setupKeybindings();

    XUngrabKey(display, AnyKey, AnyModifier, root);
    for (auto &b : keybindings)
    {
        KeyCode c = XKeysymToKeycode(display, b.key);
        if (!c)
            continue;
        XGrabKey(display, c, b.modifiers, root, False, GrabModeAsync, GrabModeAsync);
        XGrabKey(display, c, b.modifiers | LockMask, root, False, GrabModeAsync, GrabModeAsync);
        XGrabKey(display, c, b.modifiers | Mod2Mask, root, False, GrabModeAsync, GrabModeAsync);
        XGrabKey(display, c, b.modifiers | LockMask | Mod2Mask, root, False, GrabModeAsync, GrabModeAsync);
    }

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

void WindowManager::addClient(Window w, ManagementMode mode)
{
    if (findClient(w))
        return;
    if (mode == MODE_TILED)
        tiledClients.push_back(Client{w, current_workspace, MODE_TILED, 0, 0, 800, 600, 0});
    else
        floatingClients.push_back(Client{w, current_workspace, MODE_FLOATING, 0, 0, 800, 600, 0});
    updateClientNumbers();
}

bool WindowManager::removeClient(Window w)
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

Client *WindowManager::findClient(Window w)
{
    for (auto &c : tiledClients)
        if (c.window == w)
            return &c;
    for (auto &c : floatingClients)
        if (c.window == w)
            return &c;
    return nullptr;
}

void WindowManager::updateClientNumbers()
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

int WindowManager::getWindowNumber(Window w)
{
    Client *c = findClient(w);
    return c ? c->number : 0;
}

void WindowManager::handleButtonPress()
{
    if (event.xbutton.button != Button1)
        return;

    Window w = event.xbutton.subwindow;

    if (w == None || w == root)
        return;

    Client *c = findClient(w);
    if (!c)
        return;

    // Start dragging
    draggedWindow = w;
    dragTarget = None;

    dragStartX = event.xbutton.x_root;
    dragStartY = event.xbutton.y_root;

    dragWindowX = c->x;
    dragWindowY = c->y;

    dragIsFloating = (c->mode == MODE_FLOATING);

    // Grab pointer so we continue receiving MotionNotify
    // and ButtonRelease even when the pointer leaves the window.
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
    addClient(w, MODE_TILED);
    XSelectInput(display, w, EnterWindowMask);
    XMapWindow(display, w);
    XSetInputFocus(display, w, RevertToPointerRoot, CurrentTime);
    tile();
}

void WindowManager::handleConfigureRequest()
{
    XConfigureRequestEvent &request = event.xconfigurerequest;
    Client *client = findClient(request.window);
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
    if (removeClient(event.xdestroywindow.window))
        tile();
}

void WindowManager::handleEnterNotify()
{
    Window w = event.xcrossing.window;
    if (w == root || draggedWindow != None)
        return;
    XSetInputFocus(display, w, RevertToPointerRoot, CurrentTime);
}

void WindowManager::handleMotionNotify()
{
    if (draggedWindow == None)
        return;
    Client *c = findClient(draggedWindow);
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
                int srcNum = getWindowNumber(draggedWindow);
                int tgtNum = getWindowNumber(targetWin);
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

    Client *c = findClient(draggedWindow);
    if (c && dragIsFloating)
    {
        updateClientGeometry(*c);
    }
    else if (c && !dragIsFloating)
    {
        int srcNum = getWindowNumber(draggedWindow);
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

void WindowManager::spawnTerminal() { spawnProcess(TERMINAL); }
void WindowManager::spawnProcess(const std::string &cmd)
{
    if (fork() == 0)
    {
        execlp(cmd.c_str(), cmd.c_str(), nullptr);
        perror(cmd.c_str());
        _exit(127);
    }
}

void WindowManager::tile()
{
    updateClientNumbers();
    dwindleTile(display, tiledClients, GAP, current_workspace);
    for (auto &c : floatingClients)
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

void WindowManager::switchTileWinToFloating(Window &w)
{
    Client *c = findClient(w);
    if (!c || c->mode != MODE_TILED)
        return;
    updateClientGeometry(*c);
    Client saved = *c;
    removeClient(w);
    saved.mode = MODE_FLOATING;
    floatingClients.push_back(saved);
    tile();
}

void WindowManager::updateClientGeometry(Client &c)
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

void WindowManager::switchWorkspace(int ws)
{
    if (ws < 1 || ws > WORKSPACE_COUNT || ws == current_workspace)
        return;
    hideWorkspace(current_workspace);
    current_workspace = ws;
    showWorkspace(current_workspace);
    tile();
}
void WindowManager::moveToWorkspace(int ws)
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
        showWorkspace(current_workspace);
    tile();
}
void WindowManager::showWorkspace(int ws)
{
    for (auto &c : tiledClients)
        if (c.workspace == ws)
            XMapWindow(display, c.window);
    for (auto &c : floatingClients)
        if (c.workspace == ws)
            XMapWindow(display, c.window);
}
void WindowManager::hideWorkspace(int ws)
{
    for (auto &c : tiledClients)
        if (c.workspace == ws)
            XUnmapWindow(display, c.window);
    for (auto &c : floatingClients)
        if (c.workspace == ws)
            XUnmapWindow(display, c.window);
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
