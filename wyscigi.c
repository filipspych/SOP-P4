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
#define ERR(source) (perror(source),                                 \
                     fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), \
                     exit(EXIT_FAILURE))
#define DEBUG_MODE false
#define D(msg) if(DEBUG_MODE) fprintf(stderr, "DEBUG %ld t=%ld ln=%ld: %s \n", pthread_self(), time(0), __LINE__, msg)
#define FILE_INPUT_BUFF_SIZE MAX_RACER_NAME_LENGTH

#define MAX_INPUT_LENGTH 255
volatile sig_atomic_t shouldPrint = false;


struct Racer_s;
typedef struct Racer_s Racer;

struct Race_s;
typedef struct Race_s Race;

void *intQuitHandler(void *args);

void sigusr1Handler(int sig);

void cmdStart(Race *race);

void setHandler(void (*f)(int), int sigNo);

void *racer_t(void *args);

void readArguments(int argc, char **argv, char *inputFilePath, char *outputFilePath, Race *race);

void validateArgs(int threadCount, const char *inputFilePath);

void welcome();

void usage();

void badArgFor(char opt);

void badArg();

void giveNames(char *path, int racerCount, Racer *racers);

bool isUserInput();

void readNamesFromFile(int fd, int racerCount, Racer *racers);

void generateNames(int racerCount, Racer *racers);

double getRandomDouble(double min, double max);

void initRacers(Race *race);

void sendSigusr1(pthread_t target);

void setSigusr1Handling();

void initRace(Race *race, Racer *racers, Racer **sortedRacers, char *path);

void setIntQuitHandling();

void printRaceState(Race *r);

void handleUserCommand(Race *race, bool *reconfigurationNeeded);

bool isRaceFinished(Race *r);

void cmdCancel();

void cmdInfo(Race *r);

void initSortedRacers(struct Racer_s *racers, struct Racer_s **sortedRacers, int count);

void cmdChangeLaps(Race *r);

void cmdChangeLength(Race *r);

void cmdExit(Race *r);

void cmdAddParticipant(Race *r);

void initRacer(Racer *racer, int ID, Race *race);

void cmdResults(Race *r);

void printResults(Race *r);

void printRacer(Racer *racer);

void cmdFault(Race *r);

void sortResults(Race *r);

Racer *getRacer(char *name, Race *r);

void cmdCheat(Race *r);

Racer *chooseParticipant(Race *r);

void cmdDropout(Race *r);

struct Race_s {
    int racerCount, lapCount;
    double lapLen;
    pthread_t mainThreadID;
    Racer *racers;
    Racer **sortedRacers;
    bool started;
    pthread_mutex_t raceStateMx;
};

struct Racer_s {
    int racerID;
    char name[MAX_RACER_NAME_LENGTH];
    double v;
    Race *race;
    int lastFinishedLap;
    long lastFinishedLapTimestamp;
    bool droppedOut;
    bool hasUnreportedProgress;
};

int main(int argc, char **argv) {
    setIntQuitHandling();
    srand(time(NULL));

    char inputFilePath[MAX_FILEPATH_LEN + 1], outputPath[MAX_FILEPATH_LEN + 1];
    Racer racers[MAX_RACER_COUNT];
    Racer *sortedRacers[MAX_RACER_COUNT];
    Race race;

    readArguments(argc, argv, inputFilePath, outputPath, &race);
    welcome();

    while (true) {
        initRace(&race, racers, sortedRacers, inputFilePath);
        while (true) {
            bool reconfigurationNeeded = false;
            handleUserCommand(&race, &reconfigurationNeeded);
            if (reconfigurationNeeded) break;
            if (shouldPrint) {
                shouldPrint = false;
                printRaceState(&race);
                if (isRaceFinished(&race)) return EXIT_SUCCESS;
            }
        }
    }


}

void initSortedRacers(struct Racer_s racers[8], struct Racer_s *sortedRacers[8], int count) {
    for (int i = 0; i < count; ++i) {
        sortedRacers[i] = racers++;
    }
}

void cmdStart(Race *race) {
    // starting all racers' threads
    race->started = true;
    setSigusr1Handling();
    for (int i = 0; i < race->racerCount; i++) {
        pthread_t ignore;
        int err = pthread_create(&ignore, NULL, racer_t, (void *) &race->racers[i]);
        if (err != 0)
            ERR("Couldn't create a racer thread");
    }
    printf("THE RACE BEGINS\n");
}

bool isRaceFinished(Race *r) {
    for (int i = 0; i < r->racerCount; ++i) {
        if (!r->racers[i].droppedOut && r->racers[i].lastFinishedLap < r->lapCount)
            return false;
    }
    return true;
}

