CFLAGS = -Wall -Wextra -O2 -std=gnu11 -pedantic -g `pkg-config --cflags freetype2`
CC = gcc
INCLUDES = -I/usr/include/freetype2
ALL_OBJS = main.o pty.o config.o display.o buffer.c
ALL_SRCFILES = main.c display.c pty.c config.c buffer.c

TARGETS = elterm

LIBS = -lutil -lX11 -lXft

elterm: $(ALL_OBJS) Makefile.tail
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(LIBS) $(ALL_OBJS) 

clean:
	rm -f -- $(ALL_OBJS) $(TARGETS)

.PHONY: Makefile.tail
Makefile.tail:
	gcc -MM $(ALL_SRCFILES) > Makefile.tail

include Makefile.tail
