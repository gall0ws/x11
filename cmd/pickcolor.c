#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>

#define nil ((void *)0)

Display *dpy;
Window root;
char *argv0;

void
die(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	fprintf(stderr, "%s: ", argv0);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	exit(1);
}

void
printcolor(int x, int y)
{
	XImage *img;
	unsigned long pixel;

	img = XGetImage(dpy, root, x, y, 1, 1, 0xFFFFFF, XYPixmap);
	if (img == nil) {
		die("could not get image from root.\n");
	}
	pixel = XGetPixel(img, 0, 0);
	printf("%06lX\n", pixel);
}

int
pick(int *x, int *y)
{
	XEvent e;
	Cursor cur;
	int res;

	cur = XCreateFontCursor(dpy, XC_tcross);
	res = XGrabPointer(dpy, root, False, ButtonPressMask, 
		GrabModeSync, GrabModeAsync, root, cur, CurrentTime);
	if (res != 0) {
		die("could not grab pointer.\n");
	}
	XAllowEvents(dpy, SyncPointer, CurrentTime);
	XNextEvent(dpy, &e);
	if (e.xbutton.button == Button1) {
		*x = e.xbutton.x_root;
		*y = e.xbutton.y_root;
	} else {
		res = -1;
	}
	XUngrabPointer(dpy, CurrentTime);
	return res;
}

int
main(int argc, char *argv[])
{
	int i, xy[2];
	char *p;
	
	argv0 = argv[0];
	argv++, argc--;
	if (argc == 1 || argc > 2) {
		fprintf(stderr, "usage: %s [x y]\n", argv0);
		return 1;
	}
	dpy = XOpenDisplay(nil);
	if (dpy == nil) {
		die("could not open display.\n");
	}
	root = DefaultRootWindow(dpy);
	if (argc == 0) {
		if (pick(xy, xy+1) != 0) {
			return 0;
		}
	} else {
		for (i=0; i<2; i++) {
			xy[i] = strtol(argv[i], &p, 10);
			if (*p != 0) {
				die("invalid number: %s\n", argv[i]);
			}
		}
	}
	printcolor(xy[0], xy[1]);
	return 0;
}
