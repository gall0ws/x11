#include <X11/Xatom.h>
#include <X11/Xutil.h>

#include "dat.h"
#include "fns.h"

static
Window
newframe(void)
{
	int mask;
	XSetWindowAttributes attr;
	Window w;

	mask = CWBorderPixel | CWOverrideRedirect | CWEventMask;
	attr.border_pixel = bcolor[Inactive];
	attr.override_redirect = 1;
	attr.event_mask = FrameMask;
	w = XCreateWindow(dpy, root, 0, 0, 1, 1, BorderWidth,
	    CopyFromParent, CopyFromParent, CopyFromParent,
	    mask, &attr);
	return w;
}

Client *
newclient(Window w)
{
	Client *c;
	XClassHint class;

	debug("%#x", (int)w);
	if (!XGetClassHint(dpy, w, &class)) {
		class.res_name = nil;
		class.res_class = nil;
	}
	c = emalloc(sizeof(*c));
	c->window = w;
	c->setinput = 1;
	c->rcname = class.res_name;
	c->class = class.res_class;
	c->is9term = (c->rcname && strcmp(c->rcname, "9term") == 0);
	XSelectInput(dpy, w, ClientMask);
	return c;
}

void
freeclient(Client *c)
{
	debug("%#x", clientid(c));
	if (c == nil) {
		return;
	}
	if (c->frame) {
		XDestroyWindow(dpy, c->frame);
	}
	xfree(c->rcname);
	xfree(c->class);
	xfree(c->name);
	xfree(c->iconname);
	free(c);
}

void
manage(Client *c)
{
	debug("%#x", clientid(c));
	if (c->frame) {
		err("managing a window (%#x) already framed (%#x)", (int)c->window, (int)c->frame)
	}
	c->frame = newframe();
	XSetWindowBorderWidth(dpy, c->window, 0);
	XReparentWindow(dpy, c->window, c->frame, 0, 0);
	XAddToSaveSet(dpy, c->window);
	synchints(c);
	syncname(c);
	syncproto(c);
	synctrans(c);
}

static
void
_setstate(Client *c, int state)
{
	long data[2];

	c->state = state;
	data[0] = state;
	data[1] = None; /* icon */
	XChangeProperty(dpy, c->window, wm_state, XA_ATOM, 32,
	    PropModeReplace, (uchar *)data, 2);
}

void
map(Client *c)
{
	debug("%#x", clientid(c));
	if (c->frame) {
		XMapWindow(dpy, c->frame);
	}
	XMapWindow(dpy, c->window);
	_setstate(c, NormalState);
}

void
unmap(Client *c)
{
	debug("%#x", clientid(c));
	if (c->frame) {
		XUnmapWindow(dpy, c->frame);
	}
	XUnmapWindow(dpy, c->window);
	_setstate(c, IconicState);
}

void
withdrawn(Client *c)
{
	debug("%#x", clientid(c));
	if (c->frame) {
		XUnmapWindow(dpy, c->frame);
	}
	/* c->window is already unmapped (see ICCCM 2.0, 4.1.4) */
	_setstate(c, WithdrawnState);
}

static
void
setactive(Client *c, int on)
{
	Window focus;

	c->hasfocus = on;
	debug("%#x (%d)", clientid(c), on);
	if (on) {
		focus = c->setinput? c->window : nofocus;
		XSetInputFocus(dpy, focus, RevertToPointerRoot, CurrentTime);
		if (c->proto.takefocus) {
			sendmsg(c->window, wm_take_focus);
		}
	} else if (c->proto.losefocus) {
		sendmsg(c->window, ninewm_lose_focus);
	}
	if (c->frame) {
		XSetWindowBorder(dpy, c->frame, bcolor[on]);
	}
}

/* http://standards.freedesktop.org/wm-spec/wm-spec-1.5.html#idp6314016 */
void
fullscr(Client *c, int on)
{
	Rect r;

	debug("%#x (%d)", clientid(c), c->ewmh.fullscr);
	if (c->ewmh.fullscr == on) {
		return;
	}
	if (on) {
		r.x = 0;
		r.y = 0;
		r.dx = rootdx;
		r.dy = rootdy;
		memcpy(&c->ewmh.oldr, &c->r, sizeof(c->r));
		geom(c, &r, 1);
	} else {
		geom(c, &c->ewmh.oldr, 0);
	}
	c->ewmh.fullscr = on;
	/* this one must be changed if we're going to support other _NET_WM_STATE states: */
	XChangeProperty(dpy, c->window, net_wm_state, XA_ATOM, 32, 
		PropModeReplace, (uchar *)&net_wm_state_fullscreen, on);
}

void
geom(Client *c, Rect *r, int win)
{
	uint mask;
	XWindowChanges wc;

	debug("%#x (%d)", clientid(c), win);
	if (!win && !c->frame) {
		err("warning: geometry change for %#x frame, but it has no frame!", clientid(c));
	}
	memcpy(&c->r, r, sizeof(*r));
	if (win && c->frame) {
		c->r.x -= BorderWidth;
		c->r.y -= BorderWidth;
	}
	mask = CWX | CWY | CWWidth | CWHeight;
	wc.x = c->r.x;
	wc.y = c->r.y;
	wc.width = c->r.dx;
	wc.height = c->r.dy;
	if (c->frame) {
		XConfigureWindow(dpy, c->frame, mask, &wc);
		mask = CWWidth | CWHeight; /* set window mask */
	}
	XConfigureWindow(dpy, c->window, mask, &wc);
}

