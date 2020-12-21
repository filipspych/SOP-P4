/*
* Oświadczam, że niniejsza praca stanowiąca podstawę 
* do uznania osiągnięcia efektów uczenia się z przedmiotu 
* SOP została wykonana przezemnie samodzielnie.
* Filip Spychala 305797
*/

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>

#define DEFAULT_LAPS 1
#define MAX_RACER_NAME_LENGTH 255
#define RACER_DEFAULT_NAME_LENGTH 7
#define MAX_FILEPATH_LEN 255
#define MIN_RACER_COUNT 2
#define MAX_RACER_COUNT 8
#define MIN_LAP_LEN 100
#define MAX_LAP_LEN 5000
#define MIN_LAP_COUNT 1
#define MAX_LAP_COUNT 5
#define ERR(source) (perror(source),\
             fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
             exit(EXIT_FAILURE))
#define FILE_INPUT_BUFF_SIZE MAX_RACER_NAME_LENGTH

typedef struct Race_s {
    int laps;
    double lapLen;
} Race;

typedef struct Racer_s {
    pthread_t mainThreadID;
    double racerID;
    char name[MAX_RACER_NAME_LENGTH];
    double v;
    Race *race;
} Racer_arg;

void *racer_t(void *args);

void readArguments(int argc, char **argv, int *racerCount, char *inputFilePath,
                   int *lapLen, int *lapCount, char *outputFilePath);

void validateArgs(int threadCount, const char *inputFilePath, int lapLen, int lapCount);

void welcome();

void usage();

void badArgFor(char opt);

void badArg();

void giveNames(char *path, int racerCount, Racer_arg *racers);

bool isUserInput();

void readNamesFromFile(int fd, int racerCount, Racer_arg racers[MAX_RACER_COUNT]);

void generateNames(int racerCount, Racer_arg racers[MAX_RACER_COUNT]);

double getRandomDouble(double min, double max);

void initRacers(char *path, Racer_arg racers[MAX_RACER_COUNT], int count, int lapLen);

void sendSigusr1(pthread_t target);

int main(int argc, char **argv) {
    int racerCount, lapLen, lapCount;
    char inputFilePath[MAX_FILEPATH_LEN + 1], outputPath[MAX_FILEPATH_LEN + 1];
    Racer_arg racers[MAX_RACER_COUNT];
    srand(time(NULL));

    readArguments(argc, argv, &racerCount, inputFilePath, &lapLen, &lapCount, outputPath);
    welcome();

    initRacers(inputFilePath, racers, racerCount, lapLen);

    for (int i = 0; i < racerCount; i++) {
        pthread_t ignore;
        int err = pthread_create(&ignore, NULL, racer_t, (void *) &racers[i]);
        if (err != 0) ERR("Couldn't create thread");
    }
    return EXIT_SUCCESS;
}

void initRacers(char *path, Racer_arg racers[MAX_RACER_COUNT], int count, int lapLen) {
    for (int i = 0; i < count; ++i) {
        racers[i].racerID = i;
        racers[i].v = getRandomDouble(0.095, 0.105);
    }

    giveNames(path, count, racers);
}

double getRandomDouble(double min, double max) {
    return ((min) + (double) rand() / RAND_MAX * (max - min));
}

void giveNames(char *path, int racerCount, Racer_arg *racers) {
    int fd;
    if (path[0] != NULL) {
        if ((fd = open(path, O_RDONLY)) == -1) ERR("open");
    } else if (isUserInput()) {
        fd = STDIN_FILENO;
    } else {
        generateNames(racerCount, racers);
        return;
    }
    readNamesFromFile(fd, racerCount, racers);
}

void generateNames(int racerCount, Racer_arg racers[MAX_RACER_COUNT]) {
    char name[RACER_DEFAULT_NAME_LENGTH + 1] = "Racer_arg 1";
    for (int i = 0; i < racerCount; i++) {
        strcpy(racers[i].name, name);
        name[RACER_DEFAULT_NAME_LENGTH]++;
    }
}

