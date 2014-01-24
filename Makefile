CFLAGS = -Wall -Wextra -O2 -std=c11 -pedantic -g `pkg-config --cflags freetype2`
CC = gcc
INCLUDES = -I/usr/include/freetype2
ALL_OBJS = main.o pty.o 

TARGETS = anterm

LIBS = -lX11 -lXft

anterm: $(ALL_OBJS)
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(LIBS) $(ALL_OBJS) 

clean:
	rm -f -- $(ALL_OBJS) $(TARGETS)
