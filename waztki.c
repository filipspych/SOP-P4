/*
* Oświadczam, że niniejsza praca stanowiąca podstawę 
* do uznania osiągnięcia efektów uczenia się z przedmiotu 
* SOP została wykonana przezemnie samodzielnie.
* Filip Spychala 305797
*/

#include <stdio.h>
#include <stdlib.h>
#define ERR(source) (perror(source),\
		     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
		     exit(EXIT_FAILURE))

int main(int argc, char** argv) {
	printf("Hello world\n");
	return EXIT_SUCCESS;
}