#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#ifndef PASSWD
# define PASSWD	"1234"
#endif

#define nil ((void *)0)

enum {
	BorderWidth = 1,
	PasswdLen = sizeof(PASSWD) - 1,
};

enum {
	ColorStart,
	ColorTyping,
	ColorFailed,
	NColors,
};

char *colornames[NColors] = {
	"#112233",
	"grey",
	"red",
};

unsigned long color[NColors];

Display *dpy;
int screen;
Window win;

void
setborder(unsigned long pixel)
{
	XSetWindowAttributes attr;

	attr.border_pixel = pixel;
	XChangeWindowAttributes(dpy, win, CWBorderPixel, &attr);
}

Cursor
invisiblecursor(void)
{
	Pixmap none;
	XColor nothing;
	char zero = 0;

	none = XCreateBitmapFromData(dpy, RootWindow(dpy, screen), &zero, 1, 1);
	return XCreatePixmapCursor(dpy, none, none, &nothing, &nothing, 0, 0);
}

void
lockwin(void)
{
	Window root;
	XSetWindowAttributes attr;
	int width, height, mask;

	root = RootWindow(dpy, screen);
	width = DisplayWidth(dpy, screen) - BorderWidth * 2;
	height = DisplayHeight(dpy, screen) - BorderWidth * 2;

	mask = CWOverrideRedirect | CWBackPixel | CWBorderPixel | CWCursor;
	attr.override_redirect = True;
	attr.background_pixel = color[ColorStart];
	attr.border_pixel = color[ColorStart];
	attr.cursor = invisiblecursor();

	win = XCreateWindow(dpy, root, 0, 0, width, height, BorderWidth, DefaultDepth(dpy, screen),
		CopyFromParent, DefaultVisual(dpy, screen), mask, &attr);
	XMapRaised(dpy, win);
	
	XGrabPointer(dpy, root, False, ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
			GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
	XGrabKeyboard(dpy, root, True, GrabModeAsync, GrabModeAsync, CurrentTime);

	/* spy root for MapNotify: */
	XSelectInput(dpy, root, SubstructureNotifyMask);

	XFlush(dpy);
}

void
eventloop(void)
{
	XEvent e;
	KeySym ksym;
	char input[PasswdLen+1], buf[32];
	int i, n, locked;

	locked = True;
	i = 0;
	while (locked) {
		XNextEvent(dpy, &e);
		if (e.type == KeyPress) {
			n = XLookupString(&e.xkey, buf, sizeof(buf), &ksym, nil);
			if (n == 0) {
				continue;
			}
			switch (ksym) {
			case XK_Escape:
				i = 0;
				setborder(color[ColorStart]);
				break;
			case XK_BackSpace:
				if (i > 0) {
					i--;
					if (i == 0) {
						setborder(color[ColorStart]);
					}
				}
				break;
			case XK_Return:
				input[i] = 0;
				if (i != PasswdLen || memcmp(input, PASSWD, PasswdLen) != 0) {
					i = 0;
					setborder(color[ColorFailed]);
					XFlush(dpy);
					usleep(75e4);
					XSync(dpy, True);
					setborder(color[ColorStart]);
					break;
				} else { // input was ok: unlock the screen.
					locked = False;
				}
				break;
			default:
				if (i+n > sizeof(input)) {
					continue;
				}
				if (i == 0) {
					setborder(color[ColorTyping]);
				}
				memcpy(input+i, buf, n);
				i += n;
				break;
			}
		} else if (e.type == MapNotify) {
			// another client want to be mapped: lock window have to stay up:
			XRaiseWindow(dpy, win);
			XFlush(dpy);
		}
	}
}

int
main(void)
{
	XColor c;
	int i;

	dpy = XOpenDisplay(nil);
	if (dpy == nil) {
		fprintf(stderr, "could not open display\n");
		return 1;
	}
	screen = DefaultScreen(dpy);

	for (i=0; i<NColors; i++) {
		XAllocNamedColor(dpy, DefaultColormap(dpy, screen), colornames[i], &c, &c);
			color[i] = c.pixel;
	}

	lockwin();
	eventloop();

	XDestroyWindow(dpy, win);
	XCloseDisplay(dpy);
	return 0;
}
