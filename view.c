// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http:\/\/www.viva64.com

#include "lib.h"
#include <signal.h>

#define NAME_SIZE 10

int main(int argc, char *argv[]) {
    fprintf(stderr,"%d\n", argc);

    fprintf(stderr, "In view...\n");

    char shmName[NAME_SIZE]={0}; 
    /*initializing to avoid PVS*/
    char semName[NAME_SIZE]={0};
    // char semDone[NAME_SIZE]={0};
    int shmFd;
    sem_t * semaphore; 
    SharedMemoryStruct * shmData = NULL; 


    int n; // FixMe: needs a more descriptive name
    

    for (int i = 0; i < argc; i++) {
        fprintf(stderr, "view %d>> %s\n", i, argv[i]);
    }
    
    // caso pipe
    //use scanf to tokenize
    if (argc == 1) {
    /*I set  %9s to avoid the PVS warning */
        if((n = scanf("%9s" ,shmName)) < 0 ){
            fprintf(stderr, "Error reading input\n");
            exit(ERROR);
        }

        fprintf(stderr,"View know shm is: %s\n", shmName);

        if((n = scanf("%9s", semName)) < 0){
            fprintf(stderr, "Error reading input\n");
            exit(ERROR);
        }

        fprintf(stderr, "View knows sem is: %s\n",semName);
    }

    //caso por parámetro -> que pasa con el semaforo?
    else if (argc == 3) { // caso argc == 3 
        strncpy(shmName, argv[1], sizeof(shmName) - 1);
        strncpy(semName, argv[2], sizeof(semName) - 1); // FIxMe: is this how its going to work? 
    } 
    else { 
        ERROR_EXIT("Parameters missing...\n");
    }
    // sem_t * done_semaphore = sem_open(semDone, O_RDONLY, 0); || done_semaphore == SEM_FAILED
    if ((semaphore = sem_open(semName, 0)) == SEM_FAILED ) {
        ERROR_EXIT("Error opening semaphore in view\n");
    } 

    fprintf(stderr, "Attempting to open shared memory: %s\n", shmName);

    if ((shmFd = shm_open(shmName, O_RDONLY, S_IRUSR | S_IWUSR)) == ERROR) {
        ERROR_EXIT("Error opening / reading shared memory\n");
    }
    
    struct stat shmStat; 
    if (fstat(shmFd, &shmStat) == ERROR) {
        ERROR_EXIT("Error getting shared memory size");
    }

    if ((shmData = mmap(NULL, shmStat.st_size, PROT_READ, MAP_SHARED, shmFd, 0)) == MAP_FAILED) {
        ERROR_EXIT("Error mapping shared memory\n");
    }

    fprintf(stderr, "Shared memory mapped at address: %p\n", (void*)shmData);

    
    //write header 
    printf(HEADER); // FixMe: writing header twice?? why is it also in app.c 
    fflush(stdout);

    char buffer[BUFFER_SIZE];
    size_t readIdx = 0;
    int sem_return = 0;
    while(1){ 
        if ((sem_return = sem_wait(semaphore)) < 0) {
            fprintf(stderr, "sem_wait failed");
        }
        fprintf(stderr,"im here\n");

        // FIxMe: consider using atomic 
        size_t currentSize = BUFFER_SIZE;

        while (readIdx < currentSize) {
            size_t len = 0;
            while (readIdx + len < currentSize && shmData->buffer[readIdx + len] != '\n') {
                len++;
            }

            if (readIdx + len < currentSize) {
                memcpy(buffer, &shmData->buffer[readIdx], len);
                buffer[len] = '\0';
                printf("view>> %s\n", buffer);
                readIdx += len + 1; // skip newline 
            } else {
                break;
            }    
        }    
        
        if(atomic_load(&shmData->done) == 1 && readIdx >= currentSize){
            break;
        }
    }
    sem_close(semaphore);
    munmap(shmData, shmStat.st_size);
    close(shmFd);

    return 0;
}