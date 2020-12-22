CC=gcc
CFLAGS= -std=gnu99 -Wall -Wextra -Wshadow
LDLIBS = -lpthread -lm

all: wyscigi
wyscigi: wyscigi.c
.PHONY: clean all
clean:
	rm wyscigi
