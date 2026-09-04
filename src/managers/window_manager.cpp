#include "window_manager.h"
#include "config_manager.h"
#include "helpers/strut_helper.h"
#include "helpers/string_helper.h"
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <utility>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cctype>
#include <algorithm>
#include <X11/extensions/Xinerama.h>

// ── Error handling ─────────────────────────────────────────────────────────

static bool wm_detected = false;
static int detectError(Display *, XErrorEvent *e)
{
    if (e->error_code == BadAccess) wm_detected = true;
    return 0;
}

// ── State file path ────────────────────────────────────────────────────────

static std::string getStateFilePath(Display *display)
{
    std::string disp = XDisplayString(display);
    for (char &c : disp)
        if (!isalnum(static_cast<unsigned char>(c))) c = '_';
    return "/tmp/.beanwm_state_" + disp;
}

// ── Constructor / Destructor ───────────────────────────────────────────────

WindowManager::WindowManager() : display(nullptr), root(0)
{
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);
    display = XOpenDisplay(nullptr);
    if (!display) { fprintf(stderr, "Cannot open display\n"); exit(1); }
    XSetErrorHandler(handleXError);
    root = DefaultRootWindow(display);
    detectScreens();
    setup();
}

WindowManager::~WindowManager()
{
    if (display) XCloseDisplay(display);
}

// ── Main loop ─────────────────────────────────────────────────────────────

void WindowManager::run()
{
    while (true)
    {
        XNextEvent(display, &event);
        handleEvent();
    }
}

// ── Screen detection via Xinerama ─────────────────────────────────────────

void WindowManager::detectScreens()
{
    screens.clear();

    int xineramaEvent, xineramaError;
    if (XineramaIsActive(display))
    {
        int count = 0;
        XineramaScreenInfo *info = XineramaQueryScreens(display, &count);
        if (info)
        {
            for (int i = 0; i < count; ++i)
            {
                ScreenInfo s;
                s.screenIndex = i;
                s.x      = info[i].x_org;
                s.y      = info[i].y_org;
                s.width  = info[i].width;
                s.height = info[i].height;
                screens.push_back(s);
                fprintf(stderr, "[beanwm] Screen %d: %dx%d+%d+%d\n",
                        i, s.width, s.height, s.x, s.y);
            }
            XFree(info);
        }
    }
    (void)xineramaEvent; (void)xineramaError;

    // Fallback: single screen from X11
    if (screens.empty())
    {
        ScreenInfo s;
        s.screenIndex = 0;
        s.x = 0; s.y = 0;
        s.width  = DisplayWidth(display, DefaultScreen(display));
        s.height = DisplayHeight(display, DefaultScreen(display));
        screens.push_back(s);
        fprintf(stderr, "[beanwm] Single screen: %dx%d\n", s.width, s.height);
    }

    currentScreenIndex = 0;

    // Ensure every detected screen has a default workspace entry
    for (const auto &s : screens)
        if (clientManager.getCurrentWorkspace(s.screenIndex) < 1)
            clientManager.setCurrentWorkspace(s.screenIndex, 1);
}

// ── Screen helpers ────────────────────────────────────────────────────────

int WindowManager::screenIndexForPoint(int x, int y) const
{
    for (const auto &s : screens)
        if (x >= s.x && x < s.x + s.width &&
            y >= s.y && y < s.y + s.height)
            return s.screenIndex;
    return 0; // fallback
}

const ScreenInfo *WindowManager::getScreenAt(int x, int y) const
{
    for (const auto &s : screens)
        if (x >= s.x && x < s.x + s.width &&
            y >= s.y && y < s.y + s.height)
            return &s;
    return screens.empty() ? nullptr : &screens[0];
}

