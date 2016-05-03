#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include "dat.h"
#include "fns.h"

Window
upickw(int but)
{
	Window w;
	XEvent e;

	if (grab(root, cursor[PickWin]) != 0) {
		err("upickw: could not grab pointer");
		return -1;
	}
	XMaskEvent(dpy, ButtonPressMask, &e);
	w = (e.xbutton.button == but)? e.xbutton.subwindow : None;
	ungrab();
	return w;
}

int
upickxy(int but, int *x, int *y)
{
	XEvent e;
	int retv;

	retv = -1;
	if (grab(root, cursor[PickPoint]) != 0) {
		err("upickxy: could not grab pointer");
		return retv;
	}
	XMaskEvent(dpy, ButtonPressMask, &e);
	if (e.xbutton.button == but) {
		*x = e.xbutton.x_root;
		*y = e.xbutton.y_root;
		retv = 0;
	}
	ungrab();
	return retv;
}

static
int
move(Window w, int but, Rect *r)
{
	XEvent e;
	int x, y;

	mousexy(root, &x, &y);
	for (;;) {
		XWindowEvent(dpy, w, GrabMask, &e);
		switch (e.type) {
		case ButtonPress:
			/* ignored */
			break;
		case ButtonRelease:
			if (e.xbutton.button != but) {
				return -1;
			}
			return 0;
		case MotionNotify:
			r->x += e.xmotion.x_root - x;
			r->y += e.xmotion.y_root - y;
			x = e.xmotion.x_root;
			y = e.xmotion.y_root;
			XMoveWindow(dpy, w, r->x, r->y);
			usleep(10000);
			if (discard(w, PointerMotionMask, &e) > 0) {
				XSendEvent(dpy, w, False, 0, &e);
			}
			break;
		}
	}
}

int
umove(Window w, int but, Rect *r)
{
	int retv;

	XMoveResizeWindow(dpy, w, r->x, r->y, r->dx, r->dy);
	XMapRaised(dpy, w);
	if (grab(w, cursor[MoveWin]) != 0) {
		err("umove: could not grab pointer");
		return -1;
	}
	retv = move(w, but, r);
	ungrab();
	return retv;
}

static
void
warpptr(Window w, Rect *r, int edge)
{
	int x, y;
	
	mousexy(w, &x, &y);
	switch (edge) {
	case TopLeft:
		x = -BorderWidth;
		y = -BorderWidth;
		break;
	case Top:
		y = -BorderWidth;
		break;
	case TopRight:
		x = r->dx + BorderWidth;
		y = -BorderWidth;
		break;
	case Right:
		x = r->dx + BorderWidth;
		break;
	case BottomRight:
		x = r->dx + BorderWidth;
		y = r->dy + BorderWidth;
		break;
	case Bottom:
		y = r->dy + BorderWidth;
		break;
	case BottomLeft:
		x = -BorderWidth;
		y = r->dy + BorderWidth;
		break;
	case Left:
		x = -BorderWidth;
		break;
	}
	XWarpPointer(dpy, None, w, 0, 0, 0, 0, x, y);
	discard(w, PointerMotionMask, nil);
}

