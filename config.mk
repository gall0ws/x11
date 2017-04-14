CC=cc
LD=cc
MAKE=gmake
STRIP=strip

CPPFLAGS=-I/usr/local/include/
CFLAGS=\
	-g\
	-Wall\
	-Wextra\
	-Werror\
	-Wno-format-zero-length\
	-Wno-missing-braces\
	-Wno-parentheses\
	-Wno-sign-compare
LDFLAGS=-L/usr/local/lib/
LDLIBS=-lX11

PREFIX?=	$(HOME)
BINDIR=	$(PREFIX)/bin
