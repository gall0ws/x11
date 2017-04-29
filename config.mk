CC=cc
LD=cc
MAKE=gmake
STRIP=strip

X11ROOT=/usr/local

CPPFLAGS=-I$(X11ROOT)/include
CFLAGS=\
	-g\
	-Wall\
	-Wextra\
	-Werror\
	-Wno-format-zero-length\
	-Wno-missing-braces\
	-Wno-parentheses\
	-Wno-sign-compare
LDFLAGS=-L$(X11ROOT)/lib
LDLIBS=-lX11

PREFIX?=	$(HOME)
BINDIR=	$(PREFIX)/bin
