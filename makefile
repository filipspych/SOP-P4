all: wyscigi
wyscigi: wyscigi.c	
	gcc -Wall -o wyscigi wyscigi.c
.PHONY: clean all
clean:
	rm wyscigi