void handleUserCommand(Race *race, bool *reconfigurationNeeded) {
    char buf[MAX_INPUT_LENGTH] = {};
    fgets(buf, MAX_INPUT_LENGTH, stdin);
    buf[strcspn(buf, "\n")] = 0;

    if (0 == strcmp(buf, "info")) {
        cmdInfo(race);
    } else if (0 == strcmp(buf, "exit")) {
        cmdExit(race);
    } else if (0 == strcmp(buf, "start")) {
        if (race->started) printf("%s FAILED: this command is disabled during the race\n", buf);
        else cmdStart(race);
    } else if (0 == strcmp(buf, "change_laps")) {
        if (race->started) printf("%s FAILED: this command is disabled during the race\n", buf);
        else {
            *reconfigurationNeeded = true;
            cmdChangeLaps(race);
        }
    } else if (0 == strcmp(buf, "change_length")) {
        if (race->started) printf("%s FAILED: this command is disabled during the race\n", buf);
        else {
            *reconfigurationNeeded = true;
            cmdChangeLength(race);
        }
    } else if (0 == strcmp(buf, "add_participant")) {
        if (race->started) printf("%s FAILED: this command is disabled during the race\n", buf);
        else if (race->racerCount + 1 <= MAX_RACER_COUNT) cmdAddParticipant(race);
        else printf("%s FAILED: too many participants!\n");
    } else if (0 == strcmp(buf, "results")) {
        if (!race->started) printf("%s FAILED: this command is disabled before the race\n", buf);
        else cmdResults(race);
    } else if (0 == strcmp(buf, "cancel")) {
        if (!race->started) printf("%s FAILED: this command is disabled before the race\n", buf);
        else cmdCancel(race);
    } else if (0 == strcmp(buf, "fault")) {
        if (!race->started) printf("%s FAILED: this command is disabled before the race\n", buf);
        else cmdFault(race);
    } else if (0 == strcmp(buf, "cheat")) {
        if (!race->started) printf("%s FAILED: this command is disabled before the race\n", buf);
        else cmdCheat(race);
    } else if (0 == strcmp(buf, "dropout")) {
        if (!race->started) printf("%s FAILED: this command is disabled before the race\n", buf);
        else cmdDropout(race);
    } else if (buf[0] == 0) {
        return;
    } else {
        printf("%s FAILED: unknown command\n", buf);
    }
}

void cmdDropout(Race *r) {
    Racer *racer = chooseParticipant(r);

}

void cmdCheat(Race *r) {
    Racer *racer = chooseParticipant(r);

    double increase = 5;
    double oldV = racer->v;
    racer->v *= increase;
    printf("SUCCESS: %s's speed increased by %f%% is now %f [m/s] (used to be %f [m/s])\n",
           racer->name, increase * 100, racer->v, oldV);
}

void cmdFault(Race *r) {
    Racer *racer = chooseParticipant(r);

    double decrease = getRandomDouble(0.1, 0.3);
    double oldV = racer->v;
    racer->v *= (1 - decrease);
    printf("SUCCESS: %s's speed decreased by %f%% is now %f [m/s] (used to be %f [m/s])\n",
           racer->name, decrease * 100, racer->v, oldV);
}

Racer *chooseParticipant(Race *r) {
    char buf[MAX_INPUT_LENGTH];
    while (NULL == fgets(buf, MAX_INPUT_LENGTH, stdin));
    buf[strcspn(buf, "\n")] = 0;

    Racer *racer = getRacer(buf, r);
    if (racer == NULL)
        printf("COMMAND FAILED: No participant named \"%s\"\n", buf);

    return racer;
}

Racer *getRacer(char *name, Race *r) {
    for (int i = 0; i < r->racerCount; ++i)
        if (strcmp((r->racers + i)->name, name) == 0) return r->racers + i;
    return NULL;
}

void cmdResults(Race *r) {
    printResults(r);
}

void printResults(Race *r) {
    pthread_mutex_lock(&r->raceStateMx);
    sortResults(r);
    printf("\n----------SCOREBOARD---------\n");
    for (int i = 0; i < r->racerCount; ++i) {
        printf("%d. ", i);
        printRacer(r->sortedRacers[i]);
    }
    printf("-----------------------------\n\n");
    pthread_mutex_unlock(&r->raceStateMx);
}

void sortResults(Race *r) {
//todo
}

void printRacer(Racer *racer) {
    if (racer->droppedOut) printf("DROPPED OUT");
    printf("%s\t\tlap: %d time: %ld [ms]\n",
           racer->name, racer->lastFinishedLap, racer->lastFinishedLapTimestamp);
}

