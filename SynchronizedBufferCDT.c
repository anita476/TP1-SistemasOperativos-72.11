#include "SynchronizedSharedBufferADT.h"

struct SynchronizedSharedBufferCDT {
    char * sharedMemoryBaseAddress;
    sem_t * mutexSemaphore; 
    sem_t * fullBufferSemaphore; 
    size_t bufferSize; 
    int sharedMemoryFd; 
    char * sharedMemoryPath; 
    char * mutexSemaphorePath; 
    char * fullBufferSemaphorePath;
};

void handleError(char * errorMsg) { // please implement this function or make it as a define macro
    perror(errorMsg); 
    exit(EXIT_FAILURE);
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



}