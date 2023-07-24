#define __LINUX_LOCAL_H__

extern qglConfig_t qglConfig;

// glimp.cpp

//#define ID_ENABLE_DGA
#if defined( ID_ENABLE_DGA )
#include <X11/extensions/xf86dga.h>
#endif
#include <X11/extensions/xf86vmode.h>
#include <X11/XKBlib.h>

extern Display *dpy;
extern Window win;

// input.cpp
extern bool dga_found;
void Sys_XEvents();
void Sys_XUninstallGrabs();

#define KEY_MASK (KeyPressMask | KeyReleaseMask)
#define MOUSE_MASK (ButtonPressMask | ButtonReleaseMask | PointerMotionMask | ButtonMotionMask )
#define X_MASK (KEY_MASK | MOUSE_MASK | VisibilityChangeMask | StructureNotifyMask )

#ifndef ID_GL_HARDLINK
bool GLimp_dlopen();
void GLimp_dlclose();

void GLimp_BindLogging();
void GLimp_BindNative();
#endif

#endif