int
uresize(Window w, double aratio, int edge, int but, int mvbut, Rect *r)
{
	Cursor c, lastc;
	XEvent e;
	Rect tmp;
	int retv;

	XMoveResizeWindow(dpy, w, r->x, r->y, r->dx, r->dy);
	XMapRaised(dpy, w);
	if (grab(w, None) != 0) {
		err("uresize: could not grab pointer");
		return -1;
	}
	if (edge == -1) {
		edge = BottomRight;
	}
	c = cursor[edge];
	warpptr(w, r, edge);
	retv = 0;
	lastc = -1;
	for (;;) {
		if (c != lastc) {
			XDefineCursor(dpy, w, c);
			lastc = c;
		}
		XWindowEvent(dpy, w, GrabMask, &e);
		switch (e.type) {
		case ButtonPress:
			if (e.xbutton.button == mvbut) {
				XDefineCursor(dpy, w, cursor[MoveWin]);
				retv = move(w, mvbut, r);
				if (retv != 0) {
					goto quit;
				}
				lastc = -1;
				continue;
			}
			break;
		case ButtonRelease:
			if (e.xbutton.button != but) {
				retv = -1;
			}
			goto quit;
		}
		/* MotionNotify */
		memcpy(&tmp, r, sizeof(tmp));
		switch (edge) {
		case TopLeft:
			r->dx += r->x - e.xmotion.x_root;
			r->dy += r->y - e.xmotion.y_root;
			if (r->dx < 0 && r->dy < 0) {
				r->x = tmp.x + tmp.dx;
				r->dx = abs(r->dx);
				r->y = tmp.y + tmp.dy;
				r->dy = abs(r->dy);
				edge = BottomRight;
				c = cursor[edge];
			} else if (r->dx < 0) {
				r->x = tmp.x + tmp.dx;
				r->dx = abs(r->dx);
				r->y = e.xmotion.y_root;
				edge = TopRight;
				c = cursor[edge];
			} else if (r->dy < 0) {
				r->x = e.xmotion.x_root;
				r->y = tmp.y + tmp.dy;
				r->dy = abs(r->dy);
				edge = BottomLeft;
				c = cursor[edge];
			} else {
				r->x = e.xmotion.x_root;
				r->y = e.xmotion.y_root;
			}
			break;
		case Top:
			r->dy += r->y - e.xmotion.y_root;
			if (r->dy < 0) {
				r->y = tmp.y + tmp.dy;
				r->dy = abs(r->dy);
				edge = Bottom;
				c = cursor[edge];
			} else {
				r->y = e.xmotion.y_root;
			}
			break;
		case TopRight:
			r->dx = e.xmotion.x_root - r->x - 2*BorderWidth + 1;
			r->dy += r->y - e.xmotion.y_root;
			if (r->dx < 0 && r->dy < 0) {
				r->x = e.xmotion.x_root;
				r->dx = tmp.x - r->x;
				r->y = tmp.y + tmp.dy;
				r->dy = abs(r->dy);
				edge = BottomLeft;
				c = cursor[edge];
			} else if (r->dx < 0) {
				r->x = e.xmotion.x_root;
				r->dx = tmp.x - r->x;
				r->y = e.xmotion.y_root;
				edge = TopLeft;
				c = cursor[edge];
			} else if (r->dy < 0) {
				r->y = tmp.y + tmp.dy;
				r->dy = abs(r->dy);
				edge = BottomRight;
				c = cursor[edge];
			} else {
				r->y = e.xmotion.y_root;
			}
			break;
		case Right:
			r->dx = e.xmotion.x_root - r->x - 2*BorderWidth + 1;
			if (r->dx < 0) {
				r->x = e.xmotion.x_root;
				r->dx = tmp.x - r->x;
				edge = Left;
				c = cursor[edge];
			}
			break;
		case BottomRight:
			r->dx = e.xmotion.x_root - r->x - 2*BorderWidth + 1;
			r->dy = e.xmotion.y_root - r->y - 2*BorderWidth + 1;
			if (r->dx < 0 && r->dy < 0) {
				r->x = e.xmotion.x_root;
				r->dx = tmp.x - r->x;
				r->y = e.xmotion.y_root;
				r->dy = tmp.y - r->y;
				edge = TopLeft;
				c = cursor[edge];
			}
			if (r->dx < 0) {
				r->x = e.xmotion.x_root;
				r->dx = tmp.x - r->x;
				edge = BottomLeft;
				c = cursor[edge];
			} else if (r->dy < 0) {
				r->y = e.xmotion.y_root;
				r->dy = tmp.y - r->y;
				edge = TopRight;
				c = cursor[edge];
			}
			break;
		case Bottom:
			r->dy = e.xmotion.y_root - r->y - 2*BorderWidth + 1;
			if (r->dy < 0) {
				r->y = e.xmotion.y_root;
				r->dy = tmp.y - r->y;
				edge = Top;
				c = cursor[edge];
			}
			break;
		case BottomLeft:
			r->dx += r->x - e.xmotion.x_root;
			r->dy = e.xmotion.y_root - r->y - 2*BorderWidth + 1;
			if (r->dx < 0 && r->dy < 0) {
				r->x = tmp.x + tmp.dx;
				r->dx = abs(r->dx);
				r->y = e.xmotion.y_root;
				r->dy = tmp.y - r->y;
				edge = TopRight;
				c = cursor[edge];
			} else if (r->dy < 0) {
				r->x = e.xmotion.x_root;
				r->y = e.xmotion.y_root;
				r->dy = tmp.y - r->y;
				edge = TopLeft;
				c = cursor[edge];
			} else if (r->dx < 0) {
				r->x = tmp.x + tmp.dx;
				r->dx = abs(r->dx);
				edge = BottomRight;
				c = cursor[edge];
			} else {
				r->x = e.xmotion.x_root;
			}
			break;
		case Left:
			r->dx += r->x - e.xmotion.x_root;
			if (r->dx <= 0) {
				r->x = tmp.x + tmp.dx;
				r->dx = abs(r->dx);
				edge = Right;
				c = cursor[edge];
			} else {
				r->x = e.xmotion.x_root;
			}
			break;
		}
		/* FIXME: this works just when the aspect ratio is not specified */
		if (r->x + r->dx + 2*BorderWidth > rootdx) {
			r->dx = rootdx - r->x - 2*BorderWidth;
		}
		if (r->y + r->dy + 2*BorderWidth > rootdy) {
			r->dy = rootdy - r->y - 2*BorderWidth;
		}
		if (aratio) { 
			switch (edge) {
			case Bottom:
			case Top:
			case TopRight:
				r->dx = r->dy * aratio;
				break;
			case TopLeft:
				tmp.dx = r->dx;
				tmp.dy = r->dy;
				r->dx = r->dy * aratio;
				r->dy = r->dx / aratio;
				r->x += tmp.dx - r->dx;
				r->y += tmp.dy - r->dy;
				break;
			default:
				r->dy = r->dx / aratio;
			}
		}
		if (r->dx > 0 && r->dy > 0) {
			XMoveResizeWindow(dpy, w, r->x, r->y, r->dx, r->dy);
			usleep(10000);
			if (discard(w, PointerMotionMask, &e) > 0) {
				XSendEvent(dpy, w, False, 0, &e);
			}
		}
	}
quit:
	ungrab();
	return retv;
}