void wraise(Client *c)
{
	debug("%#x", clientid(c));
	if (c == nil) {
		debug("skipping unknown window %#x", clientid(c));
		return;
	}
	if (c->frame) {
		XRaiseWindow(dpy, c->frame);
	}
	XRaiseWindow(dpy, c->window);
	if (c != current) { /* restack */
		if (current) {
			setactive(current, 0);
			(void)lookup(&current, c->window, 1);
		}
		add(&current, c);
	}
	setactive(c, 1);
}

void
active(Client *c)
{
	Client *p;

	debug("%#x", clientid(c));
	if (c == nil) {
		debug("skipping unknown window %#x", clientid(c));
		return;
	}
	if (focuspolicy == FocusClick) {
		wraise(c);
		return;
	} else {
		for (p=clients[curvirt]; p!=nil; p=p->next) {
			if (p != c && p->hasfocus) {
				setactive(p, 0);
				break;
			}
		}
	}
	setactive(c, 1);
}

/* ICCCM 2.0, 4.1.2.3
 * ICCCM 2.0, 4.1.2.4 */
void
synchints(Client *c)
{
	XWMHints *wmh;
	XSizeHints *szh;
	long dummy;

	wmh = XGetWMHints(dpy, c->window);
	if (wmh && (wmh->flags & InputHint)) {
		c->setinput = wmh->input;
	}
	szh = XAllocSizeHints();
	if (XGetWMNormalHints(dpy, c->window, szh, &dummy)) {
		if (szh && (szh->flags & PAspect)) {
			c->aratio = (double)szh->min_aspect.x / (double)szh->min_aspect.y;
		}
	}
	xfree(wmh);
	xfree(szh);
}

/* http://standards.freedesktop.org/wm-spec/wm-spec-1.5.html#idp6295216
 * http://standards.freedesktop.org/wm-spec/wm-spec-1.5.html#idp6298320 */
void
syncname(Client *c)
{
	xfree(c->name);
	xfree(c->iconname);
	c->name = getprop(c->window, net_wm_name);
	if (c->name) {
		c->ewmh.name = 1;
	}
	c->iconname = getprop(c->window, net_wm_icon_name);
	if (c->iconname) {
		c->ewmh.iconname = 1;
	}
	if (!c->ewmh.name) {
		c->name = getprop(c->window, XA_WM_NAME);
	}
	if (!c->ewmh.iconname) {
		c->iconname = getprop(c->window, XA_WM_ICON_NAME);
	}
}

/* ICCCM 2.0, 4.1.2.7 */
void
syncproto(Client *c)
{
	Atom *a;
	int i, n;

	if (!XGetWMProtocols(dpy, c->window, &a, &n)) {
		return;
	}
	memset(&c->proto, 0, sizeof(c->proto));
	for (i=0; i<n; i++) {
		if (a[i] == wm_delete_window) {
			c->proto.deletewin = 1;
		} else if (a[i] == wm_take_focus) {
			c->proto.takefocus = 1;
		} else if (a[i] == ninewm_lose_focus) {
			c->proto.losefocus = 1;
		}
	}
	xfree(a);
}

void
synctrans(Client *c)
{
	Client *p;

	XGetTransientForHint(dpy, c->window, &c->trans);
	if (c->trans == None) {
		return;
	}
	p = getclient(c->trans, 0);
	if (p == nil) {
		p = newclient(c->trans);
		add(&limbo, p);
	}
}

enum {
	CornerLen	= 20,
};

int
edgeat(Client *c, int rootx, int rooty)
{
	Window dummy;
	int x, y;

	if (c->frame == None) {
		return -1;
	}
	XTranslateCoordinates(dpy, root, c->frame, rootx, rooty, &x, &y, &dummy);
	if (y < 0) {
		if (x < CornerLen) {
			return TopLeft;
		} else if (x >= c->r.dx - CornerLen) {
			return TopRight;
		} else {
			return Top;
		}
	} else if (y >= c->r.dy) {
		if (x < CornerLen) {
			return BottomLeft;
		} else if (x >= c->r.dx - CornerLen) {
			return BottomRight;
		} else {
			return Bottom;
		}
	} else if (x < 0) {
		if (y < CornerLen) {
			return TopLeft;
		} else if (y >= c->r.dy - CornerLen) {
			return BottomLeft;
		} else {
			return Left;
		}
	} else if (x >= c->r.dx) {
		if (y < CornerLen) {
			return TopRight;
		} else if (y >= c->r.dy - CornerLen) {
			return BottomRight;
		} else {
			return Right;
		}
	}
	return -1;
}

static
void
printcli(Client *c)
{
	printf("\t%#x%s\t%s\t%s\n",
	    clientid(c), c==current? "*" : "", c->rcname, c->name);
}

void
dumpclients(void)
{
	int i;

	printf("#############\n");
	for (i=0; i<NVirtuals; i++) {
		printf("virtual %d\n", i+1);
		apply(clients[i], printcli);
	}
	printf("hiddens\n");
	apply(hiddens, printcli);
	printf("limbo\n");
	apply(limbo, printcli);
	printf("#############\n");
	fflush(stdout);
}
