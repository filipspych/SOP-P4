CC=gcc
CFLAGS= -std=gnu99 -Wall -Wextra -Wshadow -g3
LDLIBS = -lpthread -lm

all: wyscigi
wyscigi: wyscigi.c
.PHONY: clean all
clean:
	rm wyscigi