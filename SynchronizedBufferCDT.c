#include "SynchronizedSharedBufferADT.h"

struct SynchronizedSharedBufferCDT {
    char * sharedMemoryBaseAddress;
    sem_t * mutexSemaphore; 
    sem_t * fullBufferSemaphore; 
    size_t bufferSize; 
    int sharedMemoryFd; 
    char * sharedMemoryPath; // idk what this is for 
    char * mutexSemaphorePath; 
    char * fullBufferSemaphorePath;
};

void handleError(char * errorMsg) { // please implement this function or make it as a define macro
    // notify stderr and exit
}

SynchronizedSharedBufferADT createSynchronizedSharedBuffer(const restrict char * id, size_t bufferSize) {
    SynchronizedSharedBufferADT sharedBuffer = (SynchronizedSharedBufferADT) malloc(sizeof(struct SynchronizedSharedBufferCDT));
    if (sharedBuffer == NULL) {
        handleError("malloc");
    }

    sharedBuffer->bufferSize = bufferSize; 
    sharedBuffer->sharedMemoryPath = "/shm" + id; // FixMe 
    sharedBuffer->mutexSemaphorePath = "/sem-mutex" + id; // FixMe
    sharedBuffer->fullBufferSemaphorePath = "/sem-full" + id; // FixMe
}

void unlinkPreviousResources(SynchronizedSharedBufferADT sharedBuffer) {
    shm_unlink(sharedBuffer->sharedMemoryPath);
    shm_unlink(sharedBuffer->fullBufferSemaphorePath);
    shm_unlink(sharedBuffer->mutexSemaphorePath);
}

inline void unlinkPreviousResources(SynchronizedSharedBufferADT sharedBuffer) {// what is inline for? 
    shm_unlink(sharedBuffer->sharedMemoryPath);
    sem_unlink(sharedBuffer->fullBufferSemaphore);
    shm_unlink(sharedBuffer->mutexSemaphorePath);
}

inline void createResources(SynchronizedSharedBufferADT sharedBuffer) {
    if ((sharedBuffer->mutexSemaphore) = sem_open(sharedBuffer->mutexSemaphorePath, O_CREAT | O_EXCL, 0660, 1) == SEM_FAILED) {
        handleError("sem_open");
    }
     if ((sharedBuffer->fullBufferSemaphore) = sem_open(sharedBuffer->fullBufferSemaphorePath, O_CREAT | O_EXCL, 0660, 1) == SEM_FAILED) {
        handleError("sem_open");
    }

    if ((sharedBuffer->sharedMemoryFd, sharedBuffer->bufferSize + sizeof(long))<0) { // no entiendo aca porque agrega sizeof(long)
        handleError("shm_open");
    }
    if (ftruncate(sharedBuffer->sharedMemoryFd, sharedBuffer->bufferSize+sizeof(long)) < 0) {
        handleError("ftruncate");
    }
    if ((sharedBuffer->sharedMemoryBaseAddress = mmap(NULL, sharedBuffer->bufferSize + sizeof(long), PROT_READ | PROT_WRITE, MAP_SHARED, sharedBuffer->sharedMemoryFd, 0))) {
        handleError("mmap");
    }
}

SynchronizedSharedBufferADT openSynchronizedSharedBuffer(const restrict char * id, size_t bufferSize) {
    SynchronizedSharedBufferADT sharedBuffer = createBaseSynchronizedSharedBuffer(id, bufferSize);
    openResources(sharedBuffer); 
    return sharedBuffer;
}

void openResources(SynchronizedSharedBufferADT sharedBuffer) {
    if ((sharedBuffer->mutexSemaphore = sem_open(sharedBuffer->mutexSemaphorePath, 0, 0660, 1)) == SEM_FAILED) {
        handleError("sem_open");
    }
    if ((sharedBuffer->fullBufferSemaphore = sem_open(sharedBuffer->fullBufferSemaphorePath, 0, 0660, 1)) == SEM_FAILED) {
        handleError("sem_open");
    }

    if ((sharedBuffer->sharedMemoryFd = shm_open(sharedBuffer->sharedMemoryPath, O_RDWR, S_IWUSR | S_IRUSR)) < 0) {
        handleError("shm_open");
    }

    if ((sharedBuffer->sharedMemoryBaseAddress = mmap(NULL, sharedBuffer->bufferSize+sizeof(long), PROT_WRITE | PROT_READ, MAP_SHARED, &sharedBuffer->sharedMemoryFd, 0))) {
        handleError("mmap");
    }
}

void closeSynchronizedSharedBuffer(SynchronizedSharedBufferADT sharedBuffer) {
    if (munmap(sharedBuffer->sharedMemoryBaseAddress, sharedBuffer->bufferSize+sizeof(long))<0) {
        handleError("munmap");
    }
    if (sem_close(sharedBuffer->mutexSemaphore)<0) {
        handleError("sem_unlink");
    }
    if (sem_close(sharedBuffer->fullBufferSemaphore)<0) {
        handleError("sem_unlink");
    }
    if ((close(sharedBuffer->sharedMemoryFd))<0) {
        handleError("close");
    }
    free(sharedBuffer);
}

ssize_t writeSynchronizedBuffer(SynchronizedSharedBufferADT sharedBuffer, const void * buffer, size_t size) {
    sem_wait(sharedBuffer->mutexSemaphore);
    // TODO: write

    sem_post(sharedBuffer->mutexSemaphore);
    sem_post(sharedBuffer->fullBufferSemaphore);
}

ssize_t readSynchronizedSharedBuffer(SynchronizedSharedBufferADT sharedBuffer, void *buffer, size_t size) {
    sem_wait(sharedBuffer->fullBufferSemaphore);
    sem_wait(sharedBuffer->mutexSemaphore);
    //TODO: read
    sem_post(sharedBuffer->fullBufferSemaphore);
}


/// app.c

inline char * generateSharedBufferIdentifier(); 
inline size_t getDerivedNUmberOfWorkers(size_t filesQuantity);
void createCommunicateWorkers(size_t numberOfWorksers, pid_t workersPids[], int appToWorkerPipes[][2], int workerToAppPipes[][2]...);
void distributeFileAndWriteResults(size_t numberOfFiles, const char * files[], size_t numberOfWorksers, int appToWorkerPipes....);
void waitWorkers(pid_t workersPids[]);

int main(int argc, char const *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "How to use: %s <file1> <file2> ... \n", argv[0]);
        return 1;
    }

    SynchronizedSharedBufferADT sharedBuffer = createSynchronizedSharedBuffer(generateSharedBufferIdentifier());
    FILE * outputFile = fopen("./output.txt", "w"); // FIxMe: handle error
    size_t numberOfFiles = argc - 1; 
    size_t numberOfWorksers = getDerivedNUmberOfWorkers(numberOfFiles);
    pid_t workersPids[numberOfWorksers];
    int appToWorkerPipes[numberOfWorksers][2]; 
    int workerToAppPipes[numberOfWorksers][2];

    sleep(2); 

    createAndCommunicateWorkers(numberOfWorksers, workersPids, appToWorkerPipes, workerToAppPipes);

    distributeFileAndWriteResults(numberOfFiles, argv, numberOfWorksers, appToWorkerPipes, workerToAppPipes, sharedBuffer);

    waitWorkers(workersPids);
    destroySynchronizedSharedBuffer(sharedBuffer);
    return 0; 

}