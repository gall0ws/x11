#define CHECKFMT	__attribute__((format (printf, 1, 2)))
#define MSG(fn, ...)\
{\
	fprintf(stdout, "%s: %s: ", getprogname(), __FUNCTION__);\
	fn(__VA_ARGS__);\
}
#define die(...)	MSG(die_real, __VA_ARGS__);
#define err(...)	MSG(err_real, __VA_ARGS__);

#ifdef DEBUG
# define	debug(...)	MSG(debug_real, __VA_ARGS__);
#else
# define	debug(x, ...)	USED(x)
#endif

#define USED(x)		((void)x)
#define clientid(c)	(c?(int)c->window:0)

/* action.c */
void aterm(void);
void arun(void);
void anew(void);
void aresize(void);
void amove(void);
void astick(void);
void adelete(void);
void ahide(void);
void aswitchvirt(int v);
void aswitchwin(void);

/* client.c */
Client *newclient(Window w);
void freeclient(Client *c);
void manage(Client *c);
void map(Client *c);
void unmap(Client *c);
void withdrawn(Client *c);
void wraise(Client *c);
void active(Client *c);
void fullscr(Client *c, int on);
void sticky(Client *c, int on);
void geom(Client *c, Rect *r, int winrect);
void synchints(Client *c);
void syncname(Client *c);
void syncproto(Client *c);
void synctrans(Client *c);
int edgeat(Client *c, int x, int y);
void dumpclients(void);

/* cursor.c */
void initcursor(int x11);

/* event.c */
void ebutton(XButtonEvent *e);
void eclimsg(XClientMessageEvent *e);
void econfig(XConfigureEvent *e);
void econfigreq(XConfigureRequestEvent *e);
void ecreate(XCreateWindowEvent *e);
void ecross(XCrossingEvent *e);
void edestroy(XDestroyWindowEvent *e);
void ekey(XKeyEvent *e);
void emapreq(XMapRequestEvent *e);
void emotion(XMotionEvent *e);
void eproperty(XPropertyEvent *e);
void eunmap(XUnmapEvent *e);

/* list.c */
void apply(Client *lst, void (*fn)(Client *));
Client *lookup(Client **lst, Window w, int rem);
Client *lookup2(Client **lst, int i, int rem);
Client *getclient(Window w, int rem);
void add(Client **lst, Client *c);

/* menu.c */
void initmenu(void);
int menuhit(Menu *m, int but);

/* user.c */
Window upickw(int but);
int upickxy(int but, int *x, int *y);
int umove(Window w, int but, Rect *r);
int uresize(Window w, double aratio, int edge, int but, int mvbut, Rect *r);

/* util.c */
int max(int a, int b);
int min(int a, int b);
int mod(int a, int b);
void die_real(char *fmt, ...) CHECKFMT;
void err_real(char *fmt, ...) CHECKFMT;
void debug_real(char *fmt, ...) CHECKFMT;
void *emalloc(size_t sz);
char *estrdup(char *s);
#ifndef __FreeBSD__
void setprogname(char *s);
#endif

/* xutil.c */
int discard(Window w, long mask, XEvent *last);
char *getprop(Window w, Atom a);
int sendmsg(Window w, Atom a);
int grab(Window w, Cursor c);
void ungrab(void);
void mousexy(Window w, int *x, int *y);
int xfree(void *data);

