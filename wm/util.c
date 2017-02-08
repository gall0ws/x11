#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __FreeBSD__
#  define name	getprogname()
#else
static char *name;
#endif

int
max(int a, int b)
{
	return b>a? b : a;
}

int
min(int a, int b)
{
	return b<a? b : a;
}

int
mod(int a, int b)
{
	int y;

	if (b < 0) {
		return mod(a, -b);
	}
	y = a % b;
	if (y < 0) {
		y += b;
	}
	return y;
}

static
void
vfprint(FILE *fp, char *fmt, va_list ap)
{
	if (fp == stderr) {
		fflush(stdout);
	}
	if (name) {
		fprintf(fp, "%s: ", name);
	}
	vfprintf(fp, fmt, ap);
	if (fmt[0] != '\0' && fmt[strlen(fmt)-1] == ':') {
		fprintf(fp, " %s", strerror(errno));
	}
	fprintf(fp, "\n");
}

#define PRINT(FP, FMT)\
{\
	va_list ap;\
	va_start(ap, FMT);\
	vfprint(FP, FMT, ap);\
	va_end(ap);\
}

void
die_real(char *fmt, ...)
{
	PRINT(stderr, fmt);
	exit(2);
}

void
err_real(char *fmt, ...)
{
	PRINT(stderr, fmt);
}

void
debug_real(char *fmt, ...)
{
	PRINT(stdout, fmt);
	fflush(stdout);
}

void *
emalloc(size_t sz)
{
	void *p;

	p = calloc(1, sz);
	if (!p) {
		die_real("calloc: could not allocate %u bytes:", sz);
	}
	return p;
}

char *
estrdup(char *s)
{
	char *t;
	int n;

	if (!s) {
		return estrdup("");
	}
	n = strlen(s) + 1;
	t = emalloc(n);
	strcpy(t, s);
	return t;
}

#ifndef __FreeBSD__
void setprogname(char *s)
{
	free(name);
	name = estrdup(s);
}
#endif
