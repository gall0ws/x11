#include <signal.h>

#include <X11/XF86keysym.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>

#include "dat.h"
#include "fns.h"

static char *actionitem[NActions+1] = {
	"New",
	"Resize",
	"Move",
	"Delete",
	"Hide",
	nil,
};

static char *virtitem[NVirtuals+1] = {
	"One",
	"Two",
	"Three",
	"Four",
	nil,
};

static
int
error(Display *dpy, XErrorEvent *e)
{
	char msg[128];
	char buf[8];
	char req[32];

	if (e->request_code == X_ChangeWindowAttributes && e->error_code == BadAccess) {
		die("please, kill the other one...");
	}
	XGetErrorText(dpy, e->error_code, msg, sizeof(msg));
	snprintf(buf, sizeof(buf), "%d", e->request_code);
	XGetErrorDatabaseText(dpy, "XRequest", buf, "", req, sizeof(req));
	err("%s (%#x): %s", req, (int)e->resourceid, msg);
	return 0;
}

static
void
adoptclients(void)
{
	uint n, i;
	Window *win, dummy;
	XWindowAttributes attr;
	int *state;
	Client *c, **lst;
	Rect r;

	XGrabServer(dpy);
	debug("quering tree");
	if (!XQueryTree(dpy, root, &dummy, &dummy, &win, &n)) {
		err("could not get windows tree for root %#x", (int)root);
		goto quit;
	}
	debug("%d clients found", n);
	for (i=0; i<n; i++) {
		if (!XGetWindowAttributes(dpy, win[i], &attr)) {
			err("could not get window attributes for %#x", (int)win[i]);
			continue;
		}
		if (attr.class == InputOnly || attr.override_redirect) {
			debug("skipping %#x", (int)win[i]);
			continue;
		}
		debug("adopting %#x", (int)win[i]);
		c = newclient(win[i]);
		r.x = attr.x;
		r.y = attr.y;
		r.dx = attr.width;
		r.dy = attr.height;
		manage(c);
		geom(c, &r, 1);
		state = (int *)getprop(win[i], wm_state);
		c->state = state? *state : WithdrawnState;
		xfree(state);
		switch (c->state) {
		case WithdrawnState:
			lst = &limbo;
			break;
		case IconicState:
			lst = &hiddens;
			break;
		default:
			lst = &current;
			map(c);
		}
		add(lst, c);
	}
	active(current);
	xfree(win);
quit:
	XUngrabServer(dpy);
}

static
char *
unhidegen(int i)
{
	Client *c;

	/* The menu generation will run in O(n^2) and I should feel bad for that, but:
	 *	1. it's simple
	 *	2. the average case (n<20) is quite acceptable (ugly, but acceptable)
	 * 	3. worse is better.
	 */
	c = lookup2(&hiddens, i, 0);
	if (c == nil) {
		return nil;
	}
	return c->iconname? c->iconname : c->name? c->name : "(has no name)";
}

