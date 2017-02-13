#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <X11/Xlib.h>

#include "config.h"

#define ButtonMask	ButtonPressMask | ButtonReleaseMask
#define KeyMask		KeyPressMask | KeyReleaseMask
#define CrossingMask	EnterWindowMask | LeaveWindowMask
#define RootMask	StructureNotifyMask | SubstructureRedirectMask |\
	SubstructureNotifyMask | ButtonMask
#define FrameMask	SubstructureRedirectMask | PointerMotionMask | CrossingMask
#define ClientMask 	StructureNotifyMask | PropertyChangeMask
#define GrabMask	ButtonMask | PointerMotionMask

#define current		(clients[curvirt])
#define nil		((void *)0)

enum {
	FocusClick,
	FocusFollowsMouse,
};

enum {
	BorderWidth 	= 4,
	NVirtuals 	= 4,
};

enum {
	Inactive,
	Active,
};

enum {
	New,
	Resize,
	Move,
	Stick,
	Delete,
	Hide,
	NActions
};

enum {
	Normal,
	TopLeft,
	Top,
	TopRight,
	Right,
	BottomRight,
	Bottom,
	BottomLeft,
	Left,
	PickWin,
	PickPoint,
	MoveWin,
	NCursors
};

typedef struct Client 	Client;
typedef struct Ewmh	Ewmh;
typedef struct Menu 	Menu;
typedef struct Proto	Proto;
typedef struct Rect 	Rect;
typedef unsigned char 	uchar;
typedef unsigned long 	ulong;

struct Rect {
	int	x;
	int	y;
	int	dx;
	int	dy;
};

struct Proto {
	int takefocus : 1;
	int losefocus : 1;
	int deletewin : 1;
};

struct Ewmh {
	int	name		: 1;
	int	iconname	: 1;
	int	fullscr		: 1;
	int	sticky		: 1;
	Rect	oldr;
};

struct Client {
	Window	window;
	Window	frame;
	Window	trans;
	Rect	r;
	Proto	proto;
	Ewmh	ewmh;
	double	aratio;
	int	state;
	int	setinput 	: 1;
	int	is9term  	: 1;
	int	hasfocus	: 1;
	int	overredir	: 1;
	char *rcname;
	char	*class;
	char	*name;
	char	*iconname;

	Client	*next;
};

struct Menu {
	char	**item;
	char	*(*gen)(int);
	int	lasthit;
};

Atom	net_active_window;
Atom	net_wm_icon_name;
Atom	net_wm_name;
Atom	net_wm_state;
Atom	net_wm_state_fullscreen;
Atom	net_wm_state_sticky;
Atom	ninewm_lose_focus;
Atom	wm_delete_window;
Atom	wm_protocols;
Atom	wm_state;
Atom	wm_take_focus;
Display	*dpy;
Window	root;
Window	nofocus;
Window	blankwin;
Window	transwin;
Menu	actionmenu;
Menu	virtmenu;
Menu	unhidemenu;
Cursor	cursor[NCursors];
Client	*clients[NVirtuals];
Client *stickies;
Client	*hiddens;
Client	*limbo;
ulong	bcolor[2];	/* inactive, active */
int	focuspolicy;
int	curvirt;
int	screen;
int	rootdx;
int	rootdy;