const ScreenInfo *WindowManager::screenForWindow(Window w) const
{
    const Client *c = nullptr;
    for (const auto &cl : clientManager.getTiledClients())
        if (cl.window == w) { c = &cl; break; }
    if (!c)
        for (const auto &cl : clientManager.getFloatingClients())
            if (cl.window == w) { c = &cl; break; }
    if (c)
        for (const auto &s : screens)
            if (s.screenIndex == c->screenIndex) return &s;
    return screens.empty() ? nullptr : &screens[0];
}

// ── State persistence ─────────────────────────────────────────────────────

void WindowManager::saveState()
{
    std::string path = getStateFilePath(display);
    std::ofstream out(path);
    if (!out.is_open())
    {
        fprintf(stderr, "[beanwm] Could not save state to %s\n", path.c_str());
        return;
    }

    // Save per-screen active workspaces
    for (const auto &s : screens)
        out << "SCREEN_WS " << s.screenIndex << " " << clientManager.getCurrentWorkspace(s.screenIndex) << "\n";

    for (const auto &c : clientManager.getTiledClients())
        out << "TILED " << c.window << " " << c.screenIndex << " " << c.workspace << "\n";

    for (const auto &c : clientManager.getFloatingClients())
        out << "FLOATING " << c.window << " " << c.screenIndex << " " << c.workspace << " "
            << c.x << " " << c.y << " " << c.width << " " << c.height << "\n";

    out.close();
    fprintf(stderr, "[beanwm] Saved state to %s\n", path.c_str());
}

bool WindowManager::restoreState()
{
    std::string path = getStateFilePath(display);
    if (!std::filesystem::exists(path)) return false;

    std::ifstream in(path);
    if (!in.is_open()) return false;

    fprintf(stderr, "[beanwm] Restoring state from %s...\n", path.c_str());

    std::string line;
    while (std::getline(in, line))
    {
        line = trim(line);
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string type;
        ss >> type;

        if (type == "SCREEN_WS")
        {
            int screenIdx = 0, ws = 1;
            if (ss >> screenIdx >> ws)
                clientManager.setCurrentWorkspace(screenIdx, ws);
        }
        else if (type == "TILED")
        {
            Window w = 0;
            int screenIdx = 0, ws = 1;
            if (ss >> w >> screenIdx >> ws)
            {
                XWindowAttributes a{};
                if (XGetWindowAttributes(display, w, &a))
                {
                    clientManager.getTiledClients().push_back(
                        Client{w, screenIdx, ws, MODE_TILED, a.x, a.y, a.width, a.height, 0});
                    XSelectInput(display, w, EnterWindowMask);
                }
            }
        }
        else if (type == "FLOATING")
        {
            Window w = 0;
            int screenIdx = 0, ws = 1, x = 0, y = 0, width = 800, height = 600;
            if (ss >> w >> screenIdx >> ws >> x >> y >> width >> height)
            {
                XWindowAttributes a{};
                if (XGetWindowAttributes(display, w, &a))
                {
                    clientManager.getFloatingClients().push_back(
                        Client{w, screenIdx, ws, MODE_FLOATING, x, y, width, height, 0});
                    XSelectInput(display, w, EnterWindowMask);
                }
            }
        }
    }

    in.close();
    std::filesystem::remove(path);
    return true;
}

// ── Rebuild & Reload ──────────────────────────────────────────────────────

