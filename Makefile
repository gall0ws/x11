include config.mk

DIRS=\
	cmd\
	wm

all:
%:
	for i in $(DIRS); do $(MAKE) $@ -C $$i || exit 1; done;
