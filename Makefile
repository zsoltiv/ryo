SHELL = /bin/sh
.SUFFIXES:
.SUFFIXES: .c .o

CC ?= cc
CFLAGS ?= -g
CFLAGS += -std=c17
LDLIBS = `pkg-config --libs --cflags libavformat libavcodec libavutil`
PREFIX ?= /usr/local

all: ryo

ryo: ryo.o
	$(CC) $< $(CFLAGS) $(LDLIBS) $(LDFLAGS) -o $@

ryo.o: ryo.c
	$(CC) $< -c $(CFLAGS) -o $@

clean:
	rm -rf *.o ryo

.PHONY: clean
