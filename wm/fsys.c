#define FUSE_USE_VERSION 26

#include <errno.h>
#include <string.h>

#include <fuse.h>

#include "dat.h"
#include "fns.h"


typedef struct fuse_fill_dir_t filldir;
typedef struct fuse_file_info finfo;

static
int
fsgetattr(const char *path, struct stat *buf)
{
	memset(buf, 0, sizeof(*buf));
	if (strcmp(path, "/") == 0) {
		buf->st_mode = S_IFDIR | 0555;
		buf->st_nlink = 2;
	} else if (strcmp(path, "/index") == 0) {
		buf->st_mode = S_IFREG | 0444;
		buf->st_size = sizeof("suca");
	} else {
		return -ENOENT;
	}
	return 0;
}

static
int
fsreaddir(const char *path, void *buf, fuse_fill_dir_t fn, off_t offset, finfo *fi)
{
	USED(offset);
	USED(fi);

	if (strcmp(path, "/") != 0) {
		return -ENOENT;
	}
	fn(buf, ".", nil, 0);
	fn(buf, "..", nil, 0);	
	fn(buf, "index", nil, 0);

	return 0;
}

static
int
fsopen(const char *path, finfo *fi)
{
	USED(fi);

	if (strcmp("/index", path) != 0) {
		return -ENOENT;
	}
	if ((fi->flags & 3) != O_RDONLY) {
		return -EACCES;
	}
	return 0;
}

static
int
fsread(const char *path, char *buf, size_t sz, off_t offset, finfo *fi)
{
	ssize_t ret;
	

	USED(sz);
	USED(offset);
	USED(fi);
	if (strcmp("/index", path) != 0) {
		return -ENOENT;
	}
	ret = sizeof("suca");
	memcpy(buf, "suca", ret);
	return ret;
}

static struct fuse_operations fsops = {
	.getattr = fsgetattr,
	.readdir = fsreaddir,
	.open    = fsopen,
	.read    = fsread,
};


int
fsinit(char *mntpnt)
{
	// return fuse_main
	USED(mntpnt);
	USED(fsops);

	return 0;
}

