#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <X11/keysym.h>
#include <unistd.h>
#include <sys/types.h>
#include <X11/Xlib.h>
#include "types.h"
#include "window_manager.h"

WindowManager::WindowManager() : display(nullptr), root(0), clients{}, current_workspace(1)
{
    display = XOpenDisplay(NULL);

    if (display == NULL)
    {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }
    XSetErrorHandler(WindowManager::handleXError);
    root = DefaultRootWindow(display);

    printf("Connected to display: %s\n", DisplayString(display));

    setup();
}

WindowManager::~WindowManager()
{

    if (display != NULL)
    {
        XCloseDisplay(display);
    }
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
    XGrabKey(display, XKeysymToKeycode(display, XK_Return), Mod1Mask, root, True, GrabModeAsync, GrabModeAsync);
    XSelectInput(display, root, EnterWindowMask | SubstructureRedirectMask | SubstructureNotifyMask | KeyPressMask);
    setupKeybindings();
    XSync(display, False);
}

void WindowManager::handleEvent()
{
    switch (event.type)
    {
    case MapRequest:
        handleMapRequest();
        break;
    case CreateNotify:
        handleCreateNotify();
        break;
    case DestroyNotify:
        handleDestroyNotify();
        break;
    case KeyPress:
        printf("KeyPress event: keycode=%u, state=%u\n", event.xkey.keycode, event.xkey.state);
        handleKeyPress();
        break;
    case EnterNotify:
        handleEnterNotify();
        break;
    default:
        break;
    }
}

void WindowManager::handleMapRequest()
{
    Window window = event.xmaprequest.window;

    addClient(window);
    XSelectInput(display, window, EnterWindowMask);
    XMapWindow(display, window);
    XSetInputFocus(display, window, RevertToPointerRoot, CurrentTime);
    tile();
}

void WindowManager::handleCreateNotify()
{
    printf("Window created: %lu, parent: %lu\n", event.xcreatewindow.window, event.xcreatewindow.parent);
    for (const auto &c : clients)
    {
        printf("Client window: %lu\n", c.window);
    }
}

void WindowManager::handleDestroyNotify()
{
    Window window = event.xdestroywindow.window;
    printf("Window destroyed: %lu\n", window);
    bool removed = removeClient(window);
    if (removed)
    {
        tile();
    }
}

void WindowManager::handleEnterNotify()
{
    Window window = event.xcrossing.window;
    XSetInputFocus(display, window, RevertToPointerRoot, CurrentTime);
}

void WindowManager::spawnTerminal()
{
    if (fork() == 0)
    {
        execlp("alacritty", "alacritty", NULL);
        fprintf(stderr, "Failed to execute alacritty\n");
        exit(1);
    }
}

void WindowManager::addClient(Window window)
{
    auto it = std::find_if(clients.begin(), clients.end(), [&](const Client &c) {
        return c.window == window;
    });
    if (it != clients.end())
    {
        printf("Window %lu is already managed\n", window);
        return;
    }

    clients.emplace_back(Client{window, current_workspace});
}

bool WindowManager::removeClient(Window window)
{
    auto it = std::find_if(clients.begin(), clients.end(), [&](const Client &c) {
        return c.window == window;
    });

    if (it != clients.end())
    {
        printf("Removing client: %lu\n", window);
        clients.erase(it);
        return true;
    }
    return false;
}

Client *WindowManager::findClient(Window window)
{
    auto it = std::find_if(clients.begin(), clients.end(), [&](Client &c) {
        return c.window == window;
    });
    if (it != clients.end()) {
        return &*it;
    }
    return nullptr;
}

void WindowManager::tile()
{
    dwindleTile(display, clients, 5, current_workspace);
}
int WindowManager::handleXError(
    Display *display,
    XErrorEvent *error)
{
    char error_text[256];

    XGetErrorText(
        display,
        error->error_code,
        error_text,
        sizeof(error_text));

    fprintf(
        stderr,
        "X Error: %s\n"
        "  request=%d\n"
        "  minor=%d\n"
        "  resource=0x%lx\n",
        error_text,
        error->request_code,
        error->minor_code,
        error->resourceid);

    return 0;
}

void WindowManager::setupKeybindings()
{
    keybindings = {
        {WindowManager::MOD_MASK, XK_Return, [this]()
         { spawnTerminal(); }},
        {WindowManager::MOD_MASK, XK_1, [this]()
         { switchWorkspace(1); }},
        {WindowManager::MOD_MASK, XK_2, [this]()
         { switchWorkspace(2); }},
        {WindowManager::MOD_MASK, XK_3, [this]()
         { switchWorkspace(3); }},
        {WindowManager::MOD_MASK, XK_4, [this]()
         { switchWorkspace(4); }},
        {WindowManager::MOD_MASK, XK_5, [this]()
         { switchWorkspace(5); }},
        {WindowManager::MOD_MASK, XK_6, [this]()
         { switchWorkspace(6); }},
        {WindowManager::MOD_MASK, XK_7, [this]()
         { switchWorkspace(7); }},
        {WindowManager::MOD_MASK, XK_8, [this]()
         { switchWorkspace(8); }},
        {WindowManager::MOD_MASK, XK_9, [this]()
         { switchWorkspace(9); }}};

    
}

void WindowManager::switchWorkspace(int workspace)
{
    if (workspace < 1 || workspace > WORKSPACE_COUNT)
    {
        fprintf(stderr, "Invalid workspace: %d\n", workspace);
        return;
    }
    if (workspace == current_workspace) {
        return;
    }

    hideWorkspace(current_workspace);
    current_workspace = workspace;
    showWorkspace(current_workspace);
    tile();
}

void WindowManager::moveToWorkspace(int workspace)
{
    if (workspace < 1 || workspace > WORKSPACE_COUNT) {
        fprintf(stderr, "Invalid workspace: %d\n", workspace);
        return;
    }

    Window focused = 0;
    int revert = 0;
    XGetInputFocus(display, &focused, &revert);
    if (focused == None || focused == root) {
        return;
    }

    Client *client = findClient(focused);
    if (client == nullptr) {
        fprintf(stderr, "Focused window %lu is not a managed client\n", focused);
        return;
    }

    int old_workspace = client->workspace;
    client->workspace = workspace;
    if (old_workspace != current_workspace && workspace == current_workspace) {
        XMapWindow(display, client->window);
    } else if (old_workspace == current_workspace && workspace != current_workspace) {
        XUnmapWindow(display, client->window);
    }

    if (workspace == current_workspace) {
        showWorkspace(current_workspace);
    }
    tile();
}

void WindowManager::showWorkspace(int workspace)
{
    for (auto &c : clients) {
        if (c.workspace == workspace) {
            XMapWindow(display, c.window);
        }
    }
}

void WindowManager::hideWorkspace(int workspace)
{
    for (auto &c : clients) {
        if (c.workspace == workspace) {
            XUnmapWindow(display, c.window);
        }
    }
}

void WindowManager::handleKeyPress()
{
    KeySym key = XLookupKeysym(&event.xkey, 0);

    unsigned int modifiers = event.xkey.state & (ShiftMask | ControlMask | Mod1Mask | Mod4Mask);

    for (const KeyBinding &binding : keybindings)
    {
        XGrabKey(display, XKeysymToKeycode(display, binding.key), binding.modifiers, root, True, GrabModeAsync, GrabModeAsync);
        if (binding.key != key)
        {
            continue;
        }

        if (binding.modifiers != modifiers)
        {
            continue;
        }

        binding.action();
        return;
    }
}