static
void
init(void)
{
	XVisualInfo vinfo;
	Colormap cmap;
	int mask;
	XSetWindowAttributes attr;
	KeySym mod;

	// atoms
	net_active_window = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
	net_wm_icon_name = XInternAtom(dpy, "_NET_WM_ICON_NAME", False);
	net_wm_name = XInternAtom(dpy, "_NET_WM_NAME", False);
	net_wm_state = XInternAtom(dpy, "_NET_WM_STATE", False);
	net_wm_state_fullscreen = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
	ninewm_lose_focus = XInternAtom(dpy, "_9WM_LOSE_FOCUS", False);
	wm_delete_window = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	wm_protocols = XInternAtom(dpy, "WM_PROTOCOLS", False);
	wm_state = XInternAtom(dpy, "WM_STATE", False);
	wm_take_focus = XInternAtom(dpy, "WM_TAKE_FOCUS", False);

	// frame colors
	bcolor[Active] = 0xFF57A8A8;
	bcolor[Inactive] = 0xFF9FEFEF;

	// creating blankwin
	mask = CWOverrideRedirect;
	attr.override_redirect = 1;
	nofocus = XCreateWindow(dpy, root, -10, -10, 1, 1, 0, 
	    0, InputOnly, CopyFromParent, mask, &attr);
	XMapWindow(dpy, nofocus);
	XSetInputFocus(dpy, nofocus, None, CurrentTime);
	mask = CWBackPixel | CWBorderPixel | CWOverrideRedirect;
	attr.background_pixel = 0xFFEFEFEF;
	attr.border_pixel = 0xFFAA0000;
	attr.override_redirect = 1;
	blankwin = XCreateWindow(dpy, root, 0, 0, 1, 1, BorderWidth,
	    CopyFromParent, InputOutput, CopyFromParent, mask, &attr);

	if (!XMatchVisualInfo(dpy, screen, 32, TrueColor, &vinfo)) {
		err("unsupported visual 32/TrueColor");
		transwin = blankwin;
	} else {
		cmap = XCreateColormap(dpy, root, vinfo.visual, AllocNone);
		mask = CWBackPixel | CWBorderPixel | CWOverrideRedirect | CWColormap;
		attr.background_pixel = 0x20000000;
		attr.border_pixel = 0xFFBB0000;
		attr.colormap = cmap;
		transwin = XCreateWindow(dpy, root, 0, 0, 1, 1, BorderWidth,
		    32, InputOutput, vinfo.visual, mask, &attr);
		XFreeColormap(dpy, cmap);
	}

	initcursor(0);
	initmenu();
	unhidemenu.gen = unhidegen;
	actionmenu.item = actionitem;
	virtmenu.item = virtitem;

	XGrabKey(dpy, XKeysymToKeycode(dpy, XF86XK_Launch0), AnyModifier,
	    root, False, GrabModeSync, GrabModeSync);
	XGrabKey(dpy, XKeysymToKeycode(dpy, XF86XK_Terminal), AnyModifier,
	    root, False, GrabModeSync, GrabModeSync);
	XGrabKey(dpy, XKeysymToKeycode(dpy, XK_Next_Virtual_Screen), AnyModifier,
             root, False, GrabModeSync, GrabModeSync);
 	XGrabKey(dpy, XKeysymToKeycode(dpy, XK_Prev_Virtual_Screen), AnyModifier,
             root, False, GrabModeSync, GrabModeSync);
	XGrabKey(dpy, XKeysymToKeycode(dpy, XK_Tab), AnyModifier, 
	    root, False, GrabModeSync, GrabModeSync);

	mod = XK_Alt_L;
	XRebindKeysym(dpy, XK_Tab, &mod, 1, (uchar *)"SWITCH", sizeof("SWITCH"));
	
	XGrabButton(dpy, Button1, AnyModifier, root, False, ButtonMask,
	    GrabModeSync, GrabModeAsync, None, None);
	XGrabButton(dpy, Button3, AnyModifier, root, False, ButtonMask,
	    GrabModeSync, GrabModeAsync, None, None);
}

int
main(void)
{
	XEvent e;

	setprogname("wm");
	if (signal(SIGCHLD, SIG_IGN) == SIG_ERR) {
		die("failed signal:");
	}
	XSetErrorHandler(error);
	dpy = XOpenDisplay(nil);
	if (dpy == nil) {
		die("could not open display");
	}
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);
	rootdx = DisplayWidth(dpy, screen);
	rootdy = DisplayHeight(dpy, screen);
	XSelectInput(dpy, root, RootMask);
	XSync(dpy, 0); /* exit here if another window manager is running */
	init();
	adoptclients();
	XSetWindowBackground(dpy, root, 0xFF808080);
	XClearWindow(dpy, root);
	XDefineCursor(dpy, root, cursor[Normal]);
	for (;;) {
		XNextEvent(dpy, &e);
		switch (e.type) {
		case ButtonPress:
			ebutton(&e.xbutton);
			break;
		case ClientMessage:
			eclimsg(&e.xclient);
			break;
		case ConfigureNotify:
			econfig(&e.xconfigure);
			break;
		case ConfigureRequest:
			econfigreq(&e.xconfigurerequest);
			break;
		case CreateNotify:
			ecreate(&e.xcreatewindow);
			break;
		case EnterNotify:
		case LeaveNotify:
			ecross(&e.xcrossing);
			break;
		case DestroyNotify:
			edestroy(&e.xdestroywindow);
			break;
		case KeyPress:
		case KeyRelease:
			ekey(&e.xkey);
			break;
		case MapRequest:
			emapreq(&e.xmaprequest);
			break;
		case MotionNotify:
			emotion(&e.xmotion);
			break;
		case PropertyNotify:
			eproperty(&e.xproperty);
			break;
		case UnmapNotify:
			eunmap(&e.xunmap);
			break;
		case ButtonRelease:
		case MapNotify:
		case MappingNotify:
		case ReparentNotify:
			/* ignored */
			break;
		default:
			debug("event loop: unexpected event %d", e.type);
		}
	}
	return 0;
}
