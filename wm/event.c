#include <string.h>
#include <stdlib.h>
#include <X11/XF86keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "dat.h"
#include "fns.h"

static
void
button1(XButtonEvent *e)
{
	Client *c, *p, *t;
	Rect r;
	int i, res, edge;

	if (e->subwindow == None) {
		if (hiddens == nil) {
			XAllowEvents(dpy, SyncPointer, e->time);
			return;
		}
		i = menuhit(&unhidemenu, e->button);
		if (i == -1) {
			return;
		}
		c = lookup2(&hiddens, i, 1);
		if (c == nil) {
			err("no hidden client in position %d", i);
			dumpclients();
			return;
		}
		p = c;
		while ((t = getclient(p->trans, 1))) {
			map(t);
			add(&current, t);
			p = t;
		}
		map(c);
		active(c);
		return;
	}
	c = getclient(e->subwindow, 0);
	if (c == nil) {
		XAllowEvents(dpy, ReplayPointer, e->time);
		return;
	}
	edge = edgeat(c, e->x, e->y);
	if (edge == -1) {
		if (c == current) {
			XAllowEvents(dpy, ReplayPointer, e->time);
		} else {
			active(c);
			XAllowEvents(dpy, SyncPointer, e->time);
		}
	} else {
		memcpy(&r, &c->r, sizeof(r));
		res = uresize(transwin, c->aratio, edge, e->button, -1, &r);
		XUnmapWindow(dpy, transwin);
		if (res != 0) {
			return;
		}
		geom(c, &r, 0);
		active(c);
	}
}

static
void
button2(XButtonEvent *e)
{
	int v;

	if (e->subwindow != None) {
		XAllowEvents(dpy, ReplayPointer, e->time);
		return;
	}
	v = menuhit(&virtmenu, e->button);
	if (v == -1 || v == curvirt) {
		return;
	}
	virtmenu.lasthit = v;
	aswitchvirt(v);
}

static
void
button3(XButtonEvent *e)
{
	Client *c;
	Rect r;
	int res, edge;
	int forcemenu = 0;

	c = getclient(e->subwindow, 0);
	if (c != nil) {
		edge = edgeat(c, e->x, e->y);
		/* allow action menu on 9term (orrible hack) */
		forcemenu = (c->is9term && edge == -1 && e->x > c->r.x + BorderWidth + 15);
	}
	if (e->subwindow == None || forcemenu) {
		res = menuhit(&actionmenu, e->button);
		switch (res) {
		case -1:
			return;
		case New:
			anew();
			break;
		case Resize:
			aresize();
			break;
		case Move:
			amove();
			break;
		case Delete:
			adelete();
			break;
		case Hide:
			ahide();
			break;
		}
		actionmenu.lasthit = res;
		return;
	}
	if (c == nil || edge == -1) {
		XAllowEvents(dpy, ReplayPointer, e->time);
		return;
	}
	memcpy(&r, &c->r, sizeof(r));
	res = umove(transwin, e->button, &r);
	XUnmapWindow(dpy, transwin);
	if (res != 0) {
		return;
	}
	geom(c, &r, 0);
	active(c);
}

void
ebutton(XButtonEvent *e)
{
	switch (e->button) {
	case Button1:
		button1(e);
		break;
	case Button2:
		button2(e);
		break;
	case Button3:
		button3(e);
		break;
	}
}

void
eclimsg(XClientMessageEvent *e)
{
	Client *c;

	c = getclient(e->window, 0);
	if (c == nil) {
		return;
	}
	if (e->message_type == net_wm_state) {
		if (e->data.l[1] == net_wm_state_fullscreen) {
			fullscr(c, e->data.l[0]);
		}
	} else if (e->message_type == net_active_window) {
		c = getclient(e->window, 1);
		if (c == nil) {
			err("_NET_ACTIVE_WINDOW request for unmanaged window %#x",
			    (int)e->window);
			return;
		}
		if (c->state != NormalState) {
			map(c);
		}
		active(c);
	}
}

void
econfig(XConfigureEvent *e)
{
	if (e->window == root) {
		rootdx = e->width;
		rootdy = e->height;
	}
}

void
econfigreq(XConfigureRequestEvent *e)
{
	Client *c;
	Rect r;
	int resize;

	debug("econfigreq: %#x", (int)e->window);
	c = getclient(e->window, 0);
	if (c == nil) {
		err("config request for unknown window (%#x)", (int)e->window);
		return;
	}
	resize = 0;
	memcpy(&r, &c->r, sizeof(r));
	if (e->value_mask & CWX) {
		r.x = e->x;
		resize++;
	}
	if (e->value_mask & CWY) {
		r.y = e->y;
		resize++;
	}
	if (e->value_mask & CWWidth) {
		r.dx = e->width;
		resize++;
	}
	if (e->value_mask & CWHeight) {
		r.dy = e->height;
		resize++;
	}
	if (resize) {
		geom(c, &r, 1);
	}
	if ((e->value_mask & CWStackMode) && e->detail == Above && c != current) {
		switch (c->state) {
		case WithdrawnState:
		case IconicState:
			if (c->frame == None) {
				/* wait for MapRequest */
				break;
			}
			(void)getclient(c->window, 1);
			map(c);
			/* fall-through */
		case NormalState:
			active(c);
			break;
		}
	}
}

void
ecreate(XCreateWindowEvent *e)
{
	Client *c;

	if (e->override_redirect) {
		return;
	}
	debug("ecreate: %#x", (int)e->window);
	c = getclient(e->window, 0);
	if (c != nil) {
		err("create event for %#x", clientid(c));
		return;
	}
	c = newclient(e->window);
	c->r.x = e->x;
	c->r.y = e->y;
	c->r.dx = e->width;
	c->r.dy = e->height;
	add(&limbo, c);
}

