CC=gcc
LD=gcc
MAKE=make
STRIP=strip

CPPFLAGS=
CFLAGS=\
	-g\
	-Wall\
	-Wextra\
	-Werror\
	-Wno-format-zero-length\
	-Wno-missing-braces\
	-Wno-parentheses\
	-Wno-sign-compare
LDFLAGS=
LDLIBS=-lX11

PREFIX?=	$(HOME)
BINDIR=	$(PREFIX)/bin
