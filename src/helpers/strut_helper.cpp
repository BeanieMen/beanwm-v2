#include "helpers/strut_helper.h"
#include <X11/Xatom.h>
#include <algorithm>

bool isDockWindow(Display *display, Window window)
{
    XWindowAttributes wa{};
    if (XGetWindowAttributes(display, window, &wa) && wa.override_redirect)
        return true;

    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *prop = nullptr;
    Atom net_wm_window_type = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
    Atom net_wm_window_type_dock = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DOCK", False);

    bool is_dock = false;
    if (XGetWindowProperty(display, window, net_wm_window_type, 0, sizeof(Atom), False,
                           XA_ATOM, &actual_type, &actual_format, &nitems,
                           &bytes_after, &prop) == Success && prop)
    {
        Atom *atoms = (Atom *)prop;
        for (unsigned long i = 0; i < nitems; ++i)
        {
            if (atoms[i] == net_wm_window_type_dock)
            {
                is_dock = true;
                break;
            }
        }
        XFree(prop);
    }
    return is_dock;
}

ScreenStruts getScreenStruts(Display *display, Window root)
{
    ScreenStruts struts{};
    Atom net_wm_strut_partial = XInternAtom(display, "_NET_WM_STRUT_PARTIAL", False);
    Atom net_wm_strut = XInternAtom(display, "_NET_WM_STRUT", False);

    Window parent;
    Window *children = nullptr;
    unsigned int nchildren = 0;

    if (!XQueryTree(display, root, &root, &parent, &children, &nchildren) || !children)
        return struts;

    int screen_height = DisplayHeight(display, DefaultScreen(display));
    int screen_width = DisplayWidth(display, DefaultScreen(display));

    for (unsigned int i = 0; i < nchildren; ++i)
    {
        Window w = children[i];
        Atom actual_type;
        int actual_format;
        unsigned long nitems, bytes_after;
        unsigned char *prop = nullptr;

        bool has_strut = false;

        // Try _NET_WM_STRUT_PARTIAL (12 longs)
        if (XGetWindowProperty(display, w, net_wm_strut_partial, 0, 12, False,
                               XA_CARDINAL, &actual_type, &actual_format, &nitems,
                               &bytes_after, &prop) == Success && prop && nitems >= 4)
        {
            long *s = (long *)prop;
            if (s[0] > struts.left) struts.left = static_cast<int>(s[0]);
            if (s[1] > struts.right) struts.right = static_cast<int>(s[1]);
            if (s[2] > struts.top) struts.top = static_cast<int>(s[2]);
            if (s[3] > struts.bottom) struts.bottom = static_cast<int>(s[3]);
            XFree(prop);
            has_strut = true;
        }

        // Try _NET_WM_STRUT (4 longs) if partial not found
        if (!has_strut && XGetWindowProperty(display, w, net_wm_strut, 0, 4, False,
                                              XA_CARDINAL, &actual_type, &actual_format, &nitems,
                                              &bytes_after, &prop) == Success && prop && nitems >= 4)
        {
            long *s = (long *)prop;
            if (s[0] > struts.left) struts.left = static_cast<int>(s[0]);
            if (s[1] > struts.right) struts.right = static_cast<int>(s[1]);
            if (s[2] > struts.top) struts.top = static_cast<int>(s[2]);
            if (s[3] > struts.bottom) struts.bottom = static_cast<int>(s[3]);
            XFree(prop);
            has_strut = true;
        }

        // Fallback: If it's a dock window mapped on screen without explicit strut property
        if (!has_strut && isDockWindow(display, w))
        {
            XWindowAttributes wa{};
            if (XGetWindowAttributes(display, w, &wa) && wa.map_state == IsViewable)
            {
                if (wa.y == 0 && wa.height > struts.top && wa.height < screen_height / 2)
                    struts.top = wa.height;
                else if (wa.y + wa.height >= screen_height - 10 && wa.height > struts.bottom)
                    struts.bottom = wa.height;
                else if (wa.x == 0 && wa.width > struts.left && wa.width < screen_width / 2)
                    struts.left = wa.width;
                else if (wa.x + wa.width >= screen_width - 10 && wa.width > struts.right)
                    struts.right = wa.width;
            }
        }
    }

    if (children)
        XFree(children);

    return struts;
}
