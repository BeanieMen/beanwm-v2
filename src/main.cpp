#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <unistd.h>
#include <sys/types.h>
#include "window_manager.h"

int main(void)
{
    WindowManager wm;
    wm.run();
 }