#include <X11/Xlib.h>
#include <iostream>
#include "X11/keysym.h"
#include "util/os_time.h"

Display *dpy = XOpenDisplay(":0");

bool key_is_pressed(KeySym ks) {
    char keys_return[32];
    XQueryKeymap(dpy, keys_return);
    KeyCode kc2 = XKeysymToKeycode(dpy, ks);
    bool isPressed = !!(keys_return[kc2 >> 3] & (1 << (kc2 & 7)));
    return isPressed;
}
