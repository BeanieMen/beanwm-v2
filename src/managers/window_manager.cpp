#include "window_manager.h"
#include "config_manager.h"
#include "helpers/strut_helper.h"
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <utility>

static bool wm_detected = false;
static int detectError(Display *, XErrorEvent *e)
{
    if (e->error_code == BadAccess) wm_detected = true;
    return 0;
}

WindowManager::WindowManager() : display(nullptr), root(0)
{
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);
    display = XOpenDisplay(nullptr);
    if (!display) { fprintf(stderr, "Cannot open display\n"); exit(1); }
    XSetErrorHandler(handleXError);
    root = DefaultRootWindow(display);
    setup();
}

WindowManager::~WindowManager()
{
    if (display) XCloseDisplay(display);
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
    ConfigManager::instance().load();

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

    XSelectInput(display, root,
        EnterWindowMask | SubstructureRedirectMask | SubstructureNotifyMask |
        KeyPressMask | PropertyChangeMask | ExposureMask);

    Window parent;
    Window *children = nullptr;
    unsigned int n   = 0;
    if (XQueryTree(display, root, &root, &parent, &children, &n))
    {
        for (unsigned int i = 0; i < n; ++i)
        {
            if (isDockWindow(display, children[i])) continue;
            XWindowAttributes a{};
            if (!XGetWindowAttributes(display, children[i], &a)) continue;
            clientManager.addClient(children[i], MODE_TILED);
            XSelectInput(display, children[i], EnterWindowMask);
        }
        if (children) XFree(children);
    }

    XSetInputFocus(display, root, RevertToPointerRoot, CurrentTime);
    tile();

    keybindingManager.setupKeybindings();
    keybindingManager.grabKeys(display, root);

    for (const auto &cmd : ConfigManager::instance().get().autostart)
    {
        processManager.spawnProcess(cmd);
    }

    unsigned int modKey = ConfigManager::instance().get().modKey;
    XUngrabButton(display, AnyButton, AnyModifier, root);
    auto grabBtn = [&](unsigned int mod)
    {
        XGrabButton(display, Button1, mod, root, False, ButtonPressMask,
                    GrabModeAsync, GrabModeAsync, None, None);
        XGrabButton(display, Button1, mod | LockMask, root, False, ButtonPressMask,
                    GrabModeAsync, GrabModeAsync, None, None);
        XGrabButton(display, Button1, mod | Mod2Mask, root, False, ButtonPressMask,
                    GrabModeAsync, GrabModeAsync, None, None);
        XGrabButton(display, Button1, mod | LockMask | Mod2Mask, root, False,
                    ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
    };
    grabBtn(modKey);
    XSync(display, False);
}

void WindowManager::tile()
{
    XClearWindow(display, root);
    layoutManager.tile(display, root, clientManager);
    XFlush(display);
}

void WindowManager::switchWorkspace(int ws)
{
    clientManager.switchWorkspace(display, ws, [this]() { tile(); });
}

void WindowManager::moveToWorkspace(int ws)
{
    clientManager.moveToWorkspace(display, root, ws, [this]() { tile(); });
}

void WindowManager::handleEvent()
{
    switch (event.type)
    {
    case MapRequest:       handleMapRequest();       break;
    case ConfigureRequest: handleConfigureRequest(); break;
    case DestroyNotify:    handleDestroyNotify();    break;
    case KeyPress:         handleKeyPress();         break;
    case EnterNotify:      handleEnterNotify();      break;
    case ButtonPress:      handleButtonPress();      break;
    case MotionNotify:     handleMotionNotify();     break;
    case ButtonRelease:    handleButtonRelease();    break;
    case PropertyNotify:   handlePropertyNotify();   break;
    case Expose:           handleExpose();           break;
    }
}

void WindowManager::handleMapRequest()
{
    Window w = event.xmaprequest.window;
    if (isDockWindow(display, w))
    {
        XMapWindow(display, w);
        tile();
        return;
    }
    clientManager.addClient(w, MODE_TILED);
    XSelectInput(display, w, EnterWindowMask);
    XMapWindow(display, w);
    XSetInputFocus(display, w, RevertToPointerRoot, CurrentTime);
    tile();
}

void WindowManager::handleConfigureRequest()
{
    XConfigureRequestEvent &req = event.xconfigurerequest;
    Client *c = clientManager.findClient(req.window);
    if (!c)
    {
        XWindowChanges changes{req.x, req.y, req.width, req.height,
                               req.border_width, req.above, req.detail};
        XConfigureWindow(display, req.window, req.value_mask, &changes);
        return;
    }
    if (c->mode == MODE_FLOATING)
    {
        if (req.value_mask & CWX)      c->x      = req.x;
        if (req.value_mask & CWY)      c->y      = req.y;
        if (req.value_mask & CWWidth)  c->width  = req.width;
        if (req.value_mask & CWHeight) c->height = req.height;
        XMoveResizeWindow(display, c->window, c->x, c->y, c->width, c->height);
    }
    else
        tile();
}

void WindowManager::handleDestroyNotify()
{
    if (clientManager.removeClient(event.xdestroywindow.window))
    {
        tile();
        Window top = clientManager.getTopClientWindow();
        if (top != None)
            XSetInputFocus(display, top, RevertToPointerRoot, CurrentTime);
        else
            XSetInputFocus(display, root, RevertToPointerRoot, CurrentTime);
    }
}

void WindowManager::handleKeyPress()
{
    keybindingManager.handleKeyPress(display, event, *this);
}

void WindowManager::handleEnterNotify()
{
    Window w = event.xcrossing.window;
    if (w == root || draggedWindow != None) return;
    XSetInputFocus(display, w, RevertToPointerRoot, CurrentTime);
}

void WindowManager::handleButtonPress()
{
    if (event.xbutton.button != Button1) return;
    Window w = event.xbutton.subwindow;
    if (w == None || w == root) return;
    Client *c = clientManager.findClient(w);
    if (!c) return;

    draggedWindow  = w;
    dragTarget     = None;
    dragStartX     = event.xbutton.x_root;
    dragStartY     = event.xbutton.y_root;
    dragWindowX    = c->x;
    dragWindowY    = c->y;
    dragIsFloating = (c->mode == MODE_FLOATING);

    XGrabPointer(display, root, False,
        ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
        GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
    XSetInputFocus(display, w, RevertToPointerRoot, CurrentTime);
    XRaiseWindow(display, w);
}

void WindowManager::handleMotionNotify()
{
    if (draggedWindow == None) return;
    Client *c = clientManager.findClient(draggedWindow);
    if (!c) return;

    if (dragIsFloating)
    {
        c->x = dragWindowX + event.xmotion.x_root - dragStartX;
        c->y = dragWindowY + event.xmotion.y_root - dragStartY;
        XMoveWindow(display, c->window, c->x, c->y);
        XClearWindow(display, root);
        XFlush(display);
        return;
    }

    int px = event.xmotion.x_root;
    int py = event.xmotion.y_root;
    int ws = clientManager.getCurrentWorkspace();
    auto &tiled = clientManager.getTiledClients();

    Client *hover = nullptr;
    for (auto &cl : tiled)
    {
        if (cl.workspace != ws) continue;
        if (px >= cl.x && px < cl.x + cl.width &&
            py >= cl.y && py < cl.y + cl.height)
        {
            hover = &cl;
            break;
        }
    }

    if (!hover || hover->window == draggedWindow) return;

    int srcIdx = -1, tgtIdx = -1;
    for (size_t i = 0; i < tiled.size(); ++i)
    {
        if (tiled[i].window == draggedWindow) srcIdx = static_cast<int>(i);
        if (tiled[i].window == hover->window)  tgtIdx = static_cast<int>(i);
    }
    if (srcIdx == -1 || tgtIdx == -1) return;

    std::swap(tiled[srcIdx], tiled[tgtIdx]);
    tile();
    dragTarget = hover->window;
    XSetInputFocus(display, draggedWindow, RevertToPointerRoot, CurrentTime);
}

void WindowManager::handleButtonRelease()
{
    if (event.xbutton.button != Button1 || draggedWindow == None) return;
    Client *c = clientManager.findClient(draggedWindow);
    if (c && dragIsFloating)
        clientManager.updateClientGeometry(display, *c);
    else if (c)
    {
        XSetInputFocus(display, draggedWindow, RevertToPointerRoot, CurrentTime);
        XRaiseWindow(display, draggedWindow);
    }
    XUngrabPointer(display, CurrentTime);
    draggedWindow  = None;
    dragTarget     = None;
    dragIsFloating = false;
    XFlush(display);
}

void WindowManager::handlePropertyNotify()
{
    Atom a = event.xproperty.atom;
    if (a == XInternAtom(display, "_NET_WM_STRUT_PARTIAL", False) ||
        a == XInternAtom(display, "_NET_WM_STRUT", False))
        tile();
}

void WindowManager::handleExpose()
{
    if (event.xexpose.count == 0)
    {
        XClearWindow(display, root);
        XFlush(display);
    }
}

int WindowManager::handleXError(Display *d, XErrorEvent *e)
{
    if (e->error_code == BadAccess || e->error_code == BadWindow) return 0;
    char buf[256];
    XGetErrorText(d, e->error_code, buf, sizeof(buf));
    fprintf(stderr, "X Error: %s req=%d min=%d res=0x%lx\n",
            buf, e->request_code, e->minor_code, e->resourceid);
    return 0;
}

Window WindowManager::GetFocusedWindow()
{
    Window f = 0;
    int r    = 0;
    XGetInputFocus(display, &f, &r);
    return f;
}