void WindowManager::rebuildAndReload()
{
    fprintf(stderr, "[beanwm] Rebuild & reload requested...\n");

    std::string sourceDir;
    std::string cwd = std::filesystem::current_path().string();

    if (std::filesystem::exists(cwd + "/Makefile"))
        sourceDir = cwd;
    else if (std::filesystem::exists("/home/beanie/beanwm-v2/Makefile"))
        sourceDir = "/home/beanie/beanwm-v2";
    else
    {
        char selfBuf[1024] = {0};
        ssize_t len = readlink("/proc/self/exe", selfBuf, sizeof(selfBuf) - 1);
        if (len > 0)
        {
            std::filesystem::path p(selfBuf);
            sourceDir = (p.parent_path().filename() == "build")
                ? p.parent_path().parent_path().string()
                : p.parent_path().string();
        }
    }

    if (sourceDir.empty() || !std::filesystem::exists(sourceDir + "/Makefile"))
    {
        fprintf(stderr, "[beanwm] Error: Cannot locate Makefile\n");
        return;
    }

    std::string buildCmd = "make -C \"" + sourceDir + "\" release";
    int ret = system(buildCmd.c_str());
    if (ret != 0)
    {
        fprintf(stderr, "[beanwm] Rebuild failed (code %d). Reload aborted.\n", ret);
        return;
    }

    fprintf(stderr, "[beanwm] Rebuild successful! Preserving state & reloading...\n");
    saveState();

    std::string targetBin = sourceDir + "/build/beanwm";
    if (!std::filesystem::exists(targetBin))
        targetBin = "/usr/bin/beanwm";

    char *args[] = { const_cast<char*>(targetBin.c_str()), nullptr };
    execv(targetBin.c_str(), args);
    fprintf(stderr, "[beanwm] Error: execv failed for %s\n", targetBin.c_str());
}

// ── quit ──────────────────────────────────────────────────────────────────

void WindowManager::quit()
{
    processManager.terminateAll();
    XCloseDisplay(display);
    display = nullptr;
    std::exit(0);
}

// ── reloadConfig ──────────────────────────────────────────────────────────

