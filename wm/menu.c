#include "dat.h"
#include "fns.h"

#define SLOTDY	(font->ascent + font->descent + PadY)

enum {
	Unselected,
	Selected,
};

enum {
	PadX 	= 8,
	PadY 	= 6,
	Border	= 2,
};

static Window 	win;
static GC 	gctext[2];
static GC 	gcslot[2];
static int 	windx;
static int 	windy;

static XFontStruct *font;
static char *face = MENUFACE;

void
initmenu(void)
{
	int mask;
	XSetWindowAttributes attr;
	XGCValues v;

	font = XLoadQueryFont(dpy, face);
	if (font == nil) {
		err("could not load font \"%s\"", face);
		font = XLoadQueryFont(dpy, "fixed");
		if (font == nil) {
			die("Not even fixed?? I give up...");
		}
	}

	mask = CWBorderPixel | CWOverrideRedirect;
	attr.border_pixel = MENUBR;
	attr.override_redirect = 1;
	win = XCreateWindow(dpy, root, 0, 0, 1, 1, Border, 
	    CopyFromParent, CopyFromParent, CopyFromParent,
	    mask, &attr);


	mask = GCForeground | GCFont;
	v.font = font->fid;
	v.foreground = MENUTXTFG_U;
	gctext[Unselected] = XCreateGC(dpy, win, mask, &v);
	v.font = font->fid;
	v.foreground = MENUTXTFG_S;
	gctext[Selected] = XCreateGC(dpy, win, mask, &v);

	mask = GCForeground;
	v.foreground = MNUSLOTFG_U;
	gcslot[Unselected] = XCreateGC(dpy, win, mask, &v);
	v.foreground = MENUSLOTFG_S;
	gcslot[Selected] = XCreateGC(dpy, win, mask, &v);
}

static
void
drawslot(Menu *m, int i, int on)
{
	int len;
	int textdx;
	char *s;

	s = m->item ? m->item[i] : m->gen(i);
	len = strlen(s);
	textdx = XTextWidth(font, s, len);
	XFillRectangle(dpy, win, gcslot[on], 0, i*SLOTDY, windx,  SLOTDY);
	XDrawString(dpy, win, gctext[on],
	    (windx-textdx)/2, i*SLOTDY + PadY/2 + font->ascent, s, len);
}

static
void
show(Menu *m)
{
	int offsetx, offsety;
	int mx, my;
	int x, y;
	int tmp;

	offsetx = 0;
	offsety = 0;
	mousexy(root, &mx, &my);
	x = mx - (windx + 2*Border)/2;
	y = my - m->lasthit*SLOTDY - SLOTDY/2;
	if (x < 0) {
		offsetx = -x;
		x = 0;
	}
	if (y < 0) {
		offsety = -y;
		y = 0;
	}
	if (x + windx + 2*Border > rootdx) {
		tmp = rootdx - (windx + 2*Border);
		offsetx = tmp - x;
		x = tmp;
	}
	if (y + windy + 2*Border > rootdy) {
		tmp = rootdy - (windy + 2*Border);
		offsety = tmp - y;
		y = tmp;
	}
	if (offsetx || offsety) {
		XWarpPointer(dpy, None, None, 0, 0, 0, 0, offsetx, offsety);
	}
	XMoveResizeWindow(dpy, win, x, y, windx, windy);
	XMapRaised(dpy, win);
}

static
int
selectitem(Menu *m, int x, int y, int last)
{
	int i;

	if (x < 0 || y < 0 || x >= windx || y >= windy) {
		i = -1;
	} else {
		i = y/SLOTDY;
	}
	if (last == i) {
		return i;
	}
	if (last != -1) {
		drawslot(m, last, 0);
	}
	if (i != -1) {
		drawslot(m, i, 1);
	}
	return i;
}

/* returns m's number of items */
static
int
prepare(Menu *m)
{
	char *s;
	int maxdx;
	int textdx;
	int i;

	if (!m->item && !m->gen) {
		return 0;
	}
	maxdx = 0;
	i = 0;
	while ((m->item && (s = m->item[i]) != nil) || (m->gen && (s = m->gen(i)) != nil)) {
		textdx = XTextWidth(font, s, strlen(s));
		maxdx = max(maxdx, textdx);
		i++;
	}
	windx = maxdx + PadX;
	windy = SLOTDY * i;
	return i;
}



int
menuhit(Menu *m, int but)
{
	XEvent e;
	int i, retv;
	int n;

	n = prepare(m);
	if (n == 0) {
		return -1;
	}
	show(m);
	if (grab(win, None) != 0) {
		err("could not grab win");
		retv = -1;
		goto quit;
	}
	retv = m->lasthit;
	for (i=0; i<n; i++) {
		drawslot(m, i, i==retv);
	}
	for (;;) {
		XWindowEvent(dpy, win, GrabMask, &e);
		switch (e.type) {
		case ButtonPress:
			/* ignore */
			break;
		case ButtonRelease:
			if (e.xbutton.button == but) {
				ungrab();
				goto quit;
			}
			break;
		case MotionNotify:
			retv = selectitem(m, e.xmotion.x, e.xmotion.y, retv);
			usleep(5000);
			discard(win, PointerMotionMask, nil);
			break;
		}
	}
quit:
	XUnmapWindow(dpy, win);
	return retv;
}
