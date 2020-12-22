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
#include "wyscigi.h"

volatile sig_atomic_t shouldPrint = false;

struct Race_s {
    int racerCount, lapCount;
    double lapLen;
    pthread_t mainThreadID;
    volatile bool started;
    pthread_mutex_t raceStateMx;
    volatile bool canceled;
    volatile bool finished;
    Racer **sortedRacers;
    Racer *racers;
};

struct Racer_s {
    pthread_t tid;
    int racerID;
    char name[MAX_RACER_NAME_LENGTH];
    double v;
    int lastFinishedLap;
    long lastFinishedLapTimestamp;
    volatile bool droppedOut;
    volatile bool hasUnreportedProgress;
    int seed;
    Race *race;
};

void waitForChildren(Race *r);

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
                if (isRaceFinished(&race)) {
                    if (outputPath[0] != '\0') printRaceToFile(&race, outputPath);
                    resetRace(&race);
                }
            }
        }
    }


}

void printRaceToFile(Race *r, char *path) {
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC);
    printResults(r, fd);
}

void resetRace(Race *r) {
    r->started = false;
    r->canceled = false;
    initRacers(r);
}

void initSortedRacers(struct Racer_s *racers, struct Racer_s **sortedRacers) {
    for (int i = 0; i < MAX_RACER_COUNT; ++i) {
        sortedRacers[i] = racers++;
    }
}

void cmdStart(Race *race) {
    // starting all racers' threads
    race->started = true;
    race->canceled = false;
    race->finished = false;
    setSigusr1Handling();
    for (int i = 0; i < race->racerCount; i++) {
        int err = pthread_create(&race->racers[i].tid, NULL, racer_t, (void *) &race->racers[i]);
        if (err != 0)
            ERR("Couldn't create a racer thread");
    }
    printf("THE RACE BEGINS\n");
}