void cmdAddParticipant(Race *r) {
    r->racerCount++;

    char buf[MAX_INPUT_LENGTH];
    fgets(buf, MAX_INPUT_LENGTH, stdin);

    Racer *newRacer = &(r->racers[r->racerCount - 1]);
    initRacer(newRacer, r->racerCount - 1, r);
    strcpy(newRacer->name, buf);
}

void cmdExit(Race *r) {
    exit(EXIT_SUCCESS); //todo
}

void cmdChangeLength(Race *r) {
    char buf[MAX_INPUT_LENGTH];
    fgets(buf, MAX_INPUT_LENGTH, stdin);
    r->lapLen = atol(buf);
}

void cmdChangeLaps(Race *r) {
    char buf[MAX_INPUT_LENGTH];
    fgets(buf, MAX_INPUT_LENGTH, stdin);
    r->lapCount = atoi(buf);
}

void cmdInfo(Race *r) {
    if (r->racerCount == 0) {
        printf("No participants in this race\n");
        return;
    }

    printf("--------------RACE-------------\nThis race has %d lap(s).\nLap length: %f [m]\n",
           r->racers[0].race->lapCount,
           r->racers[0].race->lapLen);
    printf("----------PARTICIPANTS---------\nThere are %d participants in this race:\n", r->racerCount);
    for (int i = 0; i < r->racerCount; ++i) {
        printf("#%d %s\n\tv = %f [m/s]\n", r->racers[i].racerID, r->racers[i].name, r->racers[i].v);
    }

}

void cmdCancel() {
    exit(EXIT_SUCCESS); //todo
}

void printRaceState(Race *r) {
    pthread_mutex_lock(&r->raceStateMx);
    sortResults(r);
    Racer **iter = r->sortedRacers;
    for (int i = 0; i < r->racerCount; ++i) {
        if ((*iter)->hasUnreportedProgress) {
            (*iter)->hasUnreportedProgress = false;
            printRacer(*iter);
        }
        iter++;
    }
    pthread_mutex_unlock(&r->raceStateMx);
    if (isRaceFinished(r)) {
        printResults(r);
    }
}

void setIntQuitHandling() {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGQUIT);
    pthread_sigmask(SIG_BLOCK, &set, NULL);

    pthread_t ignore;
    int err = pthread_create(&ignore, NULL, intQuitHandler, NULL);
    if (err != 0)
        ERR("Couldn't create an aux thread");
}

void *intQuitHandler(void *args) {
    //todo
}

void setSigusr1Handling() {
    setHandler(sigusr1Handler, SIGUSR1);
}

void initRace(Race *race, Racer *racers, Racer **sortedRacers, char *path) {
    race->started = false;
    race->mainThreadID = pthread_self();
    race->racers = racers;
    race->sortedRacers = sortedRacers;
    race->raceStateMx = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;

    initRacers(race);
    initSortedRacers(racers, sortedRacers, race->racerCount);
    giveNames(path, race->racerCount, racers);
}

void initRacers(Race *race) {
    for (int i = 0; i < race->racerCount; ++i) {
        initRacer(race->racers + i, i, race);
    }
}

void initRacer(Racer *racer, int ID, Race *race) {
    racer->race = race;
    racer->racerID = ID;
    racer->v = getRandomDouble(0.095, 0.105) * race->lapLen;
    racer->lastFinishedLap = 0;
    racer->lastFinishedLapTimestamp = 0;
    racer->droppedOut = false;
    racer->hasUnreportedProgress = false;
}

double getRandomDouble(double min, double max) {
    return ((min) + (double) rand() / RAND_MAX * (max - min));
}

void giveNames(char *path, int racerCount, Racer *racers) {
    int fd;
    if (path[0] != '\0') {
        if ((fd = open(path, O_RDONLY)) == -1)
            ERR("open");
    } else if (isUserInput()) {
        fd = STDIN_FILENO;
    } else {
        generateNames(racerCount, racers);
        return;
    }
    readNamesFromFile(fd, racerCount, racers);
}

void generateNames(int racerCount, Racer racers[MAX_RACER_COUNT]) {
    char name[RACER_DEFAULT_NAME_LENGTH + 1] = "Racer 1";
    for (int i = 0; i < racerCount; i++) {
        strcpy(racers[i].name, name);
        name[RACER_DEFAULT_NAME_LENGTH - 1]++;
    }
}

