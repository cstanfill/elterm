CFLAGS = -Wall -Wextra -O0 -std=gnu11 -pedantic -g `pkg-config --cflags freetype2`
CC = gcc
INCLUDES = -I/usr/include/freetype2
ALL_OBJS = bin/buffer.o bin/config.o bin/display.o bin/main.o bin/pty.o
BINDIR = bin

TARGETS = elterm

LIBS = -lutil -lX11 -lXft -lXext

elterm: bindir Makefile.tail $(ALL_OBJS)
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(LIBS) $(ALL_OBJS)

bindir:
	mkdir -p $(BINDIR)

clean:
	rm -f -- bin/*

.PHONY: Makefile.tail
Makefile.tail:
	build/gentail.sh src/*.c $(INCLUDES) > Makefile.tail

include Makefile.tail