bool isRaceFinished(Race *r) {
    if (r->finished) return true;

    bool oneRacer = false;
    for (int i = 0; i < r->racerCount; ++i) {
        if (!r->racers[i].droppedOut && r->racers[i].lastFinishedLap < r->lapCount) {
            if (oneRacer) {
                return false;
            } else oneRacer = true;
        }
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
        else printf("%s FAILED: too many participants!\n", buf);
    } else if (0 == strcmp(buf, "results")) {
        if (!race->started) printf("%s FAILED: this command is disabled before the race\n", buf);
        else cmdResults(race);
    } else if (0 == strcmp(buf, "cancel")) {
        if (!race->started) printf("%s FAILED: this command is disabled before the race\n", buf);
        else {
            *reconfigurationNeeded = true;
            cmdCancel(race);
        }
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
    selectParticipant(r)->droppedOut = true;
    int racersLeft = 0;
    for (int i = 0; i < r->racerCount; ++i) {
        if (!r->racers[i].droppedOut) racersLeft++;
    }
    if (racersLeft < CALLOFF_THRESHOLD) {
        r->finished = true;
        sendSigusr1(r->mainThreadID);
    }
}

void cmdCheat(Race *r) {
    Racer *racer = selectParticipant(r);

    double increase = CHEAT_RATIO;
    double oldV = racer->v;
    racer->v *= increase;
    printf("SUCCESS: %s's speed increased by %f%% is now %f [m/s] (used to be %f [m/s])\n",
           racer->name, increase * PERCENTAGE, racer->v, oldV);
}

void cmdFault(Race *r) {
    Racer *racer = selectParticipant(r);

    double decrease = getRandomDouble(RANDOM_2_MIN, RANDOM_2_MAX, &racer->seed);
    double oldV = racer->v;
    racer->v *= (1 - decrease);
    printf("SUCCESS: %s's speed decreased by %f%% is now %f [m/s] (used to be %f [m/s])\n",
           racer->name, decrease * PERCENTAGE, racer->v, oldV);
}

Racer *selectParticipant(Race *r) {
    printf("Enter participant: ");
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
    printResults(r, STDOUT_FILENO);
}

void printResults(Race *r, int fd) {
    char buf[MAX_OUTPUT_LENGTH] = {};
    char buf2[MAX_OUTPUT_LENGTH] = {};
    strcat(buf, "\n----------SCOREBOARD---------\n");

    pthread_mutex_lock(&r->raceStateMx);
    sortResults(r);
    for (int i = 0; i < r->racerCount; ++i) {
        sprintf(buf2, "%d. ", i + 1);
        strcat(buf, buf2);
        printRacer(r->sortedRacers[i], buf2);
        strcat(buf, buf2);
    }
    strcat(buf, "-----------------------------\n\n");

    unsigned long toWrite = strlen(buf);
    char *output = buf;
    while (toWrite > 0) {
        int written;
        if ((written = write(fd, output, toWrite)) < 0) ERR("Couldn't write to buf file!");
        toWrite -= written;
        output += written;
    }
    pthread_mutex_unlock(&r->raceStateMx);
}

int comp(const void *elem1, const void *elem2) {
    Racer *f = *((Racer **) elem1);
    Racer *s = *((Racer **) elem2);
    if (f->lastFinishedLap > s->lastFinishedLap) return -1;
    if (f->lastFinishedLap < s->lastFinishedLap) return 1;
    if (f->lastFinishedLapTimestamp > s->lastFinishedLapTimestamp) return 1;
    if (f->lastFinishedLapTimestamp < s->lastFinishedLapTimestamp) return -1;
    return 0;
}

void sortResults(Race *r) {
    qsort(r->sortedRacers, r->racerCount, sizeof(Racer **), comp);
}

void printRacer(Racer *racer, char *buf) {
    buf[0] = '\0';
    char buf2[MAX_OUTPUT_LENGTH] = {};
    sprintf(buf2, "%s\t\tlap: %d time: %ld [ms]",
            racer->name, racer->lastFinishedLap, racer->lastFinishedLapTimestamp);
    strcat(buf, buf2);
    if (racer->droppedOut) {
        sprintf(buf2, "\tDROPPED OUT!");
        strcat(buf, buf2);
    }
    sprintf(buf2, "\n");
    strcat(buf, buf2);
}

void cmdAddParticipant(Race *r) {
    r->racerCount++;

    char buf[MAX_INPUT_LENGTH];
    fgets(buf, MAX_INPUT_LENGTH, stdin);
    buf[strcspn(buf, "\n")] = 0;

    Racer *newRacer = &(r->racers[r->racerCount - 1]);
    initRacer(newRacer, r->racerCount - 1, r);
    strcpy(newRacer->name, buf);
}

void cmdExit(Race *r) {
    if(r->started)
        cmdCancel(r);
    exit(EXIT_SUCCESS);
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
        printf("#%d %s\n\tv = %f [m/s]", r->racers[i].racerID, r->racers[i].name, r->racers[i].v);
        if (r->racers[i].droppedOut) printf("\tDROPPED OUT!");
        printf("\n");
    }
    printf("-------------------------------\n\n");

}

void cmdCancel(Race *r) {
    r->started = false;
    r->canceled = true;
    printf("CANCELING...\n");
    waitForChildren(r);
    printf("RACE CANCELED\n");
}

void waitForChildren(Race *r) {
    for (int i = 0; i < r->racerCount; i++)
        pthread_join(r->racers[i].tid, NULL);
}

void printRaceState(Race *r) {
    pthread_mutex_lock(&r->raceStateMx);
    sortResults(r);
    Racer **iter = r->sortedRacers;
    for (int i = 0; i < r->racerCount; ++i) {
        if ((*iter)->hasUnreportedProgress) {
            (*iter)->hasUnreportedProgress = false;
            char buf[MAX_OUTPUT_LENGTH];
            printRacer(*iter, buf);
            printf("%s", buf);
        }
        iter++;
    }
    pthread_mutex_unlock(&r->raceStateMx);
    if (isRaceFinished(r)) {
        printResults(r, STDOUT_FILENO);
    }
}

void setIntQuitHandling() {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGQUIT);
    if (pthread_sigmask(SIG_BLOCK, &set, NULL)) ERR("Cannot block SIGINT, SIGQUIT");

    pthread_t ignore;
    int err = pthread_create(&ignore, NULL, intQuitHandler, NULL);
    if (err != 0)
        ERR("Couldn't create an aux thread");
}

void *intQuitHandler(void *ignore) {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGQUIT);
    int sig;
    while (true) {
        if (0 == sigwait(&set, &sig)) {
            if (askUser("\nAre you sure you want to exit?")) exit(EXIT_SUCCESS);
        }
    }
}

