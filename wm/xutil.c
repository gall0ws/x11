#include <X11/Xatom.h>

#include "dat.h"
#include "fns.h"

int
discard(Window w, long mask, XEvent *last)
{
	XEvent e;
	int n;

	n = 0;
	if (w == None) {
		while (XCheckMaskEvent(dpy, mask, &e)) {
			n++;
		}
	} else {
		while (XCheckWindowEvent(dpy, w, mask, &e)) {
			n++;
		}
	}
	if (last) {
		memcpy(last, &e, sizeof(e));
	}
	return n;
}

char *
getprop(Window w, Atom a)
{
	Atom type;
	int fmt;
	ulong n, b;
	uchar *p;

	XGetWindowProperty(dpy, w, a, 0, 32, False,
	    AnyPropertyType, &type, &fmt,
	    &n, &b, &p);
	return (char *)p;
}

/* ICCCM 2.0, 4.2.8 */
int
sendmsg(Window w, Atom a)
{
	XEvent e;

	e.type = ClientMessage;
	e.xclient.message_type = wm_protocols;
	e.xclient.display = dpy;
	e.xclient.window = w;
	e.xclient.format = 32;
	e.xclient.data.l[0] = a;
	e.xclient.data.l[1] = CurrentTime;
	return XSendEvent(dpy, w, False, 0, &e) != 0 ? 0 : -1;
}

int
grab(Window w, Cursor c)
{
	int res;

	res = XGrabPointer(dpy, w, False, GrabMask, 
	    GrabModeAsync, GrabModeAsync, root, c, CurrentTime);
	if (res != GrabSuccess) {
		XAllowEvents(dpy, SyncPointer, CurrentTime);
		return -1;
	}
	return 0;
}

void
ungrab(void)
{
	discard(None, GrabMask, nil);
	XUngrabPointer(dpy, CurrentTime);
}

void
mousexy(Window w, int *x, int *y)
{
	int x11, hate;
	Window really, i;
	uint dummy;

	XQueryPointer(dpy, w,
	    &i, &really, &hate, &x11,
	    x, y, &dummy);
}

int
xfree(void *data)
{
	if (data == nil) { /* see XFree(3) */
		return 0;
	}
	return XFree(data);
}