/* restore the cursor when the pointer leaves client's frame (see :/^emotion/)  */
void
ecross(XCrossingEvent *e)
{
	Client *c;

	if (e->type == LeaveNotify) {
		c = getclient(e->window, 0);
		if (c == nil) {
			return;
		}
		XUndefineCursor(dpy, c->frame);
	}
}

void
edestroy(XDestroyWindowEvent *e)
{
	Client *c, *p;
	int revert;

	revert = (current && current->window == e->window);
	c = getclient(e->window, 1);
	if (c == nil) {
		return;
	}
	debug("edestroy: %#x", clientid(c));
	if (revert) {
		p = getclient(c->trans, 1);
		if (p == nil) {
			p = current;
		}
		active(p);
	}
	freeclient(c);
}

void
ekey(XKeyEvent *e)
{
	int mode;
	char buf[8];
	KeySym ksym;

	mode = ReplayKeyboard;
	if (e->type == KeyPress) {
		XLookupString(e, buf, sizeof(buf), &ksym, nil);
		switch (ksym) {
		case XK_Tab:
			if ((strcmp(buf, "SWITCH") == 0)) {
				aswitchwin();
				mode = SyncKeyboard;
				discard(None, FocusChangeMask, nil);
			}
			break;	
		case XF86XK_Launch0:
			arun();
			mode = SyncKeyboard;
			break;
		case XK_Next_Virtual_Screen:
			aswitchvirt(mod(curvirt + 1,NVirtuals));
			break;
		case XK_Prev_Virtual_Screen:
			aswitchvirt(mod(curvirt - 1, NVirtuals));
			break;
		}
	}
	XAllowEvents(dpy, mode, e->time);
}

/* clients that on MapRequest will be manually resized. */
static char *rcname[] = {
	"9term",
	"acme",
	"djview",
	"mupdf",
	"rxvt",
	"sam",
	"urxvt",
	"xterm",
	nil,
};

void
emapreq(XMapRequestEvent *e)
{
	Client *c;
	Rect r;
	int manual, i, res;

	debug("emapreq: %#x", (int)e->window);
	c = getclient(e->window, 1);
	if (c == nil) {
		err("map request for unknown window (%#x)", (int)e->window);
		return;
	}
	if (c->frame == None) {
		manage(c);
	}
	manual = 0;
	for (i=0; c->rcname && rcname[i]!=nil; i++) {
		if (strcmp(c->rcname, rcname[i]) == 0) {
			res = upickxy(Button3, &r.x, &r.y);
			if (res != 0) {
				if (c->is9term) {
					XKillClient(dpy, c->window);
					return;
				}
				break;
			}
			r.dx = 1;
			r.dy = 1;
			res = uresize(blankwin, c->aratio, -1, Button3, Button1, &r);
			if (res == 0) {
				manual = 1;
			} else if (c->is9term) {
				XKillClient(dpy, c->window);
				goto quit;
			}
			break;
		}
	}
	if (!manual) {
		memcpy(&r, &c->r, sizeof(r));
	}
	map(c);
	geom(c, &r, 0);
	active(c);
quit:
	/* this has to be done after map(c) to avoid flickering */
	XUnmapWindow(dpy, blankwin);
}

/* set the appropriate cursor when the pointer is on the client's frame */
void
emotion(XMotionEvent *e)
{
	Client *c;
	int edge;

	c = getclient(e->window, 0);
	if (c == nil) {
		err("c == nil on emotion()");
		return;
	}
	edge = edgeat(c, e->x_root, e->y_root);
	if (edge == -1) {
		return;
	}
	XDefineCursor(dpy, c->frame, cursor[edge]);
}

void
eproperty(XPropertyEvent *e)
{
	Client *c;
	int *state;

	c = getclient(e->window, 0);
	if (c == nil) {
		return;
	}
	switch (e->atom) {
	case XA_WM_NAME:
		if (c->ewmh.name) {
			return;
		}
		xfree(c->name);
		c->name = getprop(c->window, e->atom);
		return;
	case XA_WM_ICON_NAME:
		if (c->ewmh.iconname) {
			return;
		}
		xfree(c->iconname);
		c->iconname = getprop(c->window, e->atom);
		return;
	case XA_WM_HINTS:
	case XA_WM_NORMAL_HINTS:
		synchints(c);
		return;
	case XA_WM_TRANSIENT_FOR:
		synctrans(c);
		return;
	}
	if (e->atom == wm_state) {
		state = (int *)getprop(e->window, e->atom);
		c->state = state? *state : WithdrawnState;
		xfree(state);
	} else if (e->atom == net_wm_name) {
		xfree(c->name);
		c->name = getprop(c->window, e->atom);
		c->ewmh.name = 1;
	} else if (e->atom == net_wm_icon_name) {
		xfree(c->iconname);
		c->iconname = getprop(c->window, e->atom);
		c->ewmh.iconname = 1;
	} else if (e->atom == wm_protocols) {
		syncproto(c);
	}
}

void
eunmap(XUnmapEvent *e)
{
	Client *c, *p;
	int revert;

	debug("eunmap: %#x (%d)", (int)e->window, e->send_event);
	/* ICCCM 2.0, 4.1.4 */
	if (e->send_event) {
		revert = (current && current->window == e->window);
		c = getclient(e->window, 1);
		if (c == nil) {
			return;
		}
		if (c->frame) {
			withdrawn(c);
		}
		if (revert) {
			p = getclient(c->trans, 1);
			if (p == nil) {
				p = current;
			}
			active(p);
		}
		add(&limbo, c);
	}
}