void WindowManager::reloadConfig()
{
    ConfigManager::instance().load();
    keybindingManager.setupKeybindings();
    keybindingManager.grabKeys(display, root);

    unsigned int modKey = ConfigManager::instance().get().modKey;
    XUngrabButton(display, AnyButton, AnyModifier, root);
    XGrabButton(display, Button1, modKey, root, False, ButtonPressMask,
                GrabModeAsync, GrabModeAsync, None, None);
    XGrabButton(display, Button1, modKey | LockMask, root, False, ButtonPressMask,
                GrabModeAsync, GrabModeAsync, None, None);
    XGrabButton(display, Button1, modKey | Mod2Mask, root, False, ButtonPressMask,
                GrabModeAsync, GrabModeAsync, None, None);
    XGrabButton(display, Button1, modKey | LockMask | Mod2Mask, root, False,
                ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
    tile();
    fprintf(stderr, "[beanwm] Configuration reloaded\n");
}

// ── Setup ──────────────────────────────────────────────────────────────────

void WindowManager::setup()
{
    ConfigManager::instance().load();

    // Initialise workspace 1 for every screen
    for (const auto &s : screens)
        clientManager.setCurrentWorkspace(s.screenIndex, 1);

    XSetErrorHandler(detectError);
    XSelectInput(display, root, SubstructureRedirectMask | SubstructureNotifyMask);
    XSync(display, False);
    XSetErrorHandler(handleXError);
    if (wm_detected)
    {
        fprintf(stderr, "Another WM is running on :%s\n", XDisplayString(display));
        exit(1);
    }

    XSetWindowBackground(display, root, BlackPixel(display, DefaultScreen(display)));
    XClearWindow(display, root);

    XSelectInput(display, root,
        EnterWindowMask | SubstructureRedirectMask | SubstructureNotifyMask |
        KeyPressMask | PropertyChangeMask | ExposureMask);

    bool restored = restoreState();

    // Adopt existing windows, placing them on the screen they overlap most
    Window parent;
    Window *children = nullptr;
    unsigned int n = 0;
    if (XQueryTree(display, root, &root, &parent, &children, &n))
    {
        for (unsigned int i = 0; i < n; ++i)
        {
            if (isDockWindow(display, children[i])) continue;
            XWindowAttributes a{};
            if (!XGetWindowAttributes(display, children[i], &a)) continue;
            if (a.override_redirect || a.map_state != IsViewable) continue;
            if (restored && clientManager.findClient(children[i])) continue;

            // Place on whichever screen the window's centre belongs to
            int cx = a.x + a.width / 2;
            int cy = a.y + a.height / 2;
            int si = screenIndexForPoint(cx, cy);
            clientManager.addClient(children[i], si, MODE_TILED);
            XSelectInput(display, children[i], EnterWindowMask);
        }
        if (children) XFree(children);
    }

    // Show/hide workspaces per screen
    int wsCount = ConfigManager::instance().get().workspaceCount;
    for (const auto &s : screens)
    {
        int curWs = clientManager.getCurrentWorkspace(s.screenIndex);
        for (int ws = 1; ws <= wsCount; ++ws)
        {
            if (ws == curWs)
                clientManager.showWorkspace(display, s.screenIndex, ws);
            else
                clientManager.hideWorkspace(display, s.screenIndex, ws);
        }
    }

    XSetInputFocus(display, root, RevertToPointerRoot, CurrentTime);
    tile();

    keybindingManager.setupKeybindings();
    keybindingManager.grabKeys(display, root);

    if (!restored)
        for (const auto &cmd : ConfigManager::instance().get().autostart)
            processManager.spawnProcess(cmd);

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

// ── Tile ──────────────────────────────────────────────────────────────────

void WindowManager::tile()
{
    XClearWindow(display, root);
    layoutManager.tile(display, root, clientManager, screens);
    XFlush(display);
}

// ── Workspace switching (on the active screen) ────────────────────────────

void WindowManager::switchWorkspace(int ws)
{
    clientManager.switchWorkspace(display, currentScreenIndex, ws, [this]() { tile(); });
}

void WindowManager::moveToWorkspace(int ws)
{
    clientManager.moveToWorkspace(display, root, currentScreenIndex, ws, [this]() { tile(); });
}

// ── Event dispatch ────────────────────────────────────────────────────────

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

// ── Map Request: place window on correct screen ───────────────────────────

void WindowManager::handleMapRequest()
{
    Window w = event.xmaprequest.window;
    if (isDockWindow(display, w))
    {
        XMapWindow(display, w);
        tile();
        return;
    }

    // Determine target screen from pointer position
    Window qroot, qchild;
    int rx, ry, wx, wy;
    unsigned int mask;
    int si = 0;
    if (XQueryPointer(display, root, &qroot, &qchild, &rx, &ry, &wx, &wy, &mask))
        si = screenIndexForPoint(rx, ry);

    clientManager.addClient(w, si, MODE_TILED);
    XSelectInput(display, w, EnterWindowMask);
    XMapWindow(display, w);
    XSetInputFocus(display, w, RevertToPointerRoot, CurrentTime);
    currentScreenIndex = si;
    tile();
}

// ── Configure Request ─────────────────────────────────────────────────────

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

// ── Destroy Notify ────────────────────────────────────────────────────────

void WindowManager::handleDestroyNotify()
{
    Window w = event.xdestroywindow.window;
    const Client *dead = clientManager.findClient(w);
    int si = dead ? dead->screenIndex : currentScreenIndex;

    if (clientManager.removeClient(w))
    {
        tile();
        Window top = clientManager.getTopClientWindow(si);
        if (top != None)
            XSetInputFocus(display, top, RevertToPointerRoot, CurrentTime);
        else
            XSetInputFocus(display, root, RevertToPointerRoot, CurrentTime);
    }
}

// ── Key Press ─────────────────────────────────────────────────────────────

void WindowManager::handleKeyPress()
{
    keybindingManager.handleKeyPress(display, event, *this);
}

// ── Enter Notify: update active screen and focus ──────────────────────────

void WindowManager::handleEnterNotify()
{
    Window w = event.xcrossing.window;
    if (w == root || draggedWindow != None) return;

    // Update currentScreenIndex based on where the pointer entered
    int rx = event.xcrossing.x_root;
    int ry = event.xcrossing.y_root;
    currentScreenIndex = screenIndexForPoint(rx, ry);

    XSetInputFocus(display, w, RevertToPointerRoot, CurrentTime);
}

// ── Button Press: begin drag ──────────────────────────────────────────────

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
    dragWindowW    = c->width;
    dragWindowH    = c->height;
    dragIsFloating = (c->mode == MODE_FLOATING);
    dragScreenIndex = c->screenIndex;

    XGrabPointer(display, root, False,
        ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
        GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
    XSetInputFocus(display, w, RevertToPointerRoot, CurrentTime);
    XRaiseWindow(display, w);
}

// ── Motion Notify: move/swap ──────────────────────────────────────────────

void WindowManager::handleMotionNotify()
{
    if (draggedWindow == None) return;
    Client *c = clientManager.findClient(draggedWindow);
    if (!c) return;

    int px = event.xmotion.x_root;
    int py = event.xmotion.y_root;

    if (dragIsFloating)
    {
        // Move window
        c->x = dragWindowX + px - dragStartX;
        c->y = dragWindowY + py - dragStartY;

        // Detect if pointer crossed into a different screen
        int newSI = screenIndexForPoint(px, py);
        if (newSI != c->screenIndex && newSI < static_cast<int>(screens.size()))
        {
            const ScreenInfo &oldScreen = screens[c->screenIndex];
            const ScreenInfo &newScreen = screens[newSI];

            // Scale floating geometry proportionally from old screen to new screen
            double scaleW = static_cast<double>(newScreen.width)  / oldScreen.width;
            double scaleH = static_cast<double>(newScreen.height) / oldScreen.height;

            int newW = std::max(1, static_cast<int>(dragWindowW * scaleW));
            int newH = std::max(1, static_cast<int>(dragWindowH * scaleH));

            // Reposition within new screen bounds
            int relX = c->x - oldScreen.x;
            int relY = c->y - oldScreen.y;
            c->x = newScreen.x + static_cast<int>(relX * scaleW);
            c->y = newScreen.y + static_cast<int>(relY * scaleH);
            c->width  = newW;
            c->height = newH;
            c->screenIndex = newSI;

            dragWindowW = newW;
            dragWindowH = newH;
            dragScreenIndex = newSI;
        }

        XMoveResizeWindow(display, c->window, c->x, c->y, c->width, c->height);
        XClearWindow(display, root);
        XFlush(display);
        return;
    }

    // Tiled drag: swap windows on same screen only
    int ws = clientManager.getCurrentWorkspace(c->screenIndex);
    auto &tiled = clientManager.getTiledClients();

    Client *hover = nullptr;
    for (auto &cl : tiled)
    {
        if (cl.screenIndex != c->screenIndex || cl.workspace != ws) continue;
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

// ── Button Release ────────────────────────────────────────────────────────

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
    draggedWindow   = None;
    dragTarget      = None;
    dragIsFloating  = false;
    XFlush(display);
}

// ── Property Notify ───────────────────────────────────────────────────────

void WindowManager::handlePropertyNotify()
{
    Atom a = event.xproperty.atom;
    if (a == XInternAtom(display, "_NET_WM_STRUT_PARTIAL", False) ||
        a == XInternAtom(display, "_NET_WM_STRUT", False))
        tile();
}

// ── Expose ────────────────────────────────────────────────────────────────

void WindowManager::handleExpose()
{
    if (event.xexpose.count == 0)
    {
        XClearWindow(display, root);
        XFlush(display);
    }
}

// ── X Error handler ───────────────────────────────────────────────────────

int WindowManager::handleXError(Display *d, XErrorEvent *e)
{
    if (e->error_code == BadAccess || e->error_code == BadWindow) return 0;
    char buf[256];
    XGetErrorText(d, e->error_code, buf, sizeof(buf));
    fprintf(stderr, "X Error: %s req=%d min=%d res=0x%lx\n",
            buf, e->request_code, e->minor_code, e->resourceid);
    return 0;
}

// ── Input focus query ─────────────────────────────────────────────────────

Window WindowManager::GetFocusedWindow()
{
    Window f = 0;
    int r    = 0;
    XGetInputFocus(display, &f, &r);
    return f;
}
