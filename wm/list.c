#include "dat.h"
#include "fns.h"

void
apply(Client *lst, void (*fn)(Client *))
{
	Client *tmp;

	while (lst) {
		tmp = lst;
		lst = lst->next;
		fn(tmp);
	}
}

static
Client *
_lookup(Client **lst, Window w, int i, int rem)
{
	Client *c, *p;

	if (*lst == nil) {
		return nil;
	}
	if (i == 0 || (*lst)->window == w || (*lst)->frame == w) {
		c = *lst;
		if (rem) {
			*lst = c->next;
		}
		return c;
	}
	for (p=*lst; p->next!=nil; p=p->next, i--) {
		if (i == 1 || p->next->window == w || p->next->frame == w) {
			c = p->next;
			if (rem) {
				p->next = c->next;
			}
			return c;
		}
	}
	return nil;
}

Client *
lookup(Client **lst, Window w, int rem)
{
	return _lookup(lst, w, -1, rem);
}

Client *
lookup2(Client **lst, int i, int rem)
{
	return _lookup(lst, -1, i, rem);
}

Client *
getclient(Window w, int rem)
{
	Client *c;
	int i;

	if (w == None) {
		return nil;
	}
	c = lookup(&limbo, w, rem);
	if (c) {
		return c;
	}
	for (i=0; i<NVirtuals; i++) {
		c = lookup(&clients[i], w, rem);
		if (c) {
			return c;
		}
	}
	c = lookup(&stickies, w, rem);
	if (c == nil) {
		c = lookup(&hiddens, w, rem);
	}
	return c;
}

void
add(Client **lst, Client *c)
{
	c->next = *lst;
	*lst = c;
}
