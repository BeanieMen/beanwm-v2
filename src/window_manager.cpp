#include "window_manager.h"
#include "layout.h"
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

WindowManager::WindowManager() : display(nullptr), root(0), clients{} {
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);
    display = XOpenDisplay(nullptr);
    if (!display) {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }
    XSetErrorHandler(handleXError);
    root = DefaultRootWindow(display);
    setup();
}

WindowManager::~WindowManager() {
    if (display) XCloseDisplay(display);
}

void WindowManager::run() {
    while (true) {
        XNextEvent(display, &event);
        handleEvent();
    }
}

void WindowManager::setup() {
    current_workspace = 1;
    XSelectInput(display, root, EnterWindowMask | SubstructureRedirectMask | SubstructureNotifyMask | KeyPressMask);
    Window parent;
    Window *children = nullptr;
    unsigned int child_count = 0;
    if (XQueryTree(display, root, &root, &parent, &children, &child_count)) {
        for (unsigned int i = 0; i < child_count; ++i) {
            addClient(children[i]);
            XSelectInput(display, children[i], EnterWindowMask);
        }
        if (children) XFree(children);
    }
    setupKeybindings();
    XUngrabKey(display, AnyKey, AnyModifier, root);
    for (auto &b : keybindings) {
        KeyCode code = XKeysymToKeycode(display, b.key);
        if (code) {
            XGrabKey(display, code, b.modifiers, root, False, GrabModeAsync, GrabModeAsync);
            XGrabKey(display, code, b.modifiers | LockMask, root, False, GrabModeAsync, GrabModeAsync);
            XGrabKey(display, code, b.modifiers | Mod2Mask, root, False, GrabModeAsync, GrabModeAsync);
            XGrabKey(display, code, b.modifiers | LockMask | Mod2Mask, root, False, GrabModeAsync, GrabModeAsync);
        }
    }
    XSync(display, False);
}

void WindowManager::addClient(Window window) {
    for (auto &client : clients) {
        if (client.window == window) return;
    }
    clients.push_back(Client{window, current_workspace});
}

bool WindowManager::removeClient(Window window) {
    for (size_t i = 0; i < clients.size(); ++i) {
        if (clients[i].window == window) {
            clients.erase(clients.begin() + static_cast<long>(i));
            return true;
        }
    }
    return false;
}

Client* WindowManager::findClient(Window window) {
    for (auto &client : clients) {
        if (client.window == window) return &client;
    }
    return nullptr;
}

void WindowManager::handleEvent() {
    switch (event.type) {
        case MapRequest: handleMapRequest(); break;
        case CreateNotify: handleCreateNotify(); break;
        case DestroyNotify: handleDestroyNotify(); break;
        case KeyPress: handleKeyPress(); break;
        case EnterNotify: handleEnterNotify(); break;
        default: break;
    }
}

void WindowManager::handleMapRequest() {
    Window window = event.xmaprequest.window;
    addClient(window);
    XSelectInput(display, window, EnterWindowMask);
    XMapWindow(display, window);
    XSetInputFocus(display, window, RevertToPointerRoot, CurrentTime);
    tile();
}

void WindowManager::handleCreateNotify() {
    for (auto &client : clients) printf("Client window: %lu\n", client.window);
}

void WindowManager::handleDestroyNotify() {
    if (removeClient(event.xdestroywindow.window)) tile();
}

void WindowManager::handleEnterNotify() {
    XSetInputFocus(display, event.xcrossing.window, RevertToPointerRoot, CurrentTime);
}

void WindowManager::spawnTerminal() {
    if (fork() == 0) {
        execlp(TERMINAL, TERMINAL, nullptr);
        fprintf(stderr, "Failed to exec %s\n", TERMINAL);
        exit(1);
    }
}

void WindowManager::spawnProcess(const std::string& command) {
    if (fork() == 0) {
        execlp(command.c_str(), command.c_str(), nullptr);
        fprintf(stderr, "Failed to exec %s\n", command.c_str());
        exit(1);
    }
}

void WindowManager::tile() {
    dwindleTile(display, clients, GAP, current_workspace);
}

void WindowManager::switchWorkspace(int workspace) {
    if (workspace < 1 || workspace > WORKSPACE_COUNT || workspace == current_workspace) return;
    hideWorkspace(current_workspace);
    current_workspace = workspace;
    showWorkspace(current_workspace);
    tile();
}

void WindowManager::moveToWorkspace(int workspace) {
    if (workspace < 1 || workspace > WORKSPACE_COUNT) return;
    Window focused = 0;
    int revert = 0;
    XGetInputFocus(display, &focused, &revert);
    if (focused == None || focused == root) return;
    Client* client = findClient(focused);
    if (!client) return;
    int old_workspace = client->workspace;
    client->workspace = workspace;
    if (old_workspace != current_workspace && workspace == current_workspace) XMapWindow(display, client->window);
    else if (old_workspace == current_workspace && workspace != current_workspace) XUnmapWindow(display, client->window);
    if (workspace == current_workspace) showWorkspace(current_workspace);
    tile();
}

void WindowManager::showWorkspace(int workspace) {
    for (auto &client : clients) if (client.workspace == workspace) XMapWindow(display, client.window);
}

void WindowManager::hideWorkspace(int workspace) {
    for (auto &client : clients) if (client.workspace == workspace) XUnmapWindow(display, client.window);
}

int WindowManager::handleXError(Display* display_handle, XErrorEvent* error_event) {
    char buffer[256];
    XGetErrorText(display_handle, error_event->error_code, buffer, sizeof(buffer));
    fprintf(stderr, "X Error: %s request=%d minor=%d resource=0x%lx\n", buffer, error_event->request_code, error_event->minor_code, error_event->resourceid);
    return 0;
}