bool askUser(char *question) {
    char buf[MAX_INPUT_LENGTH] = {};
    do {
        printf("%s [y/n]: ", question);
        while (NULL == fgets(buf, MAX_INPUT_LENGTH, stdin));
        buf[strcspn(buf, "\n")] = 0;
    } while ('y' != buf[0] && 'n' != buf[0]);
    return 'y' == buf[0];
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

    giveNames(path, &race->racerCount, racers);
    initRacers(race);
    initSortedRacers(racers, sortedRacers);
}

void initRacers(Race *race) {
    for (int i = 0; i < race->racerCount; ++i) {
        initRacer(race->racers + i, i, race);
    }
}

void initRacer(Racer *racer, int ID, Race *race) {
    racer->seed = rand();
    racer->race = race;
    racer->racerID = ID;
    racer->v = getRandomDouble(RAND_3_MIN, RAND_3_MAX, &racer->seed) * race->lapLen;
    racer->lastFinishedLap = 0;
    racer->lastFinishedLapTimestamp = 0;
    racer->droppedOut = false;
    racer->hasUnreportedProgress = false;
}

double getRandomDouble(double min, double max, int *seed) {
    return ((min) + (double) rand_r((unsigned int *) seed) / RAND_MAX * (max - min));
}

void giveNames(char *path, int *racerCount, Racer *racers) {
    if (path[0] != '\0') {
        int fd;
        if ((fd = open(path, O_RDONLY)) == -1)
            ERR("open");
        readNamesFromFile(fd, racerCount, racers);
    } else if (isUserInput()) {
        readNamesFromStdin(*racerCount, racers);
    } else {
        generateNames(*racerCount, racers);
        return;
    }
}

void readNamesFromStdin(int count, Racer *racers) {
    char buf[FILE_INPUT_BUFF_SIZE];
    for (int i = 0; i < count; i++) {
        fgets(buf, FILE_INPUT_BUFF_SIZE, stdin);
        buf[strcspn(buf, "\n")] = 0;
        strcpy(racers[i].name, buf);
    }
}

void generateNames(int racerCount, Racer racers[MAX_RACER_COUNT]) {
    char name[RACER_DEFAULT_NAME_LENGTH + 1] = "Racer 1";
    for (int i = 0; i < racerCount; i++) {
        strcpy(racers[i].name, name);
        name[RACER_DEFAULT_NAME_LENGTH - 1]++;
    }
}

void readNamesFromFile(int fd, int *racerCount, Racer racers[MAX_RACER_COUNT]) {
    char buf[FILE_INPUT_BUFF_SIZE + 1];
    *racerCount = 0;
    int r;
    while ((r = read(fd, buf, MAX_INPUT_LENGTH)) > 0) {
        int j = 0;
        for (int i = 0; i < r; ++i) {
            if (buf[i] != '\n') {
                racers[*racerCount].name[j++] = buf[i];
            } else {
                racers[*racerCount].name[j] = '\0';
                if (strlen(racers[*racerCount].name) > 0)
                    (*racerCount)++;
                j = 0;
            }
        }
    }
    if (r < 0) ERR("Couldn't read names file");
}

bool isUserInput() {
    return askUser("Do you want to name your racers?");
}

void welcome() {
    char *name = getenv("USER");
    printf("Hello %s!\n", name);
}

void readArguments(int argc, char **argv, char *inputFilePath, char *outputFilePath, Race *race) {
    race->racerCount = UNINITIALIZED;
    inputFilePath[0] = '\0';
    race->lapLen = UNINITIALIZED;
    race->lapCount = DEFAULT_LAPS;
    outputFilePath[0] = '\0';

    bool namesInputMethodSet = false;
    int c;
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
                break;
            default:
                ERR("Could not interpret supplied arguments");
        }
    }
    validateArgs(race->racerCount, inputFilePath);
}

void validateArgs(int racerCount, const char *inputFilePath) {
    if ((racerCount == UNINITIALIZED && inputFilePath[0] == '\0') || (racerCount != UNINITIALIZED && inputFilePath[0] != '\0'))
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

void badArgFor(int opt) {
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
        sleep(SLEEP_SECONDS);
        if (thisRacer->race->canceled || thisRacer->race->finished) return NULL;
        dist += getRandomDouble(RANDOM_1_MIN, RANDOM_1_MAX, &thisRacer->seed) * thisRacer->v;
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

void sigusr1Handler(int ignore) {
    shouldPrint = true;
}

void setHandler(void (*f)(int), int sigNo) {
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}