/*
* Oświadczam, że niniejsza praca stanowiąca podstawę 
* do uznania osiągnięcia efektów uczenia się z przedmiotu 
* SOP została wykonana przezemnie samodzielnie.
* Filip Spychala 305797
*/

#include <stdio.h>
#include <stdlib.h>
#define DEFAULT_LAPS 1
#define ERR(source) (perror(source),\
		     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
		     exit(EXIT_FAILURE))

void ReadArguments(int argc, char **argv, int *threadCount, char *inuptFilePath, 
int *trackLen, int *lapCount, char *outputPath);

int main(int argc, char** argv) {
	printf("Hello world\n");
	return EXIT_SUCCESS;
}

void welcome() {
    char *
}

void ReadArguments(int argc, char **argv, int *threadCount, char *inuptFilePath, 
int *trackLen, int *lapCount, char *outputPath) {
        *threadCount = DEFAULT_THREADCOUNT;
        *samplesCount = DEFAULT_SAMPLESIZE;
        if (argc >= 2) {
                *threadCount = atoi(argv[1]);
                if (*threadCount <= 0) {
                        printf("Invalid value for 'threadCount'");
                        exit(EXIT_FAILURE);
                }
        }
        if (argc >= 3) {
                *samplesCount = atoi(argv[2]);
                if (*samplesCount <= 0) {
                        printf("Invalid value for 'samplesCount'");
                        exit(EXIT_FAILURE);
                }
        }
}