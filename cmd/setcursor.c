#include <stdio.h>
#include <stdlib.h> /* strtol */
#include <X11/Xlib.h>
#include <X11/cursorfont.h>

#define nil ((void *)0)

int 
handler(Display *dpy, XErrorEvent *e)
{
	char buf[64];
	
	XGetErrorText(dpy, e->error_code, buf, sizeof(buf));
	fprintf(stderr, "%s\n", buf);
	exit(1);
}

int
main(int argc, char **argv)
{
	Display *dpy;
	Cursor cur;
	int shape;
	char *p;
	
	dpy = XOpenDisplay(nil);
	if (dpy == nil) {
		return 1;
	}
	XSetErrorHandler(handler);
	
	if (argc == 1) {
		shape = XC_left_ptr;
	} else {
		shape = strtol(argv[1], &p, 0);
		if (*p != 0) {
			fprintf(stderr, "invalid number: %s\n", argv[1]);
			return 1;
		}
	}
	cur = XCreateFontCursor(dpy, shape);
	XDefineCursor(dpy, DefaultRootWindow(dpy), cur);
	XSync(dpy, False);
	return 0;
}
