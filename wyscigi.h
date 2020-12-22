#define UNINITIALIZED -1
#define RANDOM_1_MIN 0.9
#define RANDOM_1_MAX 1.1
#define CALLOFF_THRESHOLD 2
#define CHEAT_RATIO 5
#define PERCENTAGE 100
#define RANDOM_2_MIN 0.1
#define RANDOM_2_MAX 0.3
#define RAND_3_MIN 0.095
#define RAND_3_MAX 0.105
#define DEFAULT_LAPS 1
#define SLEEP_SECONDS 1
#define MAX_RACER_NAME_LENGTH 256
#define RACER_DEFAULT_NAME_LENGTH 7
#define MAX_FILEPATH_LEN 256
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
#define D(msg) if(DEBUG_MODE) fprintf(stderr, "DEBUG %ld t=%ld ln=%d: %s \n", pthread_self(), time(0), __LINE__, msg)
#define FILE_INPUT_BUFF_SIZE MAX_RACER_NAME_LENGTH
#define MAX_INPUT_LENGTH 256
#define MAX_OUTPUT_LENGTH 2048

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

void validateArgs(int racerCount, const char *inputFilePath);

void welcome();

void usage();

void badArgFor(int opt);

void badArg();

void giveNames(char *path, int *racerCount, Racer *racers);

bool isUserInput();

void readNamesFromFile(int fd, int *racerCount, Racer *racers);

void generateNames(int racerCount, Racer *racers);

double getRandomDouble(double min, double max, int *pInt);

void initRacers(Race *race);

void sendSigusr1(pthread_t target);

void setSigusr1Handling();

void initRace(Race *race, Racer *racers, Racer **sortedRacers, char *path);

void setIntQuitHandling();

void printRaceState(Race *r);

void handleUserCommand(Race *race, bool *reconfigurationNeeded);

bool isRaceFinished(Race *r);

void cmdCancel(Race *r);

void cmdInfo(Race *r);

void initSortedRacers(struct Racer_s *racers, struct Racer_s **sortedRacers);

void cmdChangeLaps(Race *r);

void cmdChangeLength(Race *r);

void cmdExit(Race *r);

void cmdAddParticipant(Race *r);

void initRacer(Racer *racer, int ID, Race *race);

void cmdResults(Race *r);

void printResults(Race *r, int fd);

void printRacer(Racer *racer, char *buf);

void cmdFault(Race *r);

void sortResults(Race *r);

Racer *getRacer(char *name, Race *r);

void cmdCheat(Race *r);

Racer *selectParticipant(Race *r);

void cmdDropout(Race *r);

bool askUser(char *question);

void resetRace(Race *r);

void readNamesFromStdin(int count, Racer *racers);

void printRaceToFile(Race *r, char *path);
