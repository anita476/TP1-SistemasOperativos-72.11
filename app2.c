#include "SynchronizedBufferADT.h"
/// app.c

inline char * generateSharedBufferIdentifier(); 
inline size_t getDerivedNumberOfWorkers(size_t filesQuantity);
void createCommunicateWorkers(size_t numberOfWorkers, pid_t workersPids[], int appToWorkerPipes[][2], int workerToAppPipes[][2]...);
void distributeFileAndWriteResults(size_t numberOfFiles, const char * files[], size_t numberOfWorkers, int appToWorkerPipes....);
void waitWorkers(pid_t workersPids[]);

int main(int argc, char const *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "How to use: %s <file1> <file2> ... \n", argv[0]);
        return 1;
    }

    setvbuf(stdout, NULL, _IONBF, 0);

    SynchronizedSharedBufferADT sharedBuffer = createSynchronizedSharedBuffer(generateSharedBufferIdentifier());
    FILE * outputFile = fopen("./output.txt", "w"); // FIxMe: handle error
    if (outputFile == NULL) {
        handleError("fopen output.txt");
    }

    size_t numberOfFiles = argc - 1; 
    size_t numberOfWorkers = getDerivedNumberOfWorkers(numberOfFiles);
    pid_t workersPids[numberOfWorkers];
    int appToWorkerPipes[numberOfWorkers][2]; 
    int workerToAppPipes[numberOfWorkers][2];

    sleep(2); 

    createAndCommunicateWorkers(numberOfWorkers, workersPids, appToWorkerPipes, workerToAppPipes);

    distributeFileAndWriteResults(numberOfFiles, argv, numberOfWorkers, appToWorkerPipes, workerToAppPipes, sharedBuffer);

    waitWorkers(workersPids);
    destroySynchronizedSharedBuffer(sharedBuffer);
    return 0; 
}

size_t getDerivedNumberOfWorkers(int numberOfFiles) {
    int logic = floor(numFiles / 10);
    if (logic < 1) return numFiles;
    else { 
        fprintf(stderr, "Slaves num: %d\n", SLAVES * logic);
        return (SLAVES * logic);
    }
}

void createAndCommunicateWorkers(numberOfWorkers, workersPids, appToWorkerPipes, workerToAppPipes) {

}
