SHELL = /bin/sh
.SUFFIXES:
.SUFFIXES: .c .o

CC ?= cc
CFLAGS ?= -g
LDLIBS = `pkg-config --libs --cflags libavformat`
PREFIX ?= /usr/local

all: ryo

ryo: ryo.o
	$(CC) $< $(CFLAGS) $(LDLIBS) $(LDFLAGS) -o $@

ryo.o: ryo.c
	$(CC) $< -c $(CFLAGS) -o $@

clean:
	rm -rf *.o ryo

.PHONY: clean
