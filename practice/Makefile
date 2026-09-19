# Cross-build the userspace test apps for the STM32MP157 board.
#
#   make                 build every app into build/
#   make install         copy them to $(DESTDIR)$(BINDIR) (NFS rootfs by default)
#   make clean           remove build/
#
# Override the toolchain or the install path from the command line, e.g.
#   make DESTDIR=/srv/nfs/rootfs BINDIR=/root install

CROSS_COMPILE ?= arm-none-linux-gnueabihf-
CC = $(CROSS_COMPILE)gcc
CFLAGS ?= -O2 -Wall

DESTDIR ?= /srv/nfs/rootfs
BINDIR ?= /root

OUT = build
APPS = ledApp beepApp
BINS = $(addprefix $(OUT)/,$(APPS))

all: $(BINS)

$(OUT)/%: %.c | $(OUT)
	$(CC) $(CFLAGS) $< -o $@

$(OUT):
	mkdir -p $@

install: $(BINS)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(BINS) $(DESTDIR)$(BINDIR)

clean:
	rm -rf $(OUT)

.PHONY: all install clean
