all: waztki
waztki: waztki.c	
	gcc -Wall -o waztki waztki.c
.PHONY: clean all
clean:
	rm waztki