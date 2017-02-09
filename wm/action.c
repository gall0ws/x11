#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "dat.h"
#include "fns.h"

static char *cmdterm[] =	{ "urxvtc", nil };
static char *cmdrun[] =		{ "dmenu_run",  "-l", "5", "-fn", "terminus", nil };
static char *cmdnew[] =		{ "9term", "-f", "/usr/local/plan9/font/vga/vga.font", nil };

static
void
spawn(char **argv)
{
	pid_t pid;

	debug("forking for %s", argv[0]);
	pid = fork();
	if (pid <0) {
		die("could not fork:");
	} else if (pid == 0) { // parent
		if (execvp(*argv, argv) <0) {
			err("exec failed:");
		}
	}
}

void
aterm(void)
{
	spawn(cmdterm);
}

void
arun(void)
{
	spawn(cmdrun);
}

void
anew(void)
{
	spawn(cmdnew);
}

static
void
resize_move(int resize)
{
	Window w;
	Client *c;
	Rect r;
	int res;

	w = upickw(Button3);
	if (w == None || w == root) {
		return;
	}
	if ((c = getclient(w, 0)) == nil) {
		return;
	}
	if (resize) {
		if (upickxy(Button3, &r.x, &r.y) != 0) {
			return;
		}
		r.dx = 1;
		r.dy = 1;
		res = uresize(transwin, c->aratio, -1, Button3, Button1, &r);
	} else {
		memcpy(&r, &c->r, sizeof(r));
		res = umove(transwin, Button3, &r);
	}
	XUnmapWindow(dpy, transwin);
	if (res != 0) {
		return;
	}
	geom(c, &r, 0);
	active(c);
}

void
aresize(void)
{
	resize_move(1);
}

void
amove(void)
{
	resize_move(0);
}

void
adelete(void)
{
	Window w;
	Client *c;

	w = upickw(Button3);
	if (w == None || w == root) {
		return;
	}
	c = getclient(w, 0);
	if (c == nil) {
		XKillClient(dpy, w);
		return;
	}
	if (c->proto.deletewin) {
		sendmsg(c->window, wm_delete_window);
	} else {
		XDestroyWindow(dpy, c->window);
	}
}

void
ahide(void)
{
	Window w;
	Client *c, *t;

	w = upickw(Button3);
	if (w == None || w == root) {
		return;
	}
	c = getclient(w, 1);
	if (c == nil) {
		return;
	}
	unmap(c);
	add(&hiddens, c);
	while ((t = getclient(c->trans, 1))) {
		add(&limbo, t);
		unmap(t);
		c = t;
	}
	active(current);
}

void
aswitchvirt(int v)
{
	debug("switching to virtual %d", v);
	apply(current, unmap);
	curvirt = v;
	apply(current, map);
	active(current);
	virtmenu.lasthit = v;
}

/* switch to ith client and return the next candidate's index */
static
int
switchto(int i)
{
	Client *c;

	if (current == nil || current->next == nil) {
		return -1;
	}
loop:
	c = lookup2(&current, i, 1);
	if (c == nil) {
		/* end of the list, retry */
		--i;
		goto loop;
	}
	wraise(c);
	return i+1;
}

void
aswitchwin(void)
{
	int res, next, doswitch;
	XEvent e;
	KeySym ksym;

	res = XGrabKeyboard(dpy, root, False, GrabModeAsync, GrabModeAsync, CurrentTime);
	if (res != GrabSuccess) {
		err("aswitchwin: could not grab keyboard");
		return;
	}
	next = 1;
	doswitch = 1;
	for (;;) {
		if (doswitch) {
			next = switchto(next);
			doswitch = 0;
			if (next < 0) {
				goto quit;
			}
		}
		XWindowEvent(dpy, root, KeyMask | SubstructureNotifyMask, &e);
		switch (e.type) {
		case DestroyNotify:
			/* avoid to switch to "ghost" windows */
			edestroy(&e.xdestroywindow);
			break;
		case KeyPress:
		case KeyRelease:
			XLookupString(&e.xkey, nil, 0, &ksym, nil);
			switch (ksym) {
			case XK_Alt_L:
				if (e.type == KeyRelease) {
					goto quit;
				}
				break;
			case XK_Tab:
				if (e.type == KeyPress) {
					doswitch = 1;
				}
				break;
			}
			break;
		}
	}
quit:
	XUngrabKeyboard(dpy, CurrentTime);
}