void readNamesFromFile(int fd, int racerCount, Racer racers[MAX_RACER_COUNT]) {
    char *buf;
    if ((buf = (char *) malloc(sizeof(char) * FILE_INPUT_BUFF_SIZE)) == NULL)
        ERR("Memory allocation error");

    for (int i = 0; i < racerCount; i++) {
        char tmp[MAX_RACER_NAME_LENGTH];
        FILE *file = fdopen(fd, "r"); //todo delete this
        fscanf(file, "%s\n", tmp);    // todo change this
        // TODO: low level IO
        //        while(read(fd, buf, FILE_INPUT_BUFF_SIZE)) {
        //
        //        }
        strcpy(racers[i].name, tmp);
    }
}

bool isUserInput() {
    char buf[MAX_INPUT_LENGTH] = {};
    do {
        printf("Do you want to name your racers? [y/n]: ");
        while (NULL == fgets(buf, MAX_INPUT_LENGTH, stdin));
    } while ('y' != buf[0] && 'n' != buf[0]);
    return 'y' == buf[0];
}

void welcome() {
    char *name = getenv("USER");
    printf("Hello %s!\n", name);
}

void readArguments(int argc, char **argv, char *inputFilePath, char *outputFilePath, Race *race) {
    race->racerCount = -1;
    inputFilePath[0] = '\0';
    race->lapLen = -1;
    race->lapCount = DEFAULT_LAPS;
    outputFilePath[0] = '\0';
    //    opterr = 0;

    bool namesInputMethodSet = false;
    char c;
    while ((c = getopt(argc, argv, "n:p:l:o:f:")) != -1) {

        switch (c) {
            case 'n':
                if (namesInputMethodSet)
                    badArg();
                race->racerCount = atoi(optarg);
                if (race->racerCount > MAX_RACER_COUNT || race->racerCount < MIN_RACER_COUNT)
                    badArgFor(c);
                namesInputMethodSet = true;
                break;
            case 'p':
                if (namesInputMethodSet)
                    badArg();
                strcpy(inputFilePath, optarg);
                //                if(strlen(inputFilePath) == 0) //todo possible?
                namesInputMethodSet = true;
                break;
            case 'l':
                race->lapLen = atoi(optarg);
                if (race->lapLen > MAX_LAP_LEN || race->lapLen < MIN_LAP_LEN)
                    badArgFor(c);
                break;
            case 'o':
                race->lapCount = atoi(optarg);
                if (race->lapCount > MAX_LAP_COUNT || race->lapCount < MIN_LAP_COUNT)
                    badArgFor(c);
                break;
            case 'f':
                strcpy(outputFilePath, optarg);
                break;
            case '?':
                if (strchr("nplof", optopt) != NULL)
                    fprintf(stderr, "No argument for -%c given!\n", optopt);
                else
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                usage();
            default:
                ERR("Could not interpret supplied arguments");
        }
    }
    validateArgs(race->racerCount, inputFilePath);
}

void validateArgs(int threadCount, const char *inputFilePath) {
    if ((threadCount == -1 && inputFilePath[0] == '\0') || (threadCount != -1 && inputFilePath[0] != '\0'))
        badArg();
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

void onLapCompleted(Racer *thisRacer, int curCompletedLaps, long raceStartTimestamp) {
    D("onLapCompleted");
    pthread_mutex_lock(&thisRacer->race->raceStateMx);
    D("onLapCompletedAfterMutex");
    thisRacer->lastFinishedLap = curCompletedLaps;

    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    long nowTimestamp = (now.tv_sec * 1000 + now.tv_nsec / 10000);
    thisRacer->lastFinishedLapTimestamp = nowTimestamp - raceStartTimestamp;

    thisRacer->hasUnreportedProgress = true;
    sendSigusr1(thisRacer->race->mainThreadID);
    pthread_mutex_unlock(&thisRacer->race->raceStateMx);
}

void *racer_t(void *args) {
    Racer *thisRacer = args;
    struct timespec lastLapTimestamp;
    clock_gettime(CLOCK_REALTIME, &lastLapTimestamp);

    double dist = 0;
    long raceStartTimestamp = lastLapTimestamp.tv_sec * 1000 + lastLapTimestamp.tv_nsec / 10000;
    while (true) {
        sleep(1);
        dist += getRandomDouble(0.9, 1.1) * thisRacer->v;
        int curCompletedLaps;
        if ((curCompletedLaps = dist / thisRacer->race->lapLen) > thisRacer->lastFinishedLap) {
            onLapCompleted(thisRacer, curCompletedLaps, raceStartTimestamp);
            if (thisRacer->lastFinishedLap >= thisRacer->race->lapCount) {
                // finished
                return NULL;
            }
        }
    }
}

void sendSigusr1(pthread_t target) {
    pthread_kill(target, SIGUSR1);
}

void sigusr1Handler(int sig) {
    shouldPrint = true;
}

void setHandler(void (*f)(int), int sigNo) {
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}