void readNamesFromFile(int fd, int racerCount, Racer_arg racers[MAX_RACER_COUNT]) {
    char *buf;
    if ((buf = (char *) malloc(sizeof(char) * FILE_INPUT_BUFF_SIZE)) == NULL)
        ERR("Memory allocation error");

    for (int i = 0; i < racerCount; i++) {
        char tmp[MAX_RACER_NAME_LENGTH];
        FILE *file = fdopen(fd, "r"); //todo delete this
        fscanf(file, "%s\n", tmp); // todo change this
        // TODO: low level IO
//        while(read(fd, buf, FILE_INPUT_BUFF_SIZE)) {
//
//        }
        strcpy(racers[i].name, tmp);
    }
}

bool isUserInput() {
    printf("Do you want to name your racers? [y/n]: ");
    char c;
    scanf("%c", &c);
    return 'y' == c;
}

void welcome() {
    char *name = getenv("USER");
    printf("Hello %s!\n", name);
}

void readArguments(int argc, char **argv, int *racerCount, char *inputFilePath,
                   int *lapLen, int *lapCount, char *outputFilePath) {
    *racerCount = 0;
    inputFilePath[0] = NULL;
    *lapLen = 0;
    *lapCount = DEFAULT_LAPS;
    outputFilePath[0] = NULL;
    opterr = 0;

    char c;
    while ((c = getopt(argc, argv, "n:p:l:o:f:")) != -1) {
        switch (c) {
            case 'n':
                *racerCount = atoi(optarg);
                break;
            case 'p':
                strcpy(*inputFilePath, optarg);
                break;
            case 'l':
                *lapLen = atoi(optarg);
                break;
            case 'o':
                *lapCount = atoi(optarg);
                break;
            case 'f':
                strcpy(*outputFilePath, optarg);
                break;
            case '?':
                if (strchr("nplof", optopt) != NULL)
                    fprintf(stderr, "No argument for -%c given!\n", optopt);
                else fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                usage();
            default:
                ERR("Could not interpret supplied arguments");
        }
    }
    validateArgs(*racerCount, *inputFilePath, *lapLen, *lapCount);
}

void validateArgs(int threadCount, const char *inputFilePath, int lapLen, int lapCount) {
    if (threadCount > MAX_RACER_COUNT) badArgFor('n');
    if (threadCount < MIN_RACER_COUNT && inputFilePath == NULL) badArg();
    if (threadCount >= MIN_RACER_COUNT && threadCount <= MAX_RACER_COUNT && inputFilePath != NULL) badArg();
    if (lapLen < MIN_LAP_LEN || lapLen > MAX_LAP_LEN) badArgFor('l');
    if (lapCount < MIN_LAP_COUNT || lapCount > MAX_LAP_COUNT) badArgFor('o');
}

void usage() {
    fprintf(stderr, "USAGE:\n"
                    "-n - liczba wątków/uczestników wyścigu [2-8]\n"
                    "-p PATH - wywoływany zamiennie z parametrem -n, określa ścieżkę do pliku,\n"
                    "z którego będą wczytywani uczestnicy\n"
                    "-l - długość trasy [100-5000]\n"
                    "-o -opcjonalny parametr - liczba okrążeń [1-5], gdy nie jest podany, domyślnie\n"
                    "jest jedno okrążenie\n"
                    "-f PATH - opcjonalny parametr - ścieżka, do której zapisane zostaną wyniki\n"
                    "wyścigu\n");
    exit(EXIT_FAILURE);
}

void badArgFor(char opt) {
    fprintf(stderr, "Bad argument for -%c given!\n", opt);
    usage();
}

void badArg() {
    fprintf(stderr, "Bad arguments!\n");
    usage();
}

void *racer_t(void *args) {
    Racer_arg *thisRacer = args;

    double dist = 0;
    int completedLaps = 0;
    while (true) {
        sleep(1);
        dist += getRandomDouble(0.9, 1.1) * thisRacer->v;
        int curCompletedLaps;
        if ((curCompletedLaps = dist / thisRacer->race->lapLen) > completedLaps) {
            completedLaps = curCompletedLaps;
            sendSigusr1(thisRacer->mainThreadID);
            if (completedLaps >= thisRacer->race->laps) return NULL;
        }
    }
}

void sendSigusr1(pthread_t target) {
    pthread_kill(target, SIGUSR1);
